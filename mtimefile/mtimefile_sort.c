/** @file mtimefile_sort.c
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "mtimefile_sort.h"
#include "entry.h"
#include "../log.h"
#include "../crypt/base16.h"
#include "../file/databuffer.h"
#include "../file/filehelper.h"
#include "../strings/stringhelper.h"
#include "entry.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

struct minheapnode{
	struct entry* e;
	size_t i;
};

EZB_INLINE static void free_entry(struct entry* e){
	if (!e){
		return;
	}
	free(e->file);
	free(e);
}

#define TIME_T_B16_LEN (2 * sizeof(time_t) + 1)

EZB_INLINE static int write_time_t_b16(FILE* fp, time_t time){
	char* time_b16;

	if (to_base16(&(time), sizeof(time), &time_b16) != 0){
		log_error("Failed to convert time to base 16");
		return -1;
	}

	if (strlen(time_b16) + 1 != TIME_T_B16_LEN){
		log_error("Base 16 string is not the correct length");
		free(time_b16);
		return -1;
	}

	if (fwrite(fp, 1, strlen(time_b16) + 1, fp) != strlen(time_b16) + 1){
		log_error("Failed to write base 16 time to file");
		free(time_b16);
		return -1;
	}

	free(time_b16);
	return 0;
}

EZB_INLINE static int read_time_t_b16(FILE* fp, time_t* time_out){
	/* the length of the b16 string including null term */
	char time_b16[TIME_T_B16_LEN];
	time_t* time_buf;
	unsigned time_buf_len;

	if (fread(time_b16, 1, sizeof(time_b16), fp) != sizeof(time_b16)){
		log_error("Failed to write to entry file");
		return -1;
	}

	if (time_b16[sizeof(time_b16) - 1] != '\0'){
		log_error("The read string is not the correct length");
		return -1;
	}

	if (from_base16(time_b16, (void**)&time_buf, &time_buf_len) != 0){
		log_error("Failed to convert base16 string to time_t");
		return -1;
	}

	if (time_buf_len != sizeof(time_t)){
		log_error("The converted base16 string was not the correct length");
		free(time_buf);
		return -1;
	}

	*time_out = *time_buf;
	free(time_buf);
	return 0;
}

int read_entry(FILE* fp, struct entry** out){
	struct entry* ret = NULL;
	struct databuffer buf;
	size_t len_str;
	void* tmp;
	int c;

	if (db_fill(&buf) != 0){
		log_error("Failed to fill data buffer structure.");
		return -1;
	}

	do{
		c = fgetc(fp);
		/* end of file */
		if (c == EOF){
			break;
		}
		if (db_concat_char(&buf, c) != 0){
			log_error("Failed to concatenate character to buffer structure.");
			db_free_contents(&buf);
			return -1;
		}
	}while (c != '\0');

	/* end of file */
	if (c == EOF){
		db_free_contents(&buf);
		*out = NULL;
		return 1;
	}

	ret = malloc(sizeof(*ret));
	if (!ret){
		log_enomem();
		*out = NULL;
		return -1;
	}

	/* buf.data should be a null terminated string at this point */

	/* trim the excess */
	len_str = strlen((char*)buf.data) + 1;
	tmp = realloc(buf.data, len_str);
	if (!tmp){
		log_enomem();
		*out = NULL;
		return -1;
	}

	ret->file = (char*)buf.data;

	/* now read the time from the file */
	if (read_time_t_b16(fp, &(ret->mtime)) != 0){
		log_error("Failed to read b16 time from file");
		free_entry(ret);
		*out = NULL;
		return -1;
	}

	*out = ret;
	return 0;
}

int write_entry(struct entry* e, FILE* fp){
	size_t len;

	/* write filename */
	len = fwrite(e->file, 1, strlen(e->file) + 1, fp);
	if (len != strlen(e->file) + 1){
		log_efwrite("mtime file");
		return -1;
	}

	/* write mtime */
	if (write_time_t_b16(fp, e->mtime) != 0){
		log_error("Failed to write b16 time");
		return -1;
	}

	return 0;
}

static int align_entry_file_forward(FILE* fp){
	int c;
	unsigned char buf[TIME_T_B16_LEN];
	while ((c = fgetc(fp)) != '\0' && c != EOF);
	if (c == EOF){
		return 1;
	}
	if (fread(buf, 1, sizeof(buf), fp) != sizeof(buf)){
		return -1;
	}
	return 0;
}

static int align_entry_file_inplace(FILE* fp){
	char tm_buf[TIME_T_B16_LEN];
	int c;
	do{
		fseek(fp, -2, SEEK_CUR);
	}while (fgetc(fp) != '\0' && ftell(fp) > 1);

	if (ftell(fp) <= 1){
		while ((c = fgetc(fp)) != '\0' && c != EOF);
		if (c == EOF){
			log_error("Invalid file format");
			return -1;
		}
	}

	if (fread(tm_buf, 1, sizeof(tm_buf), fp) != sizeof(tm_buf)){
		log_error("Failed to read file");
		return -1;
	}

	return 0;
}

static int align_entry_file_backward(FILE* fp){
	char tm_buf[TIME_T_B16_LEN];
	int c;
	do{
		fseek(fp, -2, SEEK_CUR);
	}while (fgetc(fp) != '\0' && ftell(fp) > 1);

	do{
		fseek(fp, -2, SEEK_CUR);
	}while (fgetc(fp) != '\0' && ftell(fp) > 1);


	if (ftell(fp) <= 1){
		while ((c = fgetc(fp)) != '\0' && c != EOF);
		if (c == EOF){
			log_error("Invalid file format");
			return -1;
		}
	}

	if (fread(tm_buf, 1, sizeof(tm_buf), fp) != sizeof(tm_buf)){
		log_error("Failed to read file");
		return -1;
	}

	return 0;
}

EZB_INLINE EZB_HOT EZB_PURE static int compare_entries(const struct entry* e1, const struct entry* e2){
	if (!e1){
		return 1;
	}
	if (!e2){
		return -1;
	}
	return strcmp(e1->file, e2->file);
}

EZB_INLINE EZB_HOT static void swap(struct entry** e1, struct entry** e2){
	struct entry* buf = *e1;
	*e1 = *e2;
	*e2 = buf;
}

EZB_INLINE static int median_of_three(struct entry** entries, int low, int high){
	int left = low;
	int mid = (high - low) / 2;
	int right = high;

	if (high - low < 3){
		return low;
	}

	if (compare_entries(entries[left], entries[mid]) > 0){
		if (compare_entries(entries[mid], entries[right]) > 0){
			return mid;
		}
		else if (compare_entries(entries[right], entries[left]) > 0){
			return left;
		}
		else{
			return right;
		}
	}
	else{
		if (compare_entries(entries[mid], entries[right]) < 0){
			return mid;
		}
		else if (compare_entries(entries[right], entries[left]) < 0){
			return left;
		}
		else{
			return right;
		}
	}
}

EZB_INLINE static int partition_m3(struct entry** entries, int low, int high){
	struct entry* pivot;
	int i, j;
	int m3 = median_of_three(entries, low, high);

	swap(&(entries[m3]), &(entries[high]));
	pivot = entries[high];

	for (i = low - 1, j = low; j <= high - 1; ++j){
		if (compare_entries(entries[j], pivot) < 0){
			i++;
			swap(&(entries[i]), &(entries[j]));
		}
	}
	swap(&(entries[i + 1]), &(entries[high]));
	return i + 1;
}

static void quicksort_entries(struct entry** entries, int low, int high){
	if (low < high){
		int pivot = partition_m3(entries, low, high);
		quicksort_entries(entries, low, pivot - 1);
		quicksort_entries(entries, pivot + 1, high);
	}
}

static int create_run(FILE* fp_in, struct TMPFILE** out){
	struct entry_array* arr = entry_arr_new();
	struct TMPFILE* tfp = NULL;
	int res;
	size_t total_len = 0;
	size_t i;
	int ret = 0;

	if (!arr){
		log_error("Failed to create entry array");
		ret = -1;
		goto cleanup;
	}

	do{
		struct entry* e = NULL;

		res = read_entry(fp_in, &e);
		if (res > 0){
			log_info("Input mtime file reached EOF");
			ret = -1;
			goto cleanup;
		}
		else if (res < 0){
			log_error("Failed to read from input mtime file");
			ret = -1;
			goto cleanup;
		}

		total_len += strlen(e->file) + 1 + sizeof(time_t) + 1;

		if (entry_arr_add(arr, e) != 0){
			log_error("Failed to add entry to array");
			free_entry(e);
			ret = -1;
			goto cleanup;
		}
	}while (total_len < MAX_RUN_LEN);

	quicksort_entries(arr->entries, 0, arr->size - 1);

	tfp = temp_fopen();
	if (!tfp){
		log_error("Failed to create temporary file");
		ret = -1;
		goto cleanup;
	}

	for (i = 0; i < arr->size; ++i){
		if (write_entry(arr->entries[i], tfp->fp) != 0){
			log_error("Failed to write entry to file");
			ret = -1;
			goto cleanup;
		}
	}

cleanup:
	if (ret == 0){
		*out = tfp;
	}
	else{
		*out = NULL;
		temp_fclose(tfp);
	}
	entry_arr_free(arr);
	return 0;
}

static int create_initial_runs(const char* file_in, struct TMPFILE*** out, size_t* n_out){
	FILE* fp_in = NULL;
	struct TMPFILE** tfp_arr = NULL;
	size_t arr_len = 0;
	int res;
	int ret = 0;

	fp_in = fopen(file_in, "rb");
	if (!fp_in){
		ret = -1;
		goto cleanup;
	}

	do{
		struct TMPFILE* tfp_tmp;
		void* tmp;
		arr_len++;
		tmp = realloc(tfp_arr, arr_len * sizeof(*tfp_arr));
		if (!tmp){
			log_error("Failed to increase length of temporary file array.");
			ret = -1;
			goto cleanup;
		}
		res = create_run(fp_in, &tfp_tmp);
		if (res < 0){
			log_error("Failed to create run.");
			ret = -1;
			goto cleanup;
		}
	}while (res == 0);

cleanup:
	if (ret == 0){
		*out = tfp_arr;
		*n_out = arr_len;
	}
	else{
		size_t i;
		*out = NULL;
		*n_out = 0;
		for (i = 0; i < arr_len; ++i){
			temp_fclose(tfp_arr[i]);
		}
		free(tfp_arr);
	}
	fp_in ? fclose(fp_in) : 0;
}

EZB_INLINE static void minheapify(struct minheapnode* entries, size_t entries_len, size_t index){
	size_t smallest = index;
	size_t left = 2 * index + 1;
	size_t right = 2 * index + 2;

	/* find the smallest element out of the current entry and its children */
	if (left < entries_len && compare_entries(entries[left].e, entries[smallest].e) < 0){
		smallest = left;
	}

	if (right < entries_len && compare_entries(entries[right].e, entries[smallest].e) < 0){
		smallest = right;
	}

	/* if one of the children is smaller than the current entry */
	if (smallest != index){
		/* swap it so the smallest is at the top */
		struct minheapnode tmp = entries[index];
		entries[index] = entries[smallest];
		entries[smallest] = tmp;
		/* recursively heapify the element we just swapped */
		minheapify(entries, entries_len, smallest);
	}
}

static int merge_files(struct TMPFILE** in, size_t n_files, const char* file_out){
	struct minheapnode* minheap = NULL;
	size_t minheap_len = 0;
	size_t n_empty = 0;
	FILE* fp_out = NULL;
	size_t i;
	int ret = 0;

	fp_out = fopen(file_out, "wb");
	if (!fp_out){
		ret = -1;
		goto cleanup;
	}

	minheap = malloc(n_files * sizeof(*minheap));
	if (!minheap){
		log_enomem();
		ret = -1;
		goto cleanup;
	}

	for (i = 0; i < n_files; ++i){
		rewind(in[i]->fp);
		if (read_entry(in[i]->fp, &(minheap[i].e)) != 0){
			log_error("Failed to read entry from file");
			ret = -1;
			goto cleanup;
		}
		minheap[i].i = i;
		minheap_len++;
	}

	/* balance our heap */
	for (i = minheap_len / 2 - 1; i >= 0; --i){
		minheapify(minheap, minheap_len, i);
	}

	while (n_empty < minheap_len){
		if (write_entry(minheap[0].e, fp_out) != 0){
			log_error("Failed to write entry to file");
			ret = -1;
			goto cleanup;
		}
		free_entry(minheap[0].e);

		if (read_entry(in[minheap[0].i]->fp, &(minheap[0].e)) != 0){
			log_error("Failed to read entry from file");
			ret = -1;
			goto cleanup;
		}

		if (!(minheap[0].e)){
			n_empty++;
		}

		minheapify(minheap, minheap_len, 0);
	}

cleanup:
	for (i = 0; i < minheap_len; ++i){
		free_entry(minheap[i].e);
	}
	free(minheap);
	fp_out ? fclose(fp_out) : 0;
	return ret;
}

int mtime_sort(const char* file){
	struct TMPFILE* tfp = NULL;
	struct TMPFILE** tfp_runs_array = NULL;
	size_t tfp_runs_array_len;
	int ret = 0;

	tfp = temp_fopen();
	if (!tfp){
		log_error("Failed to create temporary file");
		ret = -1;
		goto cleanup;
	}

	if (rename_file(file, tfp->name) != 0){
		log_error("Failed to move file to temporary location");
		ret = -1;
		goto cleanup;
	}

	if (create_initial_runs(tfp->name, &tfp_runs_array, &tfp_runs_array_len) != 0){
		log_error("Failed to create runs for merging");
		ret = -1;
		goto cleanup;
	}

	if (merge_files(tfp_runs_array, tfp_runs_array_len, file) != 0){
		log_error("Faled to merge files");
		ret = -1;
		goto cleanup;
	}

cleanup:
	if (ret != 0 && tfp){
		rename_file(tfp->name, file);
	}
	temp_fclose(tfp);
}

int mtime_search(FILE* fp, const char* key, time_t* mtime_out){
	struct entry* e;
	uint64_t size;
	uint64_t left;
	uint64_t right;
	const int64_t end_bsearch_threshold = 512;
	int res;

	if (get_file_size_fp(fp, &size) != 0){
		log_error("Failed to get the size of the file");
		return -1;
	}
	left = 0;
	right = size - 1;

	/* binsearch until the gap is less than end_bsearch_threshold */
	while ((right - left) >= end_bsearch_threshold){
		uint64_t mid = (left + right) / 2;
		int res;

		fseek(fp, mid, SEEK_SET);
		res = align_entry_file_forward(fp);
		if (res < 0){
			log_error("Failed to align entry file");
			return -1;
		}
		else if (res > 0){
			log_warning("Reached EOF during binsearch function. Possibly a bug");
			return -1;
		}

		if (read_entry(fp, &e) != 0){
			log_error("Failed to read entry from mtime file");
			return -1;
		}

		res = strcmp(key, e->file);
		if (res == 0){
			*mtime_out = e->mtime;
			free_entry(e);
			return 0;
		}
		else if (res < 0){
			right = mid - 1;
		}
		else{
			left = mid + 1;
		}
		free_entry(e);
	}

	if (ftell(fp) < end_bsearch_threshold){
		fseek(fp, 0, SEEK_SET);
	}
	else{
		fseek(fp, -end_bsearch_threshold - 1, SEEK_CUR);
	}
	if (align_entry_file_backward(fp) != 0){
		log_error("Failed to align mtime file");
		return -1;
	}

	do{
		if (read_entry(fp, &e) != 0){
			log_error("Failed to read entry from mtime file");
			return -1;
		}
		res = strcmp(e->file, key);
		if (res == 0){
			*mtime_out = e->mtime;
			free_entry(e);
			return 0;
		}
		free_entry(e);
		/* while e->file is less than the key */
	}while (res < 0);

	return 1;
}

int mtime_write(const char* file, time_t mtime, FILE* fp){
	struct entry e;
	e.file = (char*)file;
	e.mtime = mtime;
	return write_entry(&e, fp);
}

int mtime_read(FILE* fp_mtime, char** file_out, time_t* mtime_out){
	struct entry* e;
	int res = read_entry(fp_mtime, &e);

	if (res < 0){
		log_error("Error reading mtime entry file");
		return -1;
	}
	else if (res > 0){
		log_info("mtime_read() reached EOF");
		return 0;
	}

	if (file_out){
		*file_out = e->file;
	}
	if (mtime_out){
		*mtime_out = e->mtime;
	}

	free(e);
	return 0;
}
