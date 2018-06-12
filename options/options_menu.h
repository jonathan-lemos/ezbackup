/* options_menu.h -- menu driver for options.h
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __OPTIONS_MENU_H
#define __OPTIONS_MENU_H

#include "options.h"

enum OPERATION menu_main(struct options* opt);

#ifdef __UNIT_TESTING__
int menu_main_configure(struct options* opt);
int menu_compression_level(struct options* opt);
int menu_compressor(struct options* opt);
int menu_checksum(struct options* opt);
int menu_encryption(struct options* opt);
int menu_enc_password(struct options* opt);
int menu_directories(struct options* opt);
int menu_exclude(struct options* opt);
int menu_output_directory(struct options* opt);
int menu_cloud_provider(struct options* opt);
int menu_cloud_username(struct options* opt);
int menu_cloud_password(struct options* opt);

int menu_cloud_main(struct options* opt);
int menu_compression_main(struct options* opt);
int menu_directories_main(struct options* opt);
#endif

#endif
