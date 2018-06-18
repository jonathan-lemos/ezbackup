#include "test_base.h"
#include "../backup.h"
#include "../log.h"
#include <errno.h>
#include <sys/resource.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

static char* rel_path(const char* abs_path){
	static char rpath[4096];

	getcwd(rpath, sizeof(rpath));
	strcat(rpath, "/");
	strcat(rpath, abs_path);

	return rpath;
}

static void rmdir_recursive(const char* path){
	struct dirent* dnt;
	DIR* dp = opendir(path);

	massert(dp);
	while ((dnt = readdir(dp)) != NULL){
		char* path_true;
		struct stat st;

		if (!strcmp(dnt->d_name, ".") || !strcmp(dnt->d_name, "..")){
			continue;
		}

		path_true = malloc(strlen(path) + strlen(dnt->d_name) + 3);
		strcpy(path_true, path);
		if (path[strlen(path) - 1] != '/'){
			strcat(path_true, "/");
		}
		strcat(path_true, dnt->d_name);

		lstat(path_true, &st);
		if (S_ISDIR(st.st_mode)){
			rmdir_recursive(path_true);
			free(path_true);
			continue;
		}

		remove(path_true);
		free(path_true);
	}
	closedir(dp);
	rmdir(path);
}

void test_backup(void){
}

int main(void){
	set_signal_handler();
	log_setlevel(LEVEL_INFO);
	/*
	   test_disable_core_dumps();
	   test_extract_prev_checksums();
	   test_encrypt_file();
	   test_rename_ex();
	   */
	printf("Test not written yet\n");
	return 0;
	test_backup();
	return 0;
}
