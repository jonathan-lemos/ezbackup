#ifndef __MEGA_H
#define __MEGA_H

typedef void* MEGAhandle;

#ifdef __cplusplus
extern "C"{
#endif

int MEGAlogin(const char* username, const char* password, MEGAhandle* out);
int MEGAmkdir(const char* dir, MEGAhandle mh);
int MEGAreaddir(const char* dir, char** out, MEGAhandle mh);
int MEGAdownload(const char* download_path, const char* out_file, MEGAhandle mh);
int MEGAupload(const char* in_file, const char* upload_path, MEGAhandle mh);
int MEGAlogout(MEGAhandle mh);

#ifdef __cplusplus
}
#endif


#endif
