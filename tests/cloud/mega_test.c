#include "../test_base.h"
#include "../../cloud/mega.h"
#include "../../log.h"
#include <string.h>
#include <time.h>

const char* const sample_username = "***REMOVED***";
const char* const sample_password = "***REMOVED***";
const char* const sample_file = "test.txt";
const char* const sample_file2 = "test2.txt";
const char* const sample_file3 = "test3.txt";
const unsigned char sample_data[] = "pizza";
const unsigned char sample_data2[] = "avocado";

void test_MEGAdownload(void){
	MEGAhandle* mh;
	char buf[256];
	struct file_node** fn;
	size_t fn_len;
	size_t fn_len_prev;
	size_t i;

	create_file(sample_file, sample_data, strlen((const char*)sample_data));
	create_file(sample_file2, sample_data2, strlen((const char*)sample_data2));

	printf_blue("Testing MEGAdownload()\n");

	printf_yellow("Calling MEGAlogin()\n");
	massert(MEGAlogin(sample_username, sample_password, &mh) == 0);

	printf_yellow("Calling MEGAmkdir()\n");
	massert(MEGAmkdir("/test1", mh) >= 0);

	printf_yellow("Calling MEGAupload()\n");
	massert(MEGAupload(sample_file, "/test1", "Upload test 1", mh) == 0);
	massert(MEGAupload(sample_file2, "/test1", "Upload test 2", mh) == 0);

	printf_yellow("Calling MEGAreaddir()\n");
	massert(MEGAreaddir("/test1", &fn, &fn_len, mh) == 0);

	for (i = 0; i < fn_len; ++i){
		struct tm time;
		char buf[256];

		memcpy(&time, localtime(&(fn[i]->time)), sizeof(struct tm));
		strftime(buf, sizeof(buf), "%c", &time);
		printf("%s (%s)\n", fn[i]->name, buf);
	}

	printf_yellow("Calling MEGArm()\n");
	sprintf(buf, "/test1/%s", sample_file2);
	massert(MEGArm(buf, mh) == 0);

	printf_yellow("Checking that it worked\n");
	fn_len_prev = fn_len;
	massert(MEGAreaddir("/test1", &fn, &fn_len, mh) == 0);
	massert(fn_len == fn_len_prev - 1);

	printf_yellow("Calling MEGAdownload()\n");
	sprintf(buf, "/test1/%s", sample_file);
	printf("%s\n", buf);
	massert(MEGAdownload(buf, sample_file3, "Downloading file", mh) == 0);

	printf_yellow("Checking that the files match\n");
	massert(memcmp_file_file(sample_file, sample_file3) == 0);

	printf_yellow("Calling MEGAlogout()\n");
	massert(MEGAlogout(mh) == 0);

	remove(sample_file);
	remove(sample_file2);
	remove(sample_file3);
	printf_green("Finished testing MEGAdownload()\n\n");
}

int main(void){
	set_signal_handler();
	log_setlevel(LEVEL_INFO);

	test_MEGAdownload();
	return 0;
}
