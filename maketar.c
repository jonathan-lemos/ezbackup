/* prototypes */
#include "maketar.h"
/* read_file() */
#include "readfile.h"
/* errors */
#include "error.h"
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
		return err_regularerror(ERR_ARGUMENT_NULL);
	}

	/* must open as "rb" to prevent \r\n -> \n */
	fp = fopen(filename, "rb");
	if (!fp){
		return err_regularerror(ERR_FILE_INPUT);
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

int tar_add_file(TAR* tp, const char* filename){
	return tar_add_file_ex(tp, filename, filename, 0, NULL);
}

int tar_close(TAR* tp){
	if (!tp){
		return err_regularerror(ERR_ARGUMENT_NULL);
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
		fprintf(stderr, "strcmp_nocase fatal error: out of memory at the jack in the box\n");
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

static int copy_data(TAR* in, TAR* out, int verbose){
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
			if (verbose){
				fprintf(stderr, "maketar warning: %s\n", archive_error_string(in));
			}
			break;
		default:
			return err_archiveerror(ret, in);
		}

		ret = archive_write_data_block(out, buf, size, offset);
		switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_WARN:
			if (verbose){
				fprintf(stderr, "maketar warning: %s\n", archive_error_string(out));
			}
			break;
		default:
			return err_archiveerror(ret, out);
		}
	}
	return ARCHIVE_OK;
}

int tar_extract(const char* tarchive, const char* outdir, int verbose){
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
		ret = err_regularerror(ERR_FILE_INPUT);
		goto cleanup;
		break;
	case ARCHIVE_WARN:
		if (verbose){
			fprintf(stderr, "maketar warning: %s\n", archive_error_string(tp));
		}
		break;
	default:
		err_archiveerror(ret, tp);
		goto cleanup;
	}

	/* while there are files in the archive */
	while (ret != ARCHIVE_EOF){
		const char* tar_file_path = NULL;
		char* out_path = NULL;

		/* read the header */
		ret = archive_read_next_header(tp, &entry);
		switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_EOF:
			continue;
			break;
		case ARCHIVE_WARN:
			if (verbose){
				fprintf(stderr, "maketar warning: %s\n", archive_error_string(tp));
			}
			break;
		case ARCHIVE_FAILED:
			if (verbose){
				fprintf(stderr, "maketar error: %s\n", archive_error_string(tp));
			}
			continue;
			break;
		default:
			err_archiveerror(ret, tp);
			archive_entry_free(entry);
			goto cleanup;
		}

		/* make out path based on /<path of tar>/<path of file> */
		tar_file_path = archive_entry_pathname(entry);
		out_path = malloc(strlen(tar_file_path) + strlen(outdir) + 1);
		if (!out_path){
			return err_regularerror(ERR_OUT_OF_MEMORY);
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
			if (verbose){
				fprintf(stderr, "maketar warning: %s\n", archive_error_string(tp));
			}
			break;
		case ARCHIVE_FAILED:
			if (verbose){
				fprintf(stderr, "maketar error: %s\n", archive_error_string(tp));
			}
			free(out_path);
			continue;
			break;
		default:
			err_archiveerror(ret, tp);
			free(out_path);
			archive_entry_free(entry);
			goto cleanup;
		}

		/* extract the file */
		ret = copy_data(tp, ext, verbose);
		switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_WARN:
			if (verbose){
				fprintf(stderr, "maketar warning: %s\n", archive_error_string(tp));
			}
			break;
		case ARCHIVE_FAILED:
			if (verbose){
				fprintf(stderr, "maketar error: %s\n", archive_error_string(tp));
			}
			free(out_path);
			continue;
			break;
		default:
			err_archiveerror(ret, tp);
			free(out_path);
			archive_entry_free(entry);
			goto cleanup;
		}

		free(out_path);
		archive_entry_free(entry);
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
	archive_read_close(tp);
	archive_read_free(tp);
	archive_write_close(ext);
	archive_write_free(ext);
	return ret;
}

int tar_extract_file(const char* tarchive, const char* file_intar, const char* file_out, int verbose){
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
		return ERR_FILE_INPUT;
		break;
	case ARCHIVE_WARN:
		if (verbose){
			fprintf(stderr, "maketar warning: %s\n", archive_error_string(tp));
		}
		break;
	default:
		err_archiveerror(ret, tp);
		goto cleanup;
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
			if (verbose){
				fprintf(stderr, "maketar warning: %s\n", archive_error_string(tp));
			}
			break;
		case ARCHIVE_FAILED:
			if (verbose){
				fprintf(stderr, "maketar error: %s\n", archive_error_string(tp));
			}
			continue;
			break;
		default:
			err_archiveerror(ret, tp);
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
			if (verbose){
				fprintf(stderr, "maketar warning: %s\n", archive_error_string(tp));
			}
			break;
		case ARCHIVE_FAILED:
			if (verbose){
				fprintf(stderr, "maketar error: %s\n", archive_error_string(tp));
			}
			continue;
			break;
		default:
			err_archiveerror(ret, tp);
		}

		/* extract the file */
		ret = copy_data(tp, ext, verbose);
		switch (ret){
		case ARCHIVE_OK:
			break;
		case ARCHIVE_WARN:
			if (verbose){
				fprintf(stderr, "maketar warning: %s\n", archive_error_string(tp));
			}
			break;
		case ARCHIVE_FAILED:
			if (verbose){
				fprintf(stderr, "maketar error: %s\n", archive_error_string(tp));
			}
			continue;
			break;
		default:
			err_archiveerror(ret, tp);
			goto cleanup;
		}
		break;

		archive_entry_free(entry);
	}


	/* cleanup */
	ret = archive_write_finish_entry(ext);

	/*
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
	   */
cleanup:
	if (entry){
		archive_entry_free(entry);
	}
	archive_read_close(tp);
	archive_read_free(tp);
	archive_write_close(ext);
	archive_write_free(ext);
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
