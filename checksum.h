#ifndef __CHECKSUM_H
#define __CHECKSUM_H

/* FILE* */
#include <stdio.h>

typedef struct element{
	char* file;
	char* checksum;
}element;


int checksum(const char* file, const char* algorithm, unsigned char** out, unsigned* len);
int add_checksum_to_file(const char* file, const char* algorithm, FILE* out);
int search_checksum_file(FILE* in, const char* file, char** out);

#ifdef __TESTING__
element* get_next_checksum_element(FILE* fp);
element* get_checksum_element_index(FILE* fp, int index);
int median_of_three(element** elements, int low, int high);
void quicksort_elements(element** elements, int low, int high);
#endif

#endif
