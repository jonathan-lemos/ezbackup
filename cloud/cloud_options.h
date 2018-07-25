/** @file cloud/cloud_options.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CLOUD_OPTIONS_H
#define __CLOUD_OPTIONS_H

/**
 * @brief List of available cloud providers
 */
enum cloud_provider{
	CLOUD_INVALID = 0,/**< Invalid cloud provider */
	CLOUD_MEGA = 1,   /**< mega.nz */
	CLOUD_NONE = 2    /**< No cloud provider */
};

/**
 * @brief Contains information for logging into a cloud account.
 */
struct cloud_options{
	/** @brief The cloud service to use */
	enum cloud_provider cp;
	/** @brief The account's username/email (optional).
	 * If this string is NULL, it will be asked for upon login.<br>
	 * This string must be dynamically allocated or set through co_set_username().
	 * @see co_set_username()
	 */
	char* username;
	/** @brief The account's password (optional).
	 * If this string is NULL, it will be asked for upon login.<br>
	 * This string must be dynamically allocated or set through co_set_password().
	 * @see co_set_password()
	 */
	char* password;
	/** @brief The base directory of the backups.
	 * If this string is NULL, it will be asked for upon login.<br>
	 * This string must be dynamically allocated or set through co_set_password().
	 * @see co_set_password()
	 */
	char* upload_directory;
};

/**
 * @brief Creates a new cloud options structure.
 * @see struct cloud_options
 *
 * @return A new cloud options structure.<br>
 * Member cp will be set to CLOUD_NONE.<br>
 * Members username and password will be set to NULL.<br>
 * Member upload_directory will be set to "/Backups"<br>
 * <br>
 * This structure must be freed with co_free() when no longer in use.
 * @see co_free()
 */
struct cloud_options* co_new(void);

/**
 * @brief Sets the username field of a cloud options structure to a value, freeing the old one if it exists.
 *
 * @param co The cloud options structure to update.
 *
 * @param username The username to set.<br>
 * This parameter can be NULL.
 *
 * @return 0 on success, or negative on failure.
 */
int co_set_username(struct cloud_options* co, const char* username);

/**
 * @brief Sets the password field of a cloud options structure to a value, freeing the old one if it exists.
 *
 * @param co The cloud options structure to update.
 *
 * @param password The password to set.<br>
 * This parameter can be NULL.
 *
 * @return 0 on success, or negative on failure.
 */
int co_set_password(struct cloud_options* co, const char* password);

/**
 * @brief Sets the upload_directory field of a cloud options structure to a value, freeing the old one if it exists.
 *
 * @param co The cloud options structure to update.
 *
 * @param upload_directory The upload directory to set.<br>
 * This parameter can be NULL.
 *
 * @return 0 on success, or negative on failure.
 */
int co_set_upload_directory(struct cloud_options* co, const char* upload_directory);

/**
 * @brief Sets the upload_directory field of a cloud options structure to its default value, freeing the old one if it exists.
 *
 * @param co The cloud options structure to update.
 *
 * @return 0 on success, or negative on failure.
 */
int co_set_default_upload_directory(struct cloud_options* co);

/**
 * @brief Sets the cp field of a cloud_options structure.<br>
 * `co_set_cp(co, val)` is identical to `co->cp = val`<br>
 * This function only exists for completeness with the other functions.
 *
 * @param co The cloud options structure to update.
 *
 * @param cp The cloud provider value to set.
 *
 * @return 0
 */
int co_set_cp(struct cloud_options* co, enum cloud_provider cp);

/**
 * @brief Converts a string to its equivalent CLOUD_PROVIDER.<br>
 * Example: "mega.nz" -> CLOUD_MEGA
 *
 * @param str The string to convert.
 *
 * @return The string's corresponding CLOUD_PROVIDER, or CLOUD_INVALID if the string does not match any valid entries
 */
enum cloud_provider cloud_provider_from_string(const char* str);

/**
 * @brief Converts a CLOUD_PROVIDER to its string equivalent<br>
 * Example: CLOUD_MEGA -> "mega.nz"
 *
 * @param cp The CLOUD_PROVIDER to convert.
 *
 * @return The CLOUD_PROVIDER's equivalent string representation, or NULL for an invalid value.
 */
const char* cloud_provider_to_string(enum cloud_provider cp);

/**
 * @brief Frees all memory associated with a cloud options structure.
 *
 * @param co The cloud options structure to free.
 *
 * @return void
 */
void co_free(struct cloud_options* co);

/**
 * @brief Compares the contents of two cloud_options structures, returning 0 if they are equivalent, or non-zero if they are not.<br>
 * This function will always return the same value for the same two cloud options structures.
 *
 * @param co1 The first cloud options structure.
 *
 * @param co2 The second cloud options structure.
 *
 * @return 0 if they are equivalent, non-zero if not.
 */
int co_cmp(const struct cloud_options* co1, const struct cloud_options* co2);

#endif
