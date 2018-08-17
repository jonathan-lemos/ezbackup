/** @file file/filehelper.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __FILEHELPER_H
#define __FILEHELPER_H

#include "../attribute.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#ifndef BUFFER_LEN

#ifndef __UNIT_TESTING__
#define BUFFER_LEN (1 << 16) /**< The length of most file I/O buffers used within this program (64KB) */
#else
#define BUFFER_LEN (1 << 5)
#endif

#endif

/**
 * @brief Structure that holds a FILE* and filename of a temporary file.
 */
struct TMPFILE{
	FILE* fp;   /**< The temporary file's FILE*. Opened for both reading and writing. */
	char* name; /**< The temporary file's filename */
};

/**
 * @brief Reads bytes from a file.<br>
 *
 * Nearly identical to fread(dest, 1, length, fp), but this function contains some extra error checking.
 *
 * @param fp File to read from.<br>
 * This file must be opened in reading mode.
 *
 * @param dest The destination buffer.
 *
 * @param length The length of the destination buffer in bytes.
 *
 * @return Number of bytes successfully read.
 */
int read_file(FILE* fp, unsigned char* dest, size_t length);

/**
 * @brief Opens a temporary file.
 * @see struct TMPFILE
 *
 * @return A temporary file structure, or NULL on error.<br>
 * The filename will have the following format:<br>
 * /var/tmp/tmp_XXXXXX<br>
 * <br>
 * The temporary file is not placed directly in /tmp, since many unix systems mount tmp to RAM, preventing excessively large temporary files from being created.<br>
 * <br>
 * This structure must be temp_fclose()'d when no longer in use.
 * @see temp_fclose()
 */
struct TMPFILE* temp_fopen(void) EZB_MALLOC_LIKE;

/**
 * @brief Synchronizes the FILE* and filename.<br>
 *
 * This function is necessary when you need to flush the internal FILE*'s buffers before using the filename, or when the filename is directly used and you need to update the FILE* to reflect those changes.<br>
 * The structure's file pointer will be rewound if this function sucessfully completes.<br>
 * On failure, the contents of the structure are undefined and should be freed with temp_fclose().
 * @see temp_fclose()
 *
 * @param tfp The temporary file to update.
 *
 * @return 0 on success, or negative on failure
 */
int temp_fflush(struct TMPFILE* tfp);

/**
 * @brief Deletes a temporary file and frees all memory associated with it.
 *
 * @param tfp The temporary file to close.
 *
 * @return void
 */
void temp_fclose(struct TMPFILE* tfp);

/**
 * @brief Checks if a FILE* is opened for reading.
 *
 * @param fp The FILE* to check.
 *
 * @return 1 if the FILE* is opened for reading. 0 if not or if there was an error.
 */
int file_opened_for_reading(FILE* fp);

/**
 * @brief Checks if a FILE* is opened for writing.
 *
 * @param fp The FILE* to check.
 *
 * @return 1 if the FILE* is opened for writing. 0 if not or if there was an error.
 */
int file_opened_for_writing(FILE* fp);

/**
 * @brief Gets the size in bytes of the file pointed to by a file pointer.
 *
 * @param fp The FILE* to determine the size of.
 *
 * @param out A pointer to an integer that will be filled with the size of the file.
 *
 * @return 0 on success, negative on failure.
 */
int get_file_size_fp(FILE* fp, uint64_t* out);

/**
 * @brief Gets the size in bytes of a file
 *
 * @param fp The filename of the file to determine the size of.
 *
 * @param out A pointer to an integer that will be filled with the size of the file.
 *
 * @return 0 on success, negative on failure.
 */
int get_file_size(const char* file, uint64_t* out);

/**
 * @brief Copies a file to its destination
 *
 * @param _old The source file.
 *
 * @param _new The destination.
 *
 * @return 0 on success, or negative on failure.
 */
int copy_file(const char* _old, const char* _new);

/**
 * @brief Moves a file.<br>
 *
 * This function works instantly if the source and destination are on the same disk.<br>
 * Otherwise, it has to copy the file and then remove the old one.
 *
 * @param _old The file to rename.
 *
 * @param _new The file's new path.
 *
 * @return 0 on success, or negative on failure.<br>
 * On failure, the old file is unmoved.
 */
int rename_file(const char* _old, const char* _new);

/**
 * @brief Checks if a directory exists.
 *
 * @param The path of the directory to verify the existence of.
 *
 * @return 1 if it exists, or 0 if not or if there was a failure.
 */
int directory_exists(const char* path);

/**
 * @brief Checks if a file exists.
 *
 * @param The path of the file to verify the existence of.
 *
 * @return 1 if it exists, or 0 if not or if there was a failure.
 */
int file_exists(const char* path);

/**
 * @brief Creates a directory and its parent directories.
 *
 * @param The directory to create.
 *
 * @return 0 on success, positive if the directory already exists, or negative on failure.
 */
int mkdir_recursive(const char* dir);

/**
 * @brief Removes a directoriy and all of its files.
 *
 * @param The directory to remove.
 *
 * @return 0 on success, negative on failure.
 */
int rmdir_recursive(const char* dir);

#endif
