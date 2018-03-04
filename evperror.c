/* prototypes and #defines */
#include "evperror.h"
/* ERR_error_string() */
#include <openssl/err.h>
/* strcpy */
#include <string.h>

int evp_geterror(void){
	return ERR_get_error();
}

/* converts err to human-readable string */
char* evp_strerror(unsigned long err){
	/* must be 120 chars to hold all possible errors
	 * must be static so it doesn't get popped off the stack on function return, causing undefined behavior */
	static char err_buf[120];

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
		default:
			ERR_load_crypto_strings();
			ERR_error_string(err, err_buf);
			ERR_free_strings();
	}
	return err_buf;
}
