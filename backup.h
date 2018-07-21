/** @file backup.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __BACKUP_H
#define __BACKUP_H

#include "options/options.h"

/**
 * @brief Performs a backup based on the options structure specified.
 *
 * @param opt The options structure to use.<br>
 * The output will be placed into the path specified by opt->output_directory<br>
 * The output will also be uploaded to the cloud based on opt->cloud_options<br>
 * The output will be compressed based on opt->comp_algorithm and opt->comp_level<br>
 * The output will be encrypted based on opt->enc_algorithm.
 *
 * @return 0 on success, or negative on failure.
 */
int backup(const struct options* opt);

#endif
