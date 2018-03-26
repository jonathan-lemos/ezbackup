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
/* char* to EVP_MD(*)(void) */
#include <openssl/evp.h>
/* command-line tab completion */
#if defined(__linux__)
#include <editline/readline.h>
#elif defined(__APPLE__)
#include <readline/readline.h>
#else
#error "This operating system is not supported"
#endif
/* is_directory() */
#include <sys/stat.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void version(void){
	const char* program_name = "ezbackup";
	const char* version      = "0.2 beta";
	const char* year         = "2018";
	const char* name         = "Jonathan Lemos";
	const char* license      = "This software may be modified and distributed under the terms of the MIT license.\n\
								See the LICENSE file for details.";


	printf("%s %s\n", program_name, version);
	printf("Copyright (c) %s %s\n", year, name);
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
	for (i = index; i < *len - 1; ++i){
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
			OpenSSL_add_all_algorithms();
			out->hash_algorithm = EVP_get_digestbyname(argv[i]);
		}
		/* encryption */
		else if (!strcmp(argv[i], "-e")){
			/* next argument */
			++i;
			OpenSSL_add_all_algorithms();
			out->enc_algorithm = EVP_get_cipherbyname(argv[i]);
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
			cur_item = current_item(my_menu);
			ret = item_index(cur_item);
			if (ret >= num_options - 1){
				menu_driver(my_menu, REQ_FIRST_ITEM);
			}
			else{
				menu_driver(my_menu, REQ_DOWN_ITEM);
			}
			break;
		case KEY_UP:
			cur_item = current_item(my_menu);
			ret = item_index(cur_item);
			if (ret <= 0){
				menu_driver(my_menu, REQ_LAST_ITEM);
			}
			else{
				menu_driver(my_menu, REQ_UP_ITEM);
			}
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

static int menu_compression_level(options* opt){
	int res;
	const char* options_compression_level[] = {
		"Default",
		"1 (fastest, lowest compression)",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9 (slowest, highest compression)"
	};

	res = display_menu(options_compression_level, ARRAY_SIZE(options_compression_level), "Select a compression level");
	opt->comp_level = res;
	return 0;
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
	COMPRESSOR list_compressor[] = {
		COMPRESSOR_GZIP,
		COMPRESSOR_BZIP2,
		COMPRESSOR_XZ,
		COMPRESSOR_LZ4,
		COMPRESSOR_NONE
	};

	res = display_menu(options_compressor, ARRAY_SIZE(options_compressor), "Select a compression algorithm");
	if (opt->comp_algorithm != list_compressor[res]){
		opt->prev_backup = NULL;
	}
	opt->comp_algorithm = list_compressor[res];
	return menu_compression_level(opt);
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
	const EVP_MD*(*list_checksum[])(void) = {
		EVP_sha1,
		EVP_sha256,
		EVP_sha512,
		EVP_md5,
		NULL
	};

	res = display_menu(options_checksum, ARRAY_SIZE(options_checksum), "Select a checksum algorithm");
	opt->hash_algorithm = list_checksum[res] ? (list_checksum[res])() : NULL;
	return 0;
}

static int menu_encryption(options* opt){
	int res_encryption = -1;
	int res_keysize = -1;
	int res_mode = -1;
	char* tmp = NULL;
	const char* options_encryption[] = {
		"AES (default)",
		"Camellia",
		"SEED",
		"Blowfish",
		"Triple DES (EDE3)",
		"none"
	};
	const char* list_encryption[] = {
		"aes",
		"camellia",
		"seed",
		"bf",
		"des-ede3",
		NULL
	};
	const char* options_keysize[] = {
		"256 (default)",
		"192 (faster, less secure)",
		"128 (fastest, least secure)"
	};
	const char* list_keysize[] = {
		"-256",
		"-192",
		"-128"
	};
	const char* options_mode[] = {
		"Cipher Block Chaining (CBC) (default)",
		"Cipher Feedback (CFB)",
		"Output Feedback (OFB)",
		"Counter (CTR)",
	};
	const char* list_mode[] = {
		"-cbc",
		"-cfb",
		"-ofb",
		"-ctr"
	};

	res_encryption = display_menu(options_encryption, ARRAY_SIZE(options_encryption), "Select an encryption algorithm");
	if (res_encryption <= 1){
		res_keysize = display_menu(options_keysize, ARRAY_SIZE(options_keysize), "Select a key size");
	}
	if (res_encryption <= 2){
		res_mode = display_menu(options_mode, ARRAY_SIZE(options_mode), "Select an encryption mode");
	}
	else if (res_encryption <= 4){
		res_mode = display_menu(options_mode, ARRAY_SIZE(options_mode) - 1, "Select an encryption mode");
	}

	if (res_encryption < 0 || res_encryption == 5){
		return 0;
	}
	tmp = malloc(sizeof("camellia-256-cbc"));
	strcpy(tmp, list_encryption[res_encryption]);
	if (res_keysize > 0){
		strcat(tmp, list_keysize[res_keysize]);
	}
	if (res_mode > 0){
		strcat(tmp, list_mode[res_keysize]);
	}
	opt->enc_algorithm = EVP_get_cipherbyname(tmp);
	free(tmp);

	return 0;
}

int menu_directories(options* opt){
	int res;
	const char** options_initial = NULL;
	const char* title = "Directories";

	/* TODO: refactor */
	do{
		int i;

		options_initial = malloc((opt->directories_len + 2) * sizeof(*(opt->directories)));
		if (!options_initial){
			log_fatal(__FL__, STR_ENOMEM);
			return -1;
		}
		options_initial[0] = "Add a directory";
		options_initial[1] = "Exit";
		for (i = 2; i < opt->directories_len + 2; ++i){
			options_initial[i] = opt->directories[i - 2];
		}

		res = display_menu(options_initial, opt->directories_len + 2, title);

		switch (res){
			char* str;
			int res_confirm;
			char** options_confirm;
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
			break;
		default:
			options_confirm = malloc(2 * sizeof(*options_confirm));
			if (!options_confirm){
				log_fatal(__FL__, STR_ENOMEM);
				return -1;
			}
			options_confirm[0] = malloc(sizeof("Remove ") + strlen(opt->directories[res - 2]));
			if (!options_confirm){
				log_fatal(__FL__, STR_ENOMEM);
				return -1;
			}
			strcpy(options_confirm[0], "Remove ");
			strcat(options_confirm[0], opt->directories[res - 2]);
			options_confirm[1] = "Exit";
			res_confirm = display_menu((const char**)options_confirm, 2, "Removing directory");

			if (res_confirm == 0){
				if (remove_string(&(opt->directories), &(opt->directories_len), res - 2) != 0){
					log_debug(__FL__, "Failed to remove_directory()");
					return -1;
				}
			}

			free(options_confirm[0]);
			free(options_confirm);
			break;
		}
	}while (res != 1);
	return 0;
}

int menu_exclude(options* opt){
	int res;
	const char** options_initial = NULL;
	const char* title = "Exclude Paths";

	/* TODO: refactor */
	do{
		int i;

		options_initial = malloc((opt->exclude_len + 2) * sizeof(*(opt->exclude)));
		if (!options_initial){
			log_fatal(__FL__, STR_ENOMEM);
			return -1;
		}
		options_initial[0] = "Add an exclude path";
		options_initial[1] = "Exit";
		for (i = 2; i < opt->exclude_len + 2; ++i){
			options_initial[i] = opt->exclude[i - 2];
		}

		res = display_menu(options_initial, opt->exclude_len + 2, title);

		switch (res){
			char* str;
			int res_confirm;
			char** options_confirm;
		case 0:
			str = readline("Enter exclude path:");
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
			break;
		default:
			options_confirm = malloc(2 * sizeof(*options_confirm));
			if (!options_confirm){
				log_fatal(__FL__, STR_ENOMEM);
				return -1;
			}
			options_confirm[0] = malloc(sizeof("Remove ") + strlen(opt->exclude[res - 2]));
			if (!options_confirm){
				log_fatal(__FL__, STR_ENOMEM);
				return -1;
			}
			strcpy(options_confirm[0], "Remove ");
			strcat(options_confirm[0], opt->exclude[res - 2]);
			options_confirm[1] = "Exit";
			res_confirm = display_menu((const char**)options_confirm, 2, "Removing exclude path");

			if (res_confirm == 0){
				if (remove_string(&(opt->exclude), &(opt->exclude_len), res - 2) != 0){
					log_debug(__FL__, "Failed to remove_string()");
					return -1;
				}
			}

			free(options_confirm[0]);
			free(options_confirm);
			break;
		}
	}while (res != 1);
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
	opt->hash_algorithm = EVP_sha1();
	opt->enc_algorithm = EVP_aes_256_cbc();
	opt->prev_backup = NULL;
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
	opt->operation = OP_INVALID;
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
	tmp = read_file_string(fp);
	if (!tmp){
		opt->hash_algorithm = NULL;
	}
	else{
		opt->hash_algorithm = EVP_get_digestbyname(tmp);
		free(tmp);
	}
	fscanf(fp, "\nENCRYPTION=");
	tmp = read_file_string(fp);
	if (!tmp){
		opt->enc_algorithm = NULL;
	}
	else{
		opt->enc_algorithm = EVP_get_cipherbyname(tmp);
		free(tmp);
	}
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
	fprintf(fp, "\nCHECKSUM=%s%c", EVP_MD_name(opt->hash_algorithm), '\0');
	fprintf(fp, "\nENCRYPTION=%s%c", EVP_CIPHER_name(opt->enc_algorithm), '\0');
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
