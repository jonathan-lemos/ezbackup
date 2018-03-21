/*
 * Checksum file external sorting module
 * Copyright (C) 2018 Jonathan Lemos
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __CHECKSUMSORT_H
#define __CHECKSUMSORT_H

#include <stdio.h>

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
