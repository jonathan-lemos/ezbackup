#ifndef __EVPERROR_H
#define __EVPERROR_H

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

#define CONTEXT_EVP (0)
#define CONTEXT_REGULAR (1)
#define CONTEXT_ARCHIVE (2)

int evp_geterror();
char* evp_strerror(unsigned long err);

#endif
