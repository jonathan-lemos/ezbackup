#ifndef __EVPERROR_H
#define __EVPERROR_H

/* TAR* */
#include "maketar.h"

#define ERR_ENCRYPTION_UNINITIALIZED (-1)
#define ERR_OUT_OF_MEMORY (-2)
#define ERR_KEYS_UNINITIALIZED (-3)
#define ERR_FILE_OUTPUT (-4)
#define ERR_FILE_INPUT (-5)
#define ERR_FILE_INVALID (-6)
#define ERR_FUBAR (-7)
#define ERR_SALT_UNINITIALIZED (-8)
#define ERR_ARGUMENT_NULL (-9)
#define ERR_INVALID_ARGUMENT (-10)
#define ERR_EOF (-11)
#define ERR_OUT_OF_DISK (-12)

typedef enum ERR_CONTEXT{
	CONTEXT_INVALID = -1,
	CONTEXT_REGULAR = 0,
	CONTEXT_EVP = 1,
	CONTEXT_ARCHIVE = 2,
	CONTEXT_ERRNO = 3
}ERR_CONTEXT;

int err_regularerror(int err);
int err_evperror(void);
int err_archiveerror(int err, TAR* tp);
int err_errno(int __errno);
char* err_strerror(int err);

#endif
