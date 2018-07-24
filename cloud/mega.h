/** @file cloud/mega.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __MEGA_H
#define __MEGA_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif
#include <sys/stat.h>

#ifndef MEGA_WAIT_MS
#define MEGA_WAIT_MS (10000)
#endif

/**< A mega::MegaApi class that's been typecasted to void for compatibillity with C */
typedef void MEGAhandle;

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @brief Logs into a MEGA account.
 *
 * @param username The email to log into.<br>
 * This cannot be NULL.
 *
 * @param password The corresponding account's password.<br>
 * This cannot be NULL.
 *
 * @param out A pointer to a handle that this function will fill.<br>
 * The handle will be set to NULL if this function fails.<br>
 * This handle must be freed with MEGAlogout() when no longer in use.
 *
 * @return 0 on success, or negative on failure.
 * @see MEGAlogout()
 */
int MEGAlogin(const char* username, const char* password, MEGAhandle** out);

/**
 * @brief Makes a directory within a MEGA account.
 *
 * @param dir The directory to create.
 * This will not recursively create directories.
 *
 * @param mh A handle returned by MEGAlogin().
 * @see MEGAlogin().
 *
 * @return 0 on success, positive if the directory already exists, negative on failure.
 */
int MEGAmkdir(const char* dir, MEGAhandle* mh);

/**
 * @brief Reads the contents of a directory within a MEGA account.
 *
 * @param dir The directory to read.
 *
 * @param out A pointer to a string array that will contain the entries in the directory.<br>
 * This string array will be set to NULL if the function fails.
 *
 * @param out_len A pointer to an integer that will contain the length of the output array.<br>
 * This will be set to 0 if this function fails.
 *
 * @param mh A handle returned by MEGAlogin().
 * @see MEGAlogin()
 *
 * @return 0 on success, or negative on failure.
 */
int MEGAreaddir(const char* dir, char*** out, size_t* out_len, MEGAhandle* mh);

/**
 * @brief Stats a file or directory within a MEGA account.
 *
 * @param file_path The file or directory to stat.
 *
 * @param out A pointer to a stat structure to fill.<br>
 * This can be NULL, in which case this function merely checks if the file exists.
 *
 * @param mh A handle returned by MEGAlogin()
 * @see MEGAlogin()
 *
 * @return 0 on success, or negative on failure.
 */
int MEGAstat(const char* file_path, struct stat* out, MEGAhandle* mh);

/**
 * @brief Renames a file or directory within a MEGA account.
 *
 * @param _old Path to the file or directory to rename.
 *
 * @param _new The file or folder's new name.
 *
 * @mh A handle returned by MEGAlogin()
 * @see MEGAlogin()
 *
 * @return 0 on success, or negative on failure.
 */
int MEGArename(const char* _old, const char* _new, MEGAhandle* mh);

/**
 * @brief Downloads a file stored within a MEGA account.
 *
 * @param download_path The file to download.
 *
 * @param out_file The path on disk to download the file to.
 *
 * @param msg The progress message to display.<br>
 * This argument can be NULL, in which case no progress bar is displayed.
 *
 * @param mh A handle returned by MEGAlogin()
 * @see MEGAlogin()
 *
 * @return 0 on success, or negative on failure.
 */
int MEGAdownload(const char* download_path, const char* out_file, const char* msg, MEGAhandle* mh);

/**
 * @brief Uploads a file stored within a MEGA account.
 *
 * @param in_file The file on disk to upload.
 *
 * @param upload_path The directory to upload the file to.<br>
 * This can optionally have the filename appended to it.<br>
 * Example: /upload/dir or /upload/dir/file.txt is fine.<br>
 * <br>
 * If the directory does not exist, it will be created.
 *
 * @param msg The progress message to display.<br>
 * This argument can be NULL, in which case no progress bar is displayed.
 *
 * @param mh A handle returned by MEGAlogin()<br>
 * @see MEGAlogin()
 *
 * @return 0 on success, or negative on failure.
 */
int MEGAupload(const char* in_file, const char* upload_path, const char* msg, MEGAhandle* mh);

/**
 * @brief Removes a file or directory stored within a MEGA account.
 *
 * @param file The file or directory to remove.<br>
 * If a directory is specified, all of its contents will be removed as well.
 *
 * @param mh A handle returned by MEGAlogin()<br>
 * @see MEGAlogin()
 *
 * @return 0 on success, or negative on failure.
 */
int MEGArm(const char* file, MEGAhandle* mh);

/**
 * @brief Logs out of MEGA and frees all memory associated with its handle.
 *
 * @param mh A handle returned by MEGAlogin()
 * @see MEGAlogin()
 *
 * @return 0 on success, or negative on failure.
 */
int MEGAlogout(MEGAhandle* mh);

#ifdef __cplusplus
}
#endif

#endif
