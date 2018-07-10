/* coredumps.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __COREDUMPS_H
#define __COREDUMPS_H

/**
 * @brief Disables core dumps on the system.
 *
 * This prevents fragments of a password or other sensitive information from showing up in core dumps.
 * @see crypt_getpassword()
 *
 * @return 0 on success, or negative on failure.
 */
int disable_core_dumps(void);

/**
 * @brief Re-enables core dumps on the system if they were previously disabled.
 *
 * This function tends to fail even when disable_core_dumps() works, but core dumps are not necessary for this program to function, so failure to re-enable core dumps is not a major problem.
 * @see disable_core_dumps()
 *
 * @return 0 on success, or negative on failure.
 */
int enable_core_dumps(void);

#endif
