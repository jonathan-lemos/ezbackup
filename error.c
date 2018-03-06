/* prototypes and #defines */
#include "error.h"
/* TAR* */
#include "maketar.h"
#include <archive.h>
/* ERR_error_string() */
#include <openssl/err.h>
/* strcpy */
#include <string.h>
/* errno strerrror */
#include <errno.h>

static ERR_CONTEXT err_context;
static char err_buf[128];

int err_regularerror(int err){
	err_context = CONTEXT_REGULAR;
	return err;
}

int err_evperror(void){
	err_context = CONTEXT_EVP;
	return ERR_get_error();
}

int err_archiveerror(int err, TAR* tp){
	err_context = CONTEXT_ARCHIVE;
	strcpy(err_buf, archive_error_string(tp));
	return err;
}

int err_errno(int __errno){
	err_context = CONTEXT_ERRNO;
	return __errno;
}

/* converts err to human-readable string */
char* err_strerror(int err){
	if (err == 0){
		strcpy(err_buf, "no error");
		return err_buf;
	}
	switch(err_context){
		case CONTEXT_REGULAR:
			switch(err){
				case ERR_ENCRYPTION_UNINITIALIZED:
					strcpy(err_buf, "filecrypt_set_encryption() not called");
					break;
				case ERR_OUT_OF_MEMORY:
					strcpy(err_buf, "the system is out of memory");
					break;
				case ERR_KEYS_UNINITIALIZED:
					strcpy(err_buf, "crypt_gen_keys() not called");
					break;
				case ERR_FILE_OUTPUT:
					strcpy(err_buf, "could not write to file");
					break;
				case ERR_FILE_INPUT:
					strcpy(err_buf, "could not read from file");
					break;
				case ERR_FILE_INVALID:
					strcpy(err_buf, "the file was invalid");
					break;
				case ERR_FUBAR:
					strcpy(err_buf, "fubar");
					break;
				case ERR_SALT_UNINITIALIZED:
					strcpy(err_buf, "the salt was uninitialized");
					break;
				case ERR_ARGUMENT_NULL:
					strcpy(err_buf, "a required argument was NULL");
					break;
				case ERR_INVALID_ARGUMENT:
					strcpy(err_buf, "one or more arguments were invalid");
					break;
				case ERR_EOF:
					strcpy(err_buf, "end of file was reached");
					break;
				case ERR_OUT_OF_DISK:
					strcpy(err_buf, "there is no space on the disk");
					break;
				default:
					strcpy(err_buf, "unknown error, this should never happen");
					break;
			}
			break;
		case CONTEXT_EVP:
			ERR_load_crypto_strings();
			ERR_error_string(err, err_buf);
			ERR_free_strings();
		case CONTEXT_ARCHIVE:
			return err_buf;
		case CONTEXT_ERRNO:
			return strerror(err);
		default:
			strcpy(err_buf, "invalid error context");
			return err_buf;
	}
	return err_buf;
}
