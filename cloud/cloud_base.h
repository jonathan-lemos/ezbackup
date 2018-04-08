/* cloud_base.h -- common functions for cloud modules
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CLOUD_BASE_H
#define __CLOUD_BASE_H

#include <stdint.h>

struct file_node{
	char*   name;
	int64_t time;
};

#endif
