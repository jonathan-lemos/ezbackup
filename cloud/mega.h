/*
 * MEGA integration module
 * Copyright (C) 2018 Jonathan Lemos
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __MEGA_H
#define __MEGA_H

typedef void* MEGAhandle;

#ifdef __cplusplus
extern "C"{
#endif

int MEGAlogin(const char* username, const char* password, MEGAhandle* out);
int MEGAmkdir(const char* dir, MEGAhandle mh);
int MEGAreaddir(const char* dir, char** out, MEGAhandle mh);
int MEGAdownload(const char* download_path, const char* out_file, const char* msg, MEGAhandle mh);
int MEGAupload(const char* in_file, const char* upload_path, const char* msg, MEGAhandle mh);
int MEGAlogout(MEGAhandle mh);

#ifdef __cplusplus
}
#endif


#endif
