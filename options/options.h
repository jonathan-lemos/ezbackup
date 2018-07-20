/** @file options/options.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __OPTIONS_H
#define __OPTIONS_H

/* compressor */
#include "../compression/zip.h"
#include "../cloud/cloud_options.h"
#include "../strings/stringarray.h"
#include <openssl/evp.h>

/**
 * @brief An operation for the main program to perform.
 */
enum operation{
	OP_INVALID = 0,   /**< Invalid operation. */
	OP_BACKUP  = 1,   /**< Backup. */
	OP_RESTORE = 2,   /**< Restore. */
	OP_CONFIGURE = 3, /**< Configure. */
	OP_EXIT = 4       /**< Exit. */
};

/**
 * @brief A structure containing the program's options.
 */
struct options{
	char*                 prev_backup;      /**< This option is deprecated. */
	struct string_array*  directories;      /**< A list of directories to back up. This cannot be NULL, but it can contain 0 strings. */
	struct string_array*  exclude;          /**< A list of directories to exclude. This cannot be NULL, but it can contain 0 strings. */
	const EVP_MD*         hash_algorithm;   /**< The hash algorithm to use for checksum files. */
	const EVP_CIPHER*     enc_algorithm;    /**< The encryption algorithm to use. */
	char*                 enc_password;     /**< The encryption password to use. This can be NULL. Otherwise, it must be dynamically allocated. */
	enum compressor       comp_algorithm;   /**< The compression algorithm to use. */
	int                   comp_level;       /**< The compression level to use. 0 uses the default level. */
	char*                 output_directory; /**< The backup directory on disk. This must be dynamically allocated. */
	struct cloud_options* cloud_options;    /**< The cloud options to use. This cannot be NULL, but its members can be. */
	union tagflags{                         /**< The special flags to use. This can be represented as a series of bits or as an unsigned integer. */
		struct tagbits{
			unsigned      flag_verbose: 1;  /**< Verbose output. */
		}bits;
		unsigned          dword;            /**< All flags as an unsigned integer. */
	}flags;
};

/**
 * @brief Displays the version of this program.
 *
 * @return void
 */
void version(void);

/**
 * @brief Displays the usage of this program, such as command line arguments.
 *
 * @return void
 */
void usage(const char* progname);

/**
 * @brief Creates a new options structure with a set of default values.
 *
 * @return A new options structure, or NULL on failure.<br>
 * This structure must be freed with options_free() when no longer in use.
 */
struct options* options_new(void);

/**
 * @brief Compares two options structures.
 * Returns 0 if they are the same and non-zero if they are different.<br>
 * The same two structures will always result in the same value.
 *
 * @param opt1 The first options structure.
 *
 * @param opt2 The second options structure.
 *
 * @return 0 if they are the same, or non-zero if they are different.
 */
int options_cmp(const struct options* opt1, const struct options* opt2);

/**
 * @brief Frees all memory associated with an options structure.
 *
 * @param o The options structure to free.<br>
 * This can be NULL, in which case this function does nothing.
 *
 * @return void
 */
void options_free(struct options* o);

/**
 * @brief Creates an options structure from command-line arguments.
 *
 * @param argc The argument count. This is the first argument to main().
 *
 * @param argv The argument vector. This is the second argument to main().
 *
 * @param out A pointer to an options structure that should be filled.<br>
 * This cannot be NULL, but can point to NULL.<br>
 * If it does not point to NULL, the existing structure will be freed.<br>
 * If this function fails, the structure will be set to NULL.<br>
 * This structure must be freed when no longer in use.
 *
 * @param op_out A pointer to an enum operation that corresponds to the operation that the user selects.<br>
 * This will be set to OP_INVALID if the user does not select an operation or if there was an error.
 *
 * @return 0 on success, or negative on failure.
 */
int parse_options_cmdline(int argc, char** argv, struct options** out, enum operation* op_out);

/**
 * @brief Creates an options structure from a file.
 * This file must be created by write_options_tofile()<br>
 * For most cases, get_prev_options() is a better alternative, as it reads from a predetermined location.
 * @see write_options_tofile()
 * @see get_prev_options()
 *
 * @param file Path to an options file.
 *
 * @param out A pointer to an options structure that should be filled.<br>
 * This cannot be NULL, but can point to NULL.<br>
 * If it does not point to NULL, the existing structure will be freed.<br>
 * If this function fails, the structure will be set to NULL.<br>
 * This structure must be freed with options_free() when no longer in use.
 *
 * @return 0 on success, or negative on failure.
 */
int parse_options_fromfile(const char* file, struct options** out);

/**
 * @brief Writes an options structure to a file.
 * The contents can be parsed with parse_options_fromfile()<br>
 * For most cases, set_prev_options() is a better alternative, as it writes to a predetermined location.
 * @see parse_options_fromfile()
 *
 * @param file Path to the destination of the options file.<br>
 * If the file already exists, it will be overwritten.
 *
 * @param opt The options structure to write.
 *
 * @return 0 on success, or negative on failure.
 */
int write_options_tofile(const char* file, const struct options* opt);

/**
 * @brief Gets the previous options from a previous call to set_prev_options()
 * This function is valid even when the program exits and reopens, as long as the file created by set_prev_options() still exists.<br>
 * If no previous file could be found, this function creates a new default options file on disk.
 * @see set_prev_options()
 *
 * @param out A pointer to an options structure that should be filled.<br>
 * This cannot be NULL, but can point to NULL.<br>
 * If it does not point to NULL, the existing structure will be freed.<br>
 * If this function fails, the structure will be set to NULL.<br>
 * This structure must be freed with options_free() when no longer in use.
 *
 * @return 0 on success, negative on failure.
 */
int get_prev_options(struct options** out);

/**
 * @brief Writes an options structure to a predetermined location on disk.
 * This options file can be retrieved with get_prev_options().
 * @see get_prev_options()
 *
 * @param opt The options file to write to disk.<br>
 * This argument can be NULL, in which case the default options are written to the disk.
 *
 * @return 0 on success, negative on failure.
 */
int set_prev_options(const struct options* opt);

/**
 * @brief Converts an enum operation to its string equivalent.
 *
 * @param op The enum operation.
 *
 * @return The enum operation's string equivalent, or NULL if the operation was invalid.
 */
const char* operation_tostring(enum operation op);

#endif
