/* maketar.c -- tar extractor/writer with compression support
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/* prototypes */
#include "maketar.h"
/* read_file() */
#include "readfile.h"
/* progressbar stuff */
#include "progressbar.h"
/* errors */
#include "error.h"
#include <errno.h>
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

TAR* tar_create(const char* filename, COMPRESSOR comp){
	TAR* tp = archive_write_new();

	if (!filename){
		log_error(__FL__, STR_ENULL);
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

int tar_add_fp_ex(TAR* tp, FILE* fp, const char* path_in_tar, int verbose, const char* progress_msg){
	/* file stats */
	struct stat st;
	/* tar header */
	struct archive_entry* entry;
	/* gid -> gname */
	struct group* grp;
	/* uid -> uname */
	struct passwd* pwd;
	/* reading the file */
	int fd = -1;
	int len = 0;
	unsigned char buffer[BUFFER_LEN];
	/* verbose */
	progress* p;

	if (!tp || !fp || !path_in_tar){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	rewind(fp);

	fd = fileno(fp);
	fstat(fd, &st);

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
	if (verbose && st.st_size <= BUFFER_LEN){
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

	if (verbose){
		finish_progress(p);
	}

	archive_entry_free(entry);
	return 0;
}

int tar_add_file_ex(TAR* tp, const char* filename, const char* path_in_tar, int verbose, const char* progress_msg){
	FILE* fp;
	int err;

	fp = fopen(filename, "rb");
	if (!fp){
		log_error(__FL__, STR_EFOPEN, filename, strerror(errno));
		return -1;
	}

	if ((err = tar_add_fp_ex(tp, fp, path_in_tar, verbose, progress_msg)) != 0){
		return err;
	}

	if (fclose(fp) != 0){
		log_error(__FL__, STR_EFCLOSE, filename);
		return -1;
	}
	return 0;
}

int tar_add_file(TAR* tp, const char* filename){
	return tar_add_file_ex(tp, filename, filename, 0, NULL);
}

int tar_close(TAR* tp){
	if (!tp){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	archive_write_close(tp);
	archive_write_free(tp);

	return 0;
}

static int strcmp_nocase(const char* str1, const char* str2){
	char* c1 = malloc(strlen(str1) + 1);
	char* c2 = malloc(strlen(str2) + 1);
	unsigned i;
	int ret;

	if (!c1 || !c2){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
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
			log_warning(__FL__, "Reading from tar - %s", archive_error_string(in));
			break;
		case ARCHIVE_FAILED:
			log_error(__FL__, "Reading from tar - %s", archive_error_string(in));
			return -1;
		default:
			log_fatal(__FL__, "Reading from tar - %s", archive_error_string(in));
			return -1;
		}

		ret = archive_write_data_block(out, buf, size, offset);
		switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_WARN:
			log_warning(__FL__, "Extracting from tar - %s", archive_error_string(out));
			break;
		case ARCHIVE_FAILED:
			log_error(__FL__, "Extracting from tar - %s", archive_error_string(out));
			return -1;
		default:
			log_fatal(__FL__, "Extracting from tar - %s", archive_error_string(out));
			return -1;
		}
	}
	return 0;
}

int tar_extract(const char* tarchive, const char* outdir){
	TAR* tp = NULL;
	TAR* ext = NULL;
	struct archive_entry* entry = NULL;
	int flags = ARCHIVE_EXTRACT_TIME |
		ARCHIVE_EXTRACT_PERM |
		ARCHIVE_EXTRACT_ACL |
		ARCHIVE_EXTRACT_FFLAGS |
		ARCHIVE_EXTRACT_OWNER;
	int ret = 0;

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
		log_fatal(__FL__, "Opening %s returned ARCHIVE_EOF - THIS SHOULD NEVER HAPPEN", tarchive);
		ret = -1;
		goto cleanup;
		break;
	case ARCHIVE_WARN:
		log_warning(__FL__, "Opening tar (%s)", archive_error_string(tp));
		break;
	case ARCHIVE_FAILED:
		log_error(__FL__, "Failed to open tar for extracting (%s)", archive_error_string(tp));
		ret = -1;
		goto cleanup;
	default:
		log_fatal(__FL__, "Failed to open tar for extracting (%s)", archive_error_string(tp));
		ret = -1;
		goto cleanup;
	}

	/* while there are files in the archive */
	while (ret != ARCHIVE_EOF){
		const char* tar_file_path = NULL;
		char* out_path = NULL;
		entry = NULL;

		/* read the header */
		ret = archive_read_next_header(tp, &entry);
		switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_EOF:
			log_info(__FL__, "Reached EOF in %s", tarchive);
			continue;
			break;
		case ARCHIVE_WARN:
			log_warning(__FL__, "Reading next tar header (%s)", archive_error_string(tp));
			break;
		case ARCHIVE_FAILED:
			log_error(__FL__, "Failed to read next tar header (%s)", archive_error_string(tp));
			continue;
			break;
		default:
			log_fatal(__FL__, "Failed to read next tar header (%s)", archive_error_string(tp));
			ret = -1;
			archive_entry_free(entry);
			goto cleanup;
		}

		/* make out path based on /<path of tar>/<path of file> */
		tar_file_path = archive_entry_pathname(entry);
		out_path = malloc(strlen(tar_file_path) + strlen(outdir) + sizeof("/"));
		if (!out_path){
			log_fatal(__FL__, STR_ENOMEM);
			return -1;
		}
		strcpy(out_path, outdir);
		strcat(out_path, "/");
		strcat(out_path, tar_file_path);

		log_info(__FL__, "Extracting %s to %s", archive_entry_pathname(entry), out_path);

		archive_entry_set_pathname(entry, out_path);

		/* set the output directory */
		ret = archive_write_header(ext, entry);
		switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_WARN:
			log_warning(__FL__, "Writing tar header (%s)", archive_error_string(ext));
			break;
		case ARCHIVE_FAILED:
			log_error(__FL__, "Failed to write tar header (%s)", archive_error_string(ext));
			free(out_path);
			continue;
			break;
		default:
			log_fatal(__FL__, "Failed to write tar header (%s)", archive_error_string(ext));
			ret = -1;
			free(out_path);
			archive_entry_free(entry);
			goto cleanup;
		}

		/* extract the file */
		ret = copy_data(tp, ext);
		switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_WARN:
			log_debug(__FL__, "copy_data() warned us");
			break;
		case ARCHIVE_FAILED:
			log_debug(__FL__, "copy_data() failed");
			free(out_path);
			continue;
			break;
		default:
			log_fatal(__FL__, "Failed to copy data into tar");
			ret = -1;
			free(out_path);
			archive_entry_free(entry);
			goto cleanup;
		}

		free(out_path);
		/* archive_entry_free(entry); */
	}
	if (ret == ARCHIVE_EOF){
		ret = 0;
	}
	/* cleanup */
	archive_write_finish_entry(ext);

	/*
	   switch (ret){
	   case ARCHIVE_OK:
	   break;
	   case ARCHIVE_WARN:
	   if (verbose){
	   fprintf(stderr, "maketar warning: %s\n", archive_error_string(tp));
	   }
	   break;
	   default:
	   archive_read_close(tp);
	   archive_read_free(tp);
	   archive_write_close(ext);
	   archive_write_free(ext);
	   break;
	   }
	   */
cleanup:
	if (tp){
		archive_read_close(tp);
		archive_read_free(tp);
	}
	if (ext){
		archive_write_close(ext);
		archive_write_free(ext);
	}
	return ret;
}

int tar_extract_file(const char* tarchive, const char* file_intar, const char* file_out){
	TAR* tp = NULL;
	TAR* ext = NULL;
	struct archive_entry* entry = NULL;
	int flags = ARCHIVE_EXTRACT_TIME |
		ARCHIVE_EXTRACT_PERM |
		ARCHIVE_EXTRACT_ACL |
		ARCHIVE_EXTRACT_FFLAGS |
		ARCHIVE_EXTRACT_OWNER;
	int ret = 0;

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
		log_fatal(__FL__, "Opening %s returned ARCHIVE_EOF - THIS SHOULD NEVER HAPPEN", tarchive);
		ret = -1;
		goto cleanup;
		break;
	case ARCHIVE_WARN:
		log_warning(__FL__, "Opening tar (%s)", archive_error_string(tp));
		break;
	case ARCHIVE_FAILED:
		log_error(__FL__, "Failed to open tar for extracting (%s)", archive_error_string(tp));
		ret = -1;
		goto cleanup;
	default:
		log_fatal(__FL__, "Failed to open tar for extracting (%s)", archive_error_string(tp));
		ret = -1;
		goto cleanup;
	}
	/* while there are files in the archive */
	while (ret != ARCHIVE_EOF){
		entry = NULL;
		/* read the header */
		ret = archive_read_next_header(tp, &entry);
		switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_EOF:
			log_info(__FL__, "%s not found in %s", file_intar, tarchive);
			return ARCHIVE_EOF;
			break;
		case ARCHIVE_WARN:
			log_warning(__FL__, "Reading tar header (%s)", archive_error_string(tp));
			break;
		case ARCHIVE_FAILED:
			log_error(__FL__, "Failed to read tar header (%s)", archive_error_string(tp));
			continue;
			break;
		default:
			log_fatal(__FL__, "Failed to read tar header (%s)", archive_error_string(tp));
			ret = -1;
			goto cleanup;
		}

		/* check if the thing matches our file */
		if (strcmp(archive_entry_pathname(entry), file_intar) != 0){
			continue;
		}
		/* set the output directory */
		archive_entry_set_pathname(entry, file_out);
		ret = archive_write_header(ext, entry);
		switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_WARN:
			log_warning(__FL__, "Writing tar header (%s)", archive_error_string(ext));
			break;
		case ARCHIVE_FAILED:
			log_error(__FL__, "Failed to write tar header (%s)", archive_error_string(ext));
			continue;
			break;
		default:
			log_fatal(__FL__, "Failed to write tar header (%s)", archive_error_string(ext));
			ret = -1;
			goto cleanup;
		}

		/* extract the file */
		ret = copy_data(tp, ext);
		switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_WARN:
			log_debug(__FL__, "copy_data() warned us");
			break;
		case ARCHIVE_FAILED:
			log_debug(__FL__, "copy_data() failed");
			continue;
			break;
		default:
			log_fatal(__FL__, "copy_data() failed");
			ret = -1;
			goto cleanup;
		}
		break;

		archive_entry_free(entry);
	}

	/* cleanup */
	ret = archive_write_finish_entry(ext);
cleanup:
	if (tp){
		archive_read_close(tp);
		archive_read_free(tp);
	}
	if (ext){
		archive_write_close(ext);
		archive_write_free(ext);
	}
	return ret;
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

const char* compressor_to_string(COMPRESSOR comp){
	switch(comp){
	case COMPRESSOR_GZIP:
		return "gzip";
	case COMPRESSOR_BZIP2:
		return "bzip2";
	case COMPRESSOR_XZ:
		return "xz";
	case COMPRESSOR_LZ4:
		return "lz4";
	case COMPRESSOR_NONE:
		return "none";
	default:
		return "unknown";
	}
}
