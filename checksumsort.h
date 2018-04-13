/* checksumsort.h -- external sorting of checksum file
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __CHECKSUMSORT_H
#define __CHECKSUMSORT_H

#include <stdio.h>
#include "readfile.h"

typedef struct element{
	char* file;
	char* checksum;
}element;

typedef struct minheapnode{
	element* e;
	int i;
}minheapnode;

int write_element_to_file(FILE* fp, element* e);
element* get_next_checksum_element(FILE* fp);
element* get_checksum_element_index(FILE* fp, int index);
int median_of_three(element** elements, int low, int high);
void quicksort_elements(element** elements, int low, int high);
void free_element(element* e);
void free_element_array(element** elements, size_t size);
void free_filearray(FILE** elements, size_t size);
int create_initial_runs(FILE* in_file, FILE*** out, size_t* n_files);
int merge_files(FILE** in, size_t n_files, FILE* out_file);
int search_file(FILE* fp, const char* key, char** checksum);

#endif
