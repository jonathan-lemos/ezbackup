/* prototypes */
#include "maketar.h"
/* read_file() */
#include "readfile.h"
/* errors */
#include "evperror.h"
/* progressbar */
#include "progressbar.h"
/* working with tar */
#include <archive.h>
#include <archive_entry.h>
/* get file attributes */
#include <sys/stat.h>
/* FILE* */
#include <stdio.h>
/* for getting gname from gid */
#include <grp.h>
/* for getting uname from uid */
#include <pwd.h>
/* strcmp */
#include <string.h>
/* malloc */
#include <stdlib.h>

int print_error(int err, TAR* tp){
	switch(err){
		case ARCHIVE_OK:
		case ARCHIVE_EOF:
			fprintf(stderr, "maketar reached eof\n");
			break;
		case ARCHIVE_WARN:
			fprintf(stderr, "maketar warning: %s\n", archive_error_string(tp));
			break;
		case ARCHIVE_FAILED:
			fprintf(stderr, "maketar error: %s\n", archive_error_string(tp));
			break;
		case ARCHIVE_FATAL:
			fprintf(stderr, "maketar fatal error: %s\n", archive_error_string(tp));
			break;
		default:
			fprintf(stderr, "maketar unknown: %s\n", archive_error_string(tp));
	}
	return err;
}

TAR* tar_create(const char* filename, COMPRESSOR comp){
	TAR* tp = archive_write_new();

	if (!filename){
		return NULL;
	}

	switch(comp){
		case COMPRESSOR_LZ4:
			archive_write_add_filter_lz4(tp);
			break;
		case COMPRESSOR_GZIP:
			archive_write_add_filter_gzip(tp);
			break;
		case COMPRESSOR_BZIP2:
			archive_write_add_filter_bzip2(tp);
			break;
		case COMPRESSOR_XZ:
			archive_write_add_filter_xz(tp);
			break;
		default:
			archive_write_add_filter_none(tp);
	}

	archive_write_set_format_pax_restricted(tp);
	archive_write_open_filename(tp, filename);

	return tp;
}

int tar_add_file(TAR* tp, const char* filename){
	/* file stats */
	struct stat st;
	/* tar header */
	struct archive_entry* entry;
	/* gid -> gname */
	struct group* grp;
	/* uid -> uname */
	struct passwd* pwd;
	/* reading the file */
	FILE* fp = NULL;
	int len = 0;
	unsigned char buffer[BUFFER_LEN];

	if (!tp){
		return ERR_ARGUMENT_NULL;
	}

	/* must open as "rb" to prevent \r\n -> \n */
	fp = fopen(filename, "rb");
	if (!fp){
		return 1;
	}

	lstat(filename, &st);
	entry = archive_entry_new();

	/* write file metadata */
	archive_entry_set_pathname(entry, filename);
	archive_entry_set_size(entry, st.st_size);
	archive_entry_set_atime(entry, st.st_atime, 0);
	archive_entry_set_ctime(entry, st.st_ctime, 0);
	archive_entry_set_mtime(entry, st.st_mtime, 0);
	archive_entry_set_filetype(entry, st.st_mode & AE_IFMT);
	archive_entry_set_gid(entry, st.st_gid);
	grp = getgrgid(st.st_gid);
	if (grp){
		archive_entry_set_gname(entry, grp->gr_name);
	}
	archive_entry_set_perm(entry, st.st_mode & 01777);
	archive_entry_set_size(entry, st.st_size);
	archive_entry_set_uid(entry, st.st_uid);
	pwd = getpwuid(st.st_uid);
	if (pwd){
		archive_entry_set_uname(entry, pwd->pw_name);
	}

	archive_write_header(tp, entry);

	/* writes BUFFER_LEN bytes at a time to the tar */
	/* this means we can handle huge files without running out of RAM */
	while((len = read_file(fp, buffer, sizeof(buffer))) > 0){
		archive_write_data(tp, buffer, len);
	}
	fclose(fp);

	archive_entry_free(entry);
	return 0;
}

int tar_add_file_ex(TAR* tp, const char* filename, const char* path_in_tar, int verbose, const char* progress_msg){
	/* file stats */
	struct stat st;
	/* tar header */
	struct archive_entry* entry;
	/* gid -> gname */
	struct group* grp;
	/* uid -> uname */
	struct passwd* pwd;
	/* reading the file */
	FILE* fp = NULL;
	int len = 0;
	unsigned char buffer[BUFFER_LEN];
	/* verbose */
	progress* p;

	if (!tp){
		return ERR_ARGUMENT_NULL;
	}

	/* must open as "rb" to prevent \r\n -> \n */
	fp = fopen(filename, "rb");
	if (!fp){
		return 1;
	}

	stat(filename, &st);

	entry = archive_entry_new();

	/* write file metadata */
	archive_entry_set_pathname(entry, path_in_tar);
	archive_entry_set_size(entry, st.st_size);
	archive_entry_set_atime(entry, st.st_atime, 0);
	archive_entry_set_ctime(entry, st.st_ctime, 0);
	archive_entry_set_mtime(entry, st.st_mtime, 0);
	archive_entry_set_filetype(entry, st.st_mode & AE_IFMT);
	archive_entry_set_gid(entry, st.st_gid);
	grp = getgrgid(st.st_gid);
	if (grp){
		archive_entry_set_gname(entry, grp->gr_name);
	}
	archive_entry_set_perm(entry, st.st_mode & 01777);
	archive_entry_set_size(entry, st.st_size);
	archive_entry_set_uid(entry, st.st_uid);
	pwd = getpwuid(st.st_uid);
	if (pwd){
		archive_entry_set_uname(entry, pwd->pw_name);
	}

	archive_write_header(tp, entry);

	/* no progress bar if it will be 100% instantly */
	if (st.st_size <= BUFFER_LEN){
		verbose = 0;
		printf("%s\n", progress_msg);
	}
	if (verbose){
		p = start_progress(progress_msg, st.st_size);
	}

	/* writes BUFFER_LEN bytes at a time to the tar */
	/* this means we can handle huge files without running out of RAM */
	while((len = read_file(fp, buffer, sizeof(buffer))) > 0){
		if (verbose){
			inc_progress(p, len);
		}

		archive_write_data(tp, buffer, len);
	}
	fclose(fp);
	if (verbose){
		finish_progress(p);
	}

	archive_entry_free(entry);
	return 0;
}

int tar_close(TAR* tp){
	if (!tp){
		return ERR_ARGUMENT_NULL;
	}

	archive_write_close(tp);
	return archive_write_free(tp);
}

static int strcmp_nocase(const char* str1, const char* str2){
	char* c1 = malloc(strlen(str1) + 1);
	char* c2 = malloc(strlen(str2) + 1);
	unsigned i;
	int ret;

	if (!c1 || !c2){
		fprintf(stderr, "something is horribly wrong with your malloc(), fam\n");
		exit(1);
	}

	strcpy(c1, str1);
	strcpy(c2, str2);

	for (i = 0; i < strlen(str1); ++i){
		if (c1[i] >= 'A' && c1[i] <= 'Z'){
			c1[i] += 'a' - 'A';
		}
	}

	for (i = 0; i < strlen(str2); ++i){
		if (c2[i] >= 'A' && c2[i] <= 'Z'){
			c2[i] += 'a' - 'A';
		}
	}

	ret = strcmp(c1, c2);
	free(c1);
	free(c2);

	return ret;
}

static int copy_data(TAR* in, TAR* out){
	int ret = ARCHIVE_OK;
	const void* buf;
	size_t size;
	la_int64_t offset;

	while (ret != ARCHIVE_EOF){
		ret = archive_read_data_block(in, &buf, &size, &offset);
		switch (ret){
			case ARCHIVE_OK:
				break;
			case ARCHIVE_EOF:
				continue;
				break;
			case ARCHIVE_WARN:
				print_error(ret, in);
				break;
			default:
				print_error(ret, in);
				return ret;
		}

		ret = archive_write_data_block(out, buf, size, offset);
		switch (ret){
			case ARCHIVE_OK:
				break;
			case ARCHIVE_WARN:
				print_error(ret, out);
				break;
			default:
				print_error(ret, out);
				return ret;
		}
	}
	return ARCHIVE_OK;
}

int tar_extract(const char* tarchive, const char* outdir){
	TAR* tp;
	TAR* ext;
	struct archive_entry* entry;
	int flags = ARCHIVE_EXTRACT_TIME |
		ARCHIVE_EXTRACT_PERM |
		ARCHIVE_EXTRACT_ACL |
		ARCHIVE_EXTRACT_FFLAGS |
		ARCHIVE_EXTRACT_OWNER;
	int ret;

	/* open tar for reading */
	tp = archive_read_new();
	archive_read_support_filter_all(tp);
	archive_read_support_format_tar(tp);
	/* open disk for writing */
	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	archive_write_disk_set_standard_lookup(ext);

	/* set input filename */
	ret = archive_read_open_filename(tp, tarchive, BUFFER_LEN);
	switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_EOF:
			return ERR_FILE_INPUT;
			break;
		case ARCHIVE_WARN:
			print_error(ret, tp);
			break;
		default:
			print_error(ret, tp);
			archive_read_close(tp);
			archive_read_free(tp);
			archive_write_close(ext);
			archive_write_free(ext);
			return ret;
	}

	/* while there are files in the archive */
	while (ret != ARCHIVE_EOF){
		const char* tar_file_path;
		char* out_path;

		/* read the header */
		ret = archive_read_next_header(tp, &entry);
		switch (ret){
			case ARCHIVE_OK:
				break;
			case ARCHIVE_EOF:
				return 0;
				break;
			case ARCHIVE_WARN:
				print_error(ret, tp);
				break;
			case ARCHIVE_FAILED:
				print_error(ret, tp);
				continue;
				break;
			default:
				print_error(ret, tp);
				archive_entry_free(entry);
				archive_read_close(tp);
				archive_read_free(tp);
				archive_write_close(ext);
				archive_write_free(ext);
				return ret;
		}

		tar_file_path = archive_entry_pathname(entry);
		out_path = malloc(strlen(tar_file_path) + strlen(outdir) + 1);
		if (!out_path){
			fprintf(stderr, "out of memory at the jack in the box\n");
			return -1;
		}
		strcpy(out_path, outdir);
		strcat(out_path, tar_file_path);
		archive_entry_set_pathname(entry, out_path);

		/* set the output directory */
		ret = archive_write_header(ext, entry);
		switch (ret){
			case ARCHIVE_OK:
				break;
			case ARCHIVE_WARN:
				print_error(ret, tp);
				break;
			case ARCHIVE_FAILED:
				print_error(ret, tp);
				free(out_path);
				continue;
				break;
			default:
				print_error(ret, tp);
				free(out_path);
				archive_entry_free(entry);
				archive_read_close(tp);
				archive_read_free(tp);
				archive_write_close(ext);
				archive_write_free(ext);
				return ret;
		}

		/* extract the file */
		ret = copy_data(tp, ext);
		switch (ret){
			case ARCHIVE_OK:
				break;
			case ARCHIVE_WARN:
				print_error(ret, tp);
				break;
			case ARCHIVE_FAILED:
				print_error(ret, tp);
				free(out_path);
				continue;
				break;
			default:
				print_error(ret, tp);
				free(out_path);
				archive_entry_free(entry);
				archive_read_close(tp);
				archive_read_free(tp);
				archive_write_close(ext);
				archive_write_free(ext);
				return ret;
		}

		free(out_path);
	}

	/* cleanup */
	ret = archive_write_finish_entry(ext);
	switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_WARN:
			print_error(ret, tp);
			break;
		default:
			print_error(ret, tp);
			archive_read_close(tp);
			archive_read_free(tp);
			archive_write_close(ext);
			archive_write_free(ext);
			break;
	}

	archive_read_close(tp);
	archive_read_free(tp);
	archive_write_close(ext);
	archive_write_free(ext);
	return 0;
}

int tar_extract_file(const char* tarchive, const char* file_intar, const char* file_out){
	TAR* tp;
	TAR* ext;
	struct archive_entry* entry;
	int flags = ARCHIVE_EXTRACT_TIME |
		ARCHIVE_EXTRACT_PERM |
		ARCHIVE_EXTRACT_ACL |
		ARCHIVE_EXTRACT_FFLAGS |
		ARCHIVE_EXTRACT_OWNER;
	int ret;

	/* open tar for reading */
	tp = archive_read_new();
	archive_read_support_filter_all(tp);
	archive_read_support_format_tar(tp);
	/* open disk for writing */
	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	archive_write_disk_set_standard_lookup(ext);

	/* set input filename */
	ret = archive_read_open_filename(tp, tarchive, BUFFER_LEN);
	switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_EOF:
			return ERR_FILE_INPUT;
			break;
		case ARCHIVE_WARN:
			print_error(ret, tp);
			break;
		default:
			print_error(ret, tp);
			archive_read_close(tp);
			archive_read_free(tp);
			archive_write_close(ext);
			archive_write_free(ext);
			return ret;
	}

	/* while there are files in the archive */
	while (ret != ARCHIVE_EOF){
		/* read the header */
		ret = archive_read_next_header(tp, &entry);
		switch (ret){
			case ARCHIVE_OK:
				break;
			case ARCHIVE_EOF:
				return ARCHIVE_EOF;
				break;
			case ARCHIVE_WARN:
				print_error(ret, tp);
				break;
			case ARCHIVE_FAILED:
				print_error(ret, tp);
				continue;
				break;
			default:
				print_error(ret, tp);
				archive_entry_free(entry);
				archive_read_close(tp);
				archive_read_free(tp);
				archive_write_close(ext);
				archive_write_free(ext);
				return ret;
		}

		/* check if the thing matches our file */
		if (strcmp(archive_entry_pathname(entry), file_intar) == 0){
			/* set the output directory */
			archive_entry_set_pathname(entry, file_out);
			ret = archive_write_header(ext, entry);
			switch (ret){
				case ARCHIVE_OK:
					break;
				case ARCHIVE_WARN:
					print_error(ret, tp);
					break;
				case ARCHIVE_FAILED:
					print_error(ret, tp);
					continue;
					break;
				default:
					print_error(ret, tp);
					archive_entry_free(entry);
					archive_read_close(tp);
					archive_read_free(tp);
					archive_write_close(ext);
					archive_write_free(ext);
					return ret;
			}

			/* extract the file */
			ret = copy_data(tp, ext);
			switch (ret){
				case ARCHIVE_OK:
					break;
				case ARCHIVE_WARN:
					print_error(ret, tp);
					break;
				case ARCHIVE_FAILED:
					print_error(ret, tp);
					continue;
					break;
				default:
					print_error(ret, tp);
					archive_entry_free(entry);
					archive_read_close(tp);
					archive_read_free(tp);
					archive_write_close(ext);
					archive_write_free(ext);
			}
			break;
		}
	}

	/* cleanup */
	ret = archive_write_finish_entry(ext);
	switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_WARN:
			print_error(ret, tp);
			break;
		default:
			print_error(ret, tp);
			archive_read_close(tp);
			archive_read_free(tp);
			archive_write_close(ext);
			archive_write_free(ext);
			break;
	}

	archive_read_close(tp);
	archive_read_free(tp);
	archive_write_close(ext);
	archive_write_free(ext);
	return 0;
}

COMPRESSOR get_compressor_byname(const char* compressor){
	if (!strcmp_nocase(compressor, "none") ||
			!strcmp_nocase(compressor, "off")){
		return COMPRESSOR_NONE;
	}
	if (!strcmp_nocase(compressor, "gzip") ||
			!strcmp_nocase(compressor, "gz")){
		return COMPRESSOR_GZIP;
	}
	else if (!strcmp_nocase(compressor, "bzip2") ||
			!strcmp_nocase(compressor, "bz2")){
		return COMPRESSOR_BZIP2;
	}
	else if (!strcmp_nocase(compressor, "xz")){
		return COMPRESSOR_XZ;
	}
	else if (!strcmp_nocase(compressor, "lz4")){
		return COMPRESSOR_LZ4;
	}
	else{
		return COMPRESSOR_INVALID;
	}
}
