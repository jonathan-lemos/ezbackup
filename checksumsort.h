#ifndef __CHECKSUMSORT_H
#define __CHECKSUMSORT_H

typedef struct element{
	char* file;
	char* checksum;
}element;

typedef struct minheapnode{
	element* e;
	int i;
}minheapnode;

#include <stdio.h>
int write_element_to_file(FILE* fp, element* e);
element* get_next_checksum_element(FILE* fp);
element* get_checksum_element_index(FILE* fp, int index);
int median_of_three(element** elements, int low, int high);
void quicksort_elements(element** elements, int low, int high);
void free_elements(element** elements, size_t size);
void free_minheapnodes(minheapnode* mhn, size_t size);
int create_initial_runs(const char* in_file, char*** out, size_t* n_files);
int merge_files(FILE** in, size_t n_files, const char* out_file);

#endif
