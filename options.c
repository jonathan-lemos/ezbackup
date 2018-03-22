/* options.c -- command-line/menu-based options parser
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/* prototypes */
#include "options.h"
/* errors */
#include "error.h"
#include <errno.h>
/* printf */
#include <stdio.h>
/* strcmp */
#include <string.h>
/* malloc */
#include <stdlib.h>
/* menus */
#include <ncurses.h>
#include <menu.h>
/* command-line tab completion */
#include <readline/readline.h>
/* is_directory() */
#include <sys/stat.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void version(void){
	const char* program_name = "ezbackup";
	const char* version      = "0.2 beta";
	const char* year         = "2018";
	const char* name         = "Jonathan Lemos";
	const char* license      = "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n\
								This is free software: you are free to change and redistribute it.\n\
								There is NO WARRANTY, to the extent permitted by law.";


	printf("%s %s\n", program_name, version);
	printf("Copyright (C) %s %s\n", year, name);
	printf("%s\n", license);
}

void usage(const char* progname){
	printf("Usage: %s (backup|restore|configure) [options]\n", progname);
	printf("Options:\n");
	printf("\t-c compressor\n");
	printf("\t-C checksum\n");
	printf("\t-d /dir1 /dir2 /...\n");
	printf("\t-e encryption\n");
	printf("\t-h, --help\n");
	printf("\t-o /out/file\n");
	printf("\t-v\n");
	printf("\t-x /dir1 /dir2 /...\n");
}

static int is_directory(const char* path){
	struct stat st;

	if (!path){
		return 0;
	}

	stat(path, &st);
	return S_ISDIR(st.st_mode);
}

static int remove_string(char*** entries, int* len, int index){
	int i;
	for (i = index; i < *len - 2; ++i){
		(*entries)[i] = (*entries)[i + 1];
	}
	(*len)--;
	(*entries) = realloc(*entries, *len * sizeof(*(*entries)));
	if (!(*entries) && (*len) != 0){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}
	return 0;
}

static int sanitize_directories(char*** entries, int* len){
	int n_removed = 0;
	int i;
	for (i = 0; i < (*len); ++i){
		if (!is_directory((*entries)[i])){
			remove_string(entries, len, i);
			i--;
			n_removed++;
		}
	}
	return n_removed;
}

static int add_string_to_array(char*** array, int* array_len, const char* str){
	(*array_len)++;

	*array = realloc(*array, *array_len * sizeof(*(*array)));
	if (!(*array)){
		log_fatal(__FL__, STR_ENOMEM);
		(*array_len)--;
		return -1;
	}

	(*array)[*array_len - 1] = malloc(strlen(str) + 1);
	if (!(*array)[*array_len - 1]){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}

	strcpy((*array)[*array_len - 1], str);
	return 0;
}

/* parses command line args
 *
 * returns -1 if out is NULL, 0 on success
 * index of bad argument on bad argument */
int parse_options_cmdline(int argc, char** argv, options* out){
	int i;

	if (!out){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	memset(out, 0, sizeof(*out));
	out->operation = OP_INVALID;

	for (i = 0; i < argc; ++i){
		if (!strcmp(argv[i], "--version")){
			version();
			exit(0);
		}
		if (!strcmp(argv[i], "-h") ||
				!strcmp(argv[i], "--help")){
			usage(argv[0]);
			exit(0);
		}
		/* compression */
		else if (!strcmp(argv[i], "-c")){
			/* check next argument */
			++i;
			out->comp_algorithm = get_compressor_byname(argv[i]);
		}
		/* checksum */
		else if (!strcmp(argv[i], "-C")){
			/* check next argument */
			++i;
			out->hash_algorithm = argv[i];
		}
		/* encryption */
		else if (!strcmp(argv[i], "-e")){
			/* next argument */
			++i;
			out->enc_algorithm = malloc(strlen(argv[i]) + 1);
			strcpy(out->enc_algorithm, argv[i]);
		}
		/* verbose */
		else if (!strcmp(argv[i], "-v")){
			out->flags |= FLAG_VERBOSE;
		}
		/* outfile */
		else if (!strcmp(argv[i], "-o")){
			/* next argument */
			++i;
			/* must be able to call free() w/o errors */
			out->file_out = malloc(strlen(argv[i]) + 1);
			strcpy(out->file_out, argv[i]);
		}
		/* exclude */
		else if (!strcmp(argv[i], "-x")){
			while (++i < argc && argv[i][0] != '-'){
				add_string_to_array(&(out->exclude), &(out->exclude_len), argv[i]);
			}
		}
		/* exclude */
		else if (!strcmp(argv[i], "-d")){
			while (++i < argc && argv[i][0] != '-'){
				add_string_to_array(&(out->directories), &(out->directories_len), argv[i]);
			}
		}
		/* directories */
		else if (argv[i][0] != '-'){
			if (!strcmp(argv[i], "backup")){
				out->operation = OP_BACKUP;
			}
			else if (!strcmp(argv[i], "restore")){
				out->operation = OP_RESTORE;
			}
			else if (!strcmp(argv[i], "configure")){
				out->operation = OP_CONFIGURE;
			}
			else{
				return i;
			}
		}
		else{
			return i;
		}
	}

	if (!out->directories){
		out->directories = malloc(sizeof(char*));
		out->directories[0] = malloc(sizeof("/"));
		strcpy(out->directories[0], "/");
		out->directories_len = 1;
	}
	return 0;
}

/* this function causes memory leaks by design.
 * this is ncurses' fault, not mine */
int display_menu(const char** options, int num_options, const char* title){
	ITEM** my_items;
	int c;
	MENU* my_menu;
	WINDOW* my_menu_window;
	WINDOW* my_menu_subwindow;
	ITEM* cur_item;
	int i;
	int row;
	int col;
	int ret;

	if (!options || !title || num_options <= 0){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	/* initialize curses */
	initscr();
	/* disable line buffering for stdin, so our output
	 * shows up immediately */
	cbreak();
	/* disable echoing of our characters */
	noecho();
	/* enable arrow keys */
	keypad(stdscr, TRUE);

	/* disable cursor blinking */
	curs_set(0);
	/* get coordinates of current terminal */
	getmaxyx(stdscr, col, row);

	/* create item list */
	my_items = malloc((num_options + 1) * sizeof(ITEM*));
	for (i = 0; i < num_options; ++i){
		my_items[i] = new_item(options[i], NULL);
	}
	my_items[num_options] = NULL;

	/* initialize menu and windows */
	my_menu = new_menu(my_items);
	my_menu_window = newwin(col - 4, row - 4, 2, 2);
	my_menu_subwindow = derwin(my_menu_window, col - 12, row - 12, 3, 3);
	keypad(my_menu_window, TRUE);

	/* attach menu to our subwindow */
	set_menu_win(my_menu, my_menu_window);
	set_menu_sub(my_menu, my_menu_subwindow);

	/* change selected menu item mark */
	set_menu_mark(my_menu, "> ");

	clear();
	/* put a box around window */
	box(my_menu_window, 0, 0);
	/* display our title */
	mvwprintw(my_menu_window, 1, (row - 6 - strlen(title)) / 2, title);
	/* put another around the title */
	mvwaddch(my_menu_window, 2, 0, ACS_LTEE);
	mvwhline(my_menu_window, 2, 1, ACS_HLINE, row - 6);
	mvwaddch(my_menu_window, 2, row - 5, ACS_RTEE);
	refresh();

	/* post the menu */
	post_menu(my_menu);
	wrefresh(my_menu_window);

	/* while the user does not press enter */
	while ((c = wgetch(my_menu_window)) != '\n'){
		/* handle menu input */
		switch(c){
		case KEY_DOWN:
			menu_driver(my_menu, REQ_DOWN_ITEM);
			break;
		case KEY_UP:
			menu_driver(my_menu, REQ_UP_ITEM);
			break;
		}
	}

	/* get chosen item index */
	cur_item = current_item(my_menu);
	ret = item_index(cur_item);

	/* cleanup */
	curs_set(1);
	unpost_menu(my_menu);
	free_menu(my_menu);
	for (i = 0; i < num_options; ++i){
		free_item(my_items[i]);
	}
	free(my_items);
	delwin(my_menu_subwindow);
	delwin(my_menu_window);
	endwin();

	return ret;
}

static int menu_compressor(options* opt){
	int res;
	const char* options_compressor[] = {
		"gzip  (default)",
		"bzip2 (higher compression, slower)",
		"xz    (highest compression, slowest)",
		"lz4   (fastest, lowest compression)",
		"none"
	};

	res = display_menu(options_compressor, ARRAY_SIZE(options_compressor), "Select a compression algorithm");

	switch (res){
	case 0:
		opt->comp_algorithm = COMPRESSOR_GZIP;
		break;
	case 1:
		opt->comp_algorithm = COMPRESSOR_BZIP2;
		break;
	case 2:
		opt->comp_algorithm = COMPRESSOR_XZ;
		break;
	case 3:
		opt->comp_algorithm = COMPRESSOR_LZ4;
		break;
	case 4:
		opt->comp_algorithm = COMPRESSOR_NONE;
		break;
	default:
		log_warning(__FL__, "Invalid compressor selected");
		opt->comp_algorithm = COMPRESSOR_INVALID;
		break;
	}

	return 0;
}

static int menu_checksum(options* opt){
	int res;
	const char* options_checksum[] = {
		"sha1   (default)",
		"sha256 (less collisions, slower)",
		"sha512 (lowest collisions, slowest)",
		"md5    (fastest, most collisions)",
		"none"
	};

	res = display_menu(options_checksum, ARRAY_SIZE(options_checksum), "Select a checksum algorithm");

	switch(res){
	case 0:
		opt->hash_algorithm = "sha1";
		break;
	case 1:
		opt->hash_algorithm = "sha256";
		break;
	case 2:
		opt->hash_algorithm = "sha512";
		break;
	case 3:
		opt->hash_algorithm = "md5";
		break;
	case 4:
		opt->hash_algorithm = NULL;
		break;
	default:
		log_warning(__FL__, "Invalid checksum algorithm selected");
		opt->hash_algorithm = NULL;
		break;
	}

	return 0;
}

static int menu_encryption(options* opt){
	int res_encryption = -1;
	int res_keysize = -1;
	int res_mode = -1;
	const char* options_encryption[] = {
		"AES (default)",
		"Camellia",
		"SEED",
		"Blowfish",
		"Triple DES (EDE)",
		"none"
	};
	const char* options_keysize[] = {
		"256 (default)",
		"192 (faster, less secure)",
		"128 (fastest, least secure)"
	};
	const char* options_mode[] = {
		"CBC (default)",
		"CFB",
		"OFB",
		"CTR",
	};

	free(opt->enc_algorithm);

	res_encryption = display_menu(options_encryption, ARRAY_SIZE(options_encryption), "Select an encryption algorithm");
	if (res_encryption <= 1){
		res_keysize = display_menu(options_keysize, ARRAY_SIZE(options_keysize), "Select a key size");
	}
	if (res_encryption <= 2){
		res_mode = display_menu(options_mode, ARRAY_SIZE(options_keysize), "Select an encryption mode");
	}
	else if (res_encryption <= 4){
		res_mode = display_menu(options_mode, ARRAY_SIZE(options_keysize) - 1, "Select an encryption mode");
	}

	opt->enc_algorithm = malloc(sizeof("camellia-256-cbc"));
	switch (res_encryption){
	case 0:
		strcpy(opt->enc_algorithm, "aes");
		break;
	case 1:
		strcpy(opt->enc_algorithm, "camellia");
		break;
	case 2:
		strcpy(opt->enc_algorithm, "seed");
		break;
	case 3:
		strcpy(opt->enc_algorithm, "bf");
		break;
	case 4:
		strcpy(opt->enc_algorithm, "des_ede3");
		break;
	case 5:
		free(opt->enc_algorithm);
		opt->enc_algorithm = NULL;
		break;
	default:
		log_warning(__FL__, "Invalid encryption algorithm specified");
		free(opt->enc_algorithm);
		opt->enc_algorithm = NULL;
		break;
	}

	switch(res_keysize){
	case 0:
		strcat(opt->enc_algorithm, "-256");
		break;
	case 1:
		strcat(opt->enc_algorithm, "-192");
		break;
	case 2:
		strcat(opt->enc_algorithm, "-128");
		break;
	}

	switch(res_mode){
	case 0:
		strcat(opt->enc_algorithm, "-cbc");
		break;
	case 1:
		strcat(opt->enc_algorithm, "-cfb");
		break;
	case 2:
		strcat(opt->enc_algorithm, "-ofb");
		break;
	case 3:
		strcat(opt->enc_algorithm, "-ctr");
		break;
	}

	return 0;
}

int menu_directories(options* opt){
	int res;
	int res_submenu;
	const char* options_initial[] = {
		"Add a directory",
		"Remove a directory",
		"Exit"
	};
	const char** options_submenu = NULL;
	const char* title = "Directories";

	/* TODO: refactor */
	do{
		res = display_menu(options_initial, ARRAY_SIZE(options_initial), title);

		switch (res){
			int i;
			char* str;
			int res;
		case 0:
			str = readline("Enter directory:");
			if (strcmp(str, "") != 0 && add_string_to_array(&(opt->directories), &(opt->directories_len), str) != 0){
				log_debug(__FL__, "Failed to add string to directories list");
				return -1;
			}
			free(str);
			if ((res = sanitize_directories(&(opt->directories), &(opt->directories_len))) > 0){
				title = "Directory specified was invalid";
			}
			else if (res < 0){
				log_warning(__FL__, "Failed to sanitize directory list");
			}
			else{
				title = "Directories";
			}
			break;
		case 1:
			options_submenu = malloc((opt->directories_len + 1) * sizeof(*options_submenu));
			if (!options_submenu){
				log_fatal(__FL__, STR_ENOMEM);
				return -1;
			}
			for (i = 0; i < opt->directories_len; ++i){
				options_submenu[i] = opt->directories[i];
			}
			options_submenu[opt->directories_len] = "Exit";

			res_submenu = display_menu(options_submenu, opt->directories_len + 1, "Choose a directory");
			if (res_submenu == opt->directories_len){
				free(options_submenu);
				break;
			}
			else if (res_submenu < opt->directories_len){
				if (remove_string(&(opt->directories), &(opt->directories_len), res_submenu) != 0){
					log_debug(__FL__, "Failed to remove_directory()");
					return -1;
				}
			}
			free(options_submenu);
			break;
		case 2:
			return 0;
		default:
			log_warning(__FL__, "Invalid option selected");
			return 0;
		}
	}while (res != 2);
	return 0;
}

int menu_exclude(options* opt){
	int res;
	int res_submenu;
	const char* options_initial[] = {
		"Add an exclude path",
		"Remove an exclude path",
		"Exit"
	};
	const char** options_submenu = NULL;
	const char* title = "Exclude paths";

	/* TODO: refactor */
	do{
		res = display_menu(options_initial, ARRAY_SIZE(options_initial), title);

		switch (res){
			int i;
			char* str;
			int res;
		case 0:
			str = opt->exclude[opt->exclude_len - 1] = readline("Enter exclude path:");
			if (strcmp(str, "") != 0 && add_string_to_array(&(opt->exclude), &(opt->exclude_len), str) != 0){
				log_debug(__FL__, "Failed to add string to exclude list");
				return -1;
			}
			free(str);
			if ((res = sanitize_directories(&(opt->exclude), &(opt->exclude_len))) > 0){
				title = "Exclude path specified was invalid";
			}
			else if (res < 0){
				log_warning(__FL__, "Failed to sanitize exclude list");
			}
			else{
				title = "Exclude paths";
			}
			break;
		case 1:
			options_submenu = malloc((opt->exclude_len + 1) * sizeof(*options_submenu));
			if (!options_submenu){
				log_fatal(__FL__, STR_ENOMEM);
				return -1;
			}
			for (i = 0; i < opt->exclude_len; ++i){
				options_submenu[i] = opt->exclude[i];
			}
			options_submenu[opt->exclude_len] = "Exit";

			res_submenu = display_menu(options_submenu, opt->exclude_len + 1, "Choose a directory");
			if (res_submenu == opt->exclude_len){
				free(options_submenu);
				break;
			}
			else if (res_submenu < opt->exclude_len){
				if (remove_string(&(opt->exclude), &(opt->exclude_len), res_submenu) != 0){
					log_debug(__FL__, "Failed to remove_directory()");
					return -1;
				}
			}
			free(options_submenu);
			break;
		case 2:
			return 0;
		default:
			log_warning(__FL__, "Invalid option selected");
			return 0;
		}
	}while (res != 2);
	return 0;
}

int parse_options_menu(options* opt){
	int res;
	const char* options_main_menu[] = {
		"Compression",
		"Checksums",
		"Encryption",
		"Directories",
		"Exclude paths",
		"Exit"
	};

	do{
		res = display_menu(options_main_menu, ARRAY_SIZE(options_main_menu), "Configure");
		switch (res){
		case 0:
			menu_compressor(opt);
			break;
		case 1:
			menu_checksum(opt);
			break;
		case 2:
			menu_encryption(opt);
			break;
		case 3:
			menu_directories(opt);
			break;
		case 4:
			menu_exclude(opt);
			break;
		case 5:
			break;
		default:
			log_warning(__FL__, "Invalid choice");
			return 0;

		}
	}while (res != 5);
	return 0;
}

int get_default_options(options* opt){
	opt->comp_algorithm = COMPRESSOR_GZIP;
	opt->hash_algorithm = "sha1";
	opt->enc_algorithm = malloc(sizeof("aes-256-cbc"));
	if (!opt->enc_algorithm){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}
	strcpy(opt->enc_algorithm, "aes-256-cbc");
	opt->prev_backup = NULL;
	opt->operation = OP_INVALID;
	opt->directories = malloc(sizeof(*(opt->directories)));
	if (!opt->directories){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}
	opt->directories[0] = malloc(sizeof("/"));
	if (!opt->directories[0]){
		free(opt->directories);
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}
	strcpy(opt->directories[0], "/");
	opt->directories_len = 1;
	opt->exclude = NULL;
	opt->exclude_len = 0;
	opt->flags = FLAG_VERBOSE;
	return 0;
}

static char* read_file_string(FILE* in){
	int c;
	char* ret = NULL;
	int ret_len = 0;

	while ((c = fgetc(in)) != '\0'){
		if (c == EOF){
			log_debug(__FL__, "Reached EOF");
			free(ret);
			return NULL;
		}
		ret_len++;
		ret = realloc(ret, ret_len);
		if (!ret){
			log_fatal(__FL__, STR_ENOMEM);
			return NULL;
		}
		ret[ret_len - 1] = c;
	}
	ret_len++;
	ret = realloc(ret, ret_len);
	ret[ret_len - 1] = '\0';

	return ret;
}

int parse_options_fromfile(const char* file, options* opt){
	FILE* fp;
	char* tmp;

	fp = fopen(file, "rb");
	if (!fp){
		log_error(__FL__, STR_EFOPEN, file, strerror(errno));
		return -1;
	}

	memset(opt, 0, sizeof(*opt));

	fscanf(fp, "[Options]");
	fscanf(fp, "\nPREV=");
	opt->prev_backup = read_file_string(fp);
	if (strcmp(opt->prev_backup, "none") == 0){
		free(opt->prev_backup);
		opt->prev_backup = NULL;
	}
	fscanf(fp, "\nDIRECTORIES=");
	do{
		tmp = read_file_string(fp);
		if (!tmp){
			break;
		}
		if (tmp[0] != '\0'){
			opt->directories_len++;
			opt->directories = realloc(opt->directories, sizeof(char*) * opt->directories_len);
			opt->directories[opt->directories_len - 1] = tmp;
		}
		else{
			free(tmp);
			tmp = NULL;
		}
	}while (tmp && tmp[0] != '\0');
	fscanf(fp, "\nEXCLUDE=");
	do{
		tmp = read_file_string(fp);
		if (!tmp){
			break;
		}
		if (tmp[0] != '\0'){
			opt->exclude_len++;
			opt->exclude = realloc(opt->exclude, sizeof(char*) * opt->exclude_len);
			opt->exclude[opt->exclude_len - 1] = tmp;
		}
		else{
			free(tmp);
			tmp = NULL;
		}
	}while (tmp && tmp[0] != '\0');
	fscanf(fp, "\nCHECKSUM=");
	opt->hash_algorithm = read_file_string(fp);
	fscanf(fp, "\nENCRYPTION=");
	opt->enc_algorithm = read_file_string(fp);
	fscanf(fp, "\nCOMPRESSION=%d", (int*)&(opt->comp_algorithm));
	fscanf(fp, "\nFLAGS=%u", &(opt->flags));

	if (fclose(fp) != 0){
		log_error(__FL__, STR_EFCLOSE, file);
	}
	return 0;
}

/* File format:
 *
 * [Options]
 * PREV=/path/to/prev.tar\0
 * DIRECTORIES=/dir1\0/dir2\0/dir3\0\0
 * EXCLUDE=/exc1\0/exc2\0/exc3\0\0
 * CHECKSUM=sha1\0
 * ENCRYPTION=aes-256-cbc\0
 * COMPRESSION=2
 * FLAGS=1
 *
 */
int write_options_tofile(const char* file, options* opt){
	FILE* fp;
	int i;

	if (!file || !opt){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	fp = fopen(file, "wb");
	if (!fp){
		log_error(__FL__, STR_EFOPEN, file, strerror(errno));
		return -1;
	}
	fprintf(fp, "[Options]");
	fprintf(fp, "\nPREV=%s%c", opt->prev_backup ? opt->prev_backup : "none", '\0');
	fprintf(fp, "\nDIRECTORIES=");
	for (i = 0; i < opt->directories_len; ++i){
		fprintf(fp, "%s%c", opt->directories[i], '\0');
	}
	fputc('\0', fp);
	fprintf(fp, "\nEXCLUDE=");
	for (i = 0; i < opt->exclude_len; ++i){
		fprintf(fp, "%s%c", opt->exclude[i], '\0');
	}
	fputc('\0', fp);
	fprintf(fp, "\nCHECKSUM=%s%c", opt->hash_algorithm, '\0');
	fprintf(fp, "\nENCRYPTION=%s%c", opt->enc_algorithm, '\0');
	fprintf(fp, "\nCOMPRESSION=%d", opt->comp_algorithm);
	fprintf(fp, "\nFLAGS=%u", opt->flags);

	if (fclose(fp) != 0){
		log_error(__FL__, STR_EFCLOSE, file);
	}
	return 0;
}

void free_options(options* opt){
	int i;
	/* freeing a nullptr is ok */
	if (opt->prev_backup != opt->file_out){
		free(opt->prev_backup);
	}
	free(opt->enc_algorithm);
	free(opt->file_out);
	for (i = 0; i < opt->exclude_len; ++i){
		free(opt->exclude[i]);
	}
	free(opt->exclude);
	for (i = 0; i < opt->directories_len; ++i){
		free(opt->directories[i]);
	}
	free(opt->directories);
}
