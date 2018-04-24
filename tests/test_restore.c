#include "test_base.h"
#include "../restore.h"
#include "../error.h"
#include <unistd.h>
#include <string.h>

void test_getcwd_malloc(void){
	char* cwd;
	char buf[8192];

	printf_blue("Testing getcwd_malloc()\n");

	getcwd(buf, sizeof(buf));

	printf_yellow("Calling getcwd_malloc()\n");
	cwd = getcwd_malloc();
	massert(cwd);

	printf_yellow("Checking that it matches\n");
	massert(strcmp(cwd, buf) == 0);

	free(cwd);

	printf_green("Finished testing getcwd_malloc()\n\n");
}

void test_restore_local_menu(void){
	struct options* opt;
}
