/*
 * mega.h - MEGA integration module
 * Copyright (C) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __MEGA_H
#define __MEGA_H

typedef void MEGAhandle;

#ifdef __cplusplus
extern "C"{
#endif

int MEGAlogin(const char* username, const char* password, MEGAhandle* out);
int MEGAmkdir(const char* dir, MEGAhandle* mh);
int MEGAreaddir(const char* dir, char** out, MEGAhandle* mh);
int MEGAdownload(const char* download_path, const char* out_file, const char* msg, MEGAhandle* mh);
int MEGAupload(const char* in_file, const char* upload_path, const char* msg, MEGAhandle* mh);
int MEGAlogout(MEGAhandle* mh);

#ifdef __cplusplus
}
#endif


#endif
