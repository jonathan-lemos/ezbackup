/* coredumps.c - enables/disables core dumps
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "coredumps.h"
#include "error.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>

int __coredumps(int enable){
	static struct rlimit rl_prev;
	static int previously_disabled = 0;
	struct rlimit rl;
	int ret = 0;
	if (!enable){
		if (getrlimit(RLIMIT_CORE, &rl_prev) != 0){
			log_warning_ex("Failed to get previous core dump limits (%s)", strerror(errno));
			ret = -1;
		}
		rl.rlim_cur = 0;
		rl.rlim_max = 0;
		if (setrlimit(RLIMIT_CORE, &rl) != 0){
			log_warning_ex("Failed to disable core dumps (%s)", strerror(errno));
			ret = -1;
		}
		previously_disabled = 1;
	}
	else{
		if (!previously_disabled){
			return 0;
		}
		if (setrlimit(RLIMIT_CORE, &rl_prev) != 0){
			log_warning_ex("Failed to restore previous core dump limits (%s)", strerror(errno));
			ret = -1;
		}
		previously_disabled = 0;
	}
	return ret;
}

int disable_core_dumps(void){
	return __coredumps(0);
}

int enable_core_dumps(void){
	return __coredumps(1);
}
