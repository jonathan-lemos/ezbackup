/* checksumsort.c -- external sorting of checksum file
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/* prototypes */
#include "checksumsort.h"
/* errors */
#include "error.h"
#include <errno.h>
/* reading them files */
#include "readfile.h"
/* strcmp */
#include <string.h>
/* malloc */
#include <stdlib.h>
/* assert */
#include <assert.h>
/* file size */
#include <sys/stat.h>
/* increasing file limit */
#include <sys/resource.h>
#include <unistd.h>

#define MAX_RUN_SIZE (1 << 24)

void free_element(element* e){
	free(e->file);
	free(e->checksum);
	free(e);
}

static int compare_elements(element* e1, element* e2){
	/* send NULL's to bottom of heap */
	if  (!e1){
		return 1;
	}
	if (!e2){
		return -1;
	}

	return strcmp(e1->file, e2->file);
}

/* format: <file>\0<checksum>\n */
int write_element_to_file(FILE* fp, element* e){
	if (!fp || !e || !e->file || !e->checksum){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	if (!file_opened_for_writing(fp)){
		log_error(__FL__, STR_EMODE);
		return -1;
	}

	fprintf(fp, "%s%c%s\n", e->file, '\0', e->checksum);
	if (ferror(fp)){
		log_error(__FL__, STR_EFWRITE, "checksum file");
		return -1;
	}
	return 0;
}

element* get_next_checksum_element(FILE* fp){
	long pos_origin;
	long pos_file;
	long pos_checksum;
	int c;
	size_t len_file;
	size_t len_checksum;
	element* e;

	if (!fp){
		log_error(__FL__, STR_ENULL);
		return NULL;
	}

	if (!file_opened_for_reading(fp)){
		log_error(__FL__, STR_EMODE);
		return NULL;
	}

	e = malloc(sizeof(*e));
	if (!e){
		log_fatal(__FL__, STR_ENOMEM);
		return NULL;
	}

	pos_origin = ftell(fp);
	/* read an \0 */
	while ((c = fgetc(fp)) != '\0'){
		if (ferror(fp)){
			log_error(__FL__, STR_EFREAD, "file");
			free(e);
			return NULL;
		}
		/* end of file, no more checksums to read */
		if (c == EOF){
			log_debug(__FL__, "get_next_checksum_element(): reached EOF");
			free(e);
			return NULL;
		}
	}
	pos_file = ftell(fp);
	while ((c = fgetc(fp)) != '\n'){
		if (ferror(fp)){
			log_error(__FL__, STR_EFREAD, "file");
			free(e);
			return NULL;
		}
		if (c == EOF){
			log_debug(__FL__, "get_next_checksum_element(): reached EOF");
			free(e);
			return NULL;
		}
	}
	pos_checksum = ftell(fp);
	/* calculate length of file + checksum */
	len_file = pos_file - pos_origin;
	len_checksum = pos_checksum - pos_file;

	/* make space for file + checksum */
	e->file = malloc(len_file);
	if (!e->file){
		log_fatal(__FL__, STR_ENOMEM);
		free(e);
		return NULL;
	}
	e->checksum = malloc(len_checksum);
	if (!e){
		log_fatal(__FL__, STR_ENOMEM);
		free(e->file);
		free(e);
		return NULL;
	}

	/* go back to where we started */
	fseek(fp, pos_origin, SEEK_SET);
	/* and read <length of file> bytes (includes \0)*/
	fread(e->file, 1, len_file, fp);

	/* and read <length of checksum> bytes (includes \0)*/
	fread(e->checksum, 1, len_checksum, fp);
	e->checksum[len_checksum - 1] = '\0';

	if (ferror(fp)){
		log_error(__FL__, STR_EFREAD, "file");
		free_element(e);
		e = NULL;
	}

	return e;
}

static void swap(element** e1, element** e2){
	element* buf = *e1;
	*e1 = *e2;
	*e2 = buf;
}

static void swap_mhn(minheapnode* mhn1, minheapnode* mhn2){
	minheapnode tmp = *mhn1;
	*mhn1 = *mhn2;
	*mhn2 = tmp;
}

/* chooses the median of the left element, the right element,
 * and the middle element.
 *
 * this makes choosing a bad pivot much less likely */
int median_of_three(element** elements, int low, int high){
	int left = low;
	int mid = (high - low) / 2;
	int right = high;

	if (high - low < 3){
		return low;
	}

	if (compare_elements(elements[left], elements[mid]) > 0){
		if (compare_elements(elements[mid], elements[right]) > 0){
			return mid;
		}
		else if (compare_elements(elements[right], elements[left]) > 0){
			return left;
		}
		else{
			return right;
		}
	}
	else{
		if (compare_elements(elements[mid], elements[right]) < 0){
			return mid;
		}
		else if (compare_elements(elements[right], elements[left]) < 0){
			return left;
		}
		else{
			return right;
		}
	}
}

/* partitions the array by putting all the elements on the
 * correct side of the pivot
 *
 * pivot must be the high element */
static int partition(element** elements, int low, int high){
	element* pivot = elements[high];
	int i;
	int j;

	for (i = low - 1, j = low; j <= high - 1; ++j){
		if (compare_elements(elements[j], pivot) < 0){
			i++;
			swap(&elements[i], &elements[j]);
		}
	}
	swap(&elements[i + 1], &elements[high]);
	return i + 1;
}

/* swaps the median of three element with the top element,
 * so it works with the above function */
static int partition_m3(element** elements, int low, int high){
	int m3 = median_of_three(elements, low, high);
	swap(&elements[m3], &elements[high]);
	return partition(elements, low, high);
}

/* main quicksort function for sorting the elements */
void quicksort_elements(element** elements, int low, int high){
	if (low < high){
		int pivot = partition_m3(elements, low, high);
		quicksort_elements(elements, low, pivot - 1);
		quicksort_elements(elements, pivot + 1, high);
	}
}

void free_element_array(element** elements, size_t size){
	size_t i;
	for (i = 0; i < size; ++i){
		free_element(elements[i]);
	}
	free(elements);
}

static void free_minheapnodes(minheapnode* mhn, size_t size){
	size_t i;
	for (i = 0; i < size; ++i){
		if (mhn[i].e){
			free_element(mhn[i].e);
		}
	}
	free(mhn);
}

void free_strarray(char** elements, size_t size){
	size_t i;
	for (i = 0; i < size; ++i){
		free(elements[i]);
	}
	free(elements);
}

void free_filearray(FILE** elements, size_t size){
	size_t i;
	for (i = 0; i < size; ++i){
		if (fclose(elements[i]) != 0){
			log_error(__FL__, STR_EFCLOSE, "file");
		}
	}
	free(elements);
}

static __off_t get_file_size(FILE* fp){
	int fd;
	struct stat st;

	fd = fileno(fp);

	fstat(fd, &st);
	return st.st_size;
}

static int set_file_limit(int num){
	struct rlimit rl;
	rl.rlim_cur = num + 1;
	rl.rlim_max = num + 1;
	return setrlimit(RLIMIT_NOFILE, &rl);
}

/* you know this is going to be good when there's a triple pointer */

/* reads MAX_RUN_SIZE bytes worth of elements into ram,
 * sorts them, and writes them to one or more files */
int create_initial_runs(FILE* fp_in, FILE*** out, size_t* n_files){
	element** elems = NULL;
	element* tmp = NULL;
	int elems_len = 0;
	int total_len = 0;
	int end_of_file = 0;
	int lim;

	/* check null arguments */
	if (!fp_in || !out || !n_files){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	if (!file_opened_for_reading(fp_in)){
		log_error(__FL__, STR_EMODE);
		return -1;
	}
	rewind(fp_in);

	lim = get_file_size(fp_in) / MAX_RUN_SIZE + 1024;

	if (set_file_limit(lim) != 0){
		log_warning(__FL__, "Could not set max file limit (%s)", strerror(errno));
	}

	*out = NULL;
	*n_files = 0;

	while (!end_of_file){
		/* /var/tmp is mounted to disk, not RAM */
		FILE* fp;
		char template_merge[] = "/var/tmp/merge_XXXXXX";
		int i;

		/* make a new temp file */
		(*n_files)++;
		/* make space for new string */
		*out = realloc(*out, sizeof(**out) * *n_files);
		if (!*out){
			log_fatal(__FL__, STR_ENOMEM);
			return -1;
		}
		if ((fp = temp_fopen(template_merge)) == NULL){
			log_error(__FL__, "Failed to create temporary merge file");
			(*n_files)--;
			*out = realloc(*out, *n_files * sizeof(**out));
			return -1;
		}
		(*out)[*n_files - 1] = fp;

		/* auto remove merge file when fp closes */
		if (unlink(template_merge) != 0){
			log_warning(__FL__, "Failed to unlink %s (%s)", template_merge, strerror(errno));
		}

		/* read enough elements to fill MAX_RUN_SIZE */
		/* TODO: this reads one element above MAX_RUN_SIZE
		 * make it not do that */
		while (total_len < MAX_RUN_SIZE &&
				(tmp = get_next_checksum_element(fp_in)) != NULL){
			elems_len++;
			elems = realloc(elems, sizeof(*elems) * elems_len);
			if (!elems){
				log_fatal(__FL__, STR_ENOMEM);
				return -1;
			}
			/* +2 for the 2 \0's */
			total_len += strlen(tmp->file) + strlen(tmp->checksum) + 2;
			elems[elems_len - 1] = tmp;
		}
		/* if we didn't read any elements */
		if (!elems_len){
			/* we're at the end of the file */
			fclose(fp);
			(*n_files)--;
			*out = realloc(*out, *n_files * sizeof(**out));
			if (!(*out)){
				log_fatal(__FL__, STR_ENOMEM);
				return -1;
			}
			end_of_file = 1;
			continue;
		}
		/* sort elements */
		quicksort_elements(elems, 0, elems_len - 1);
		/* write them to the file */
		for (i = 0; i < elems_len; ++i){
			if (write_element_to_file(fp, elems[i]) != 0){
				log_debug(__FL__, "Failed to write element to file");
			}
		}
		if (fflush(fp) != 0){
			log_warning(__FL__, "Failed to flush merge file");
		}
		/* cleanup */
		free_element_array(elems, elems_len);
		elems = NULL;
		elems_len = 0;
		total_len = 0;
		if (!tmp){
			end_of_file = 1;
		}
	}
	return 0;
}

/* creates a min heap based on the elements */
/* this is used so we can merge the files in O(nlogn)
 * instead of O(n^2) by making a priority queue*/
void minheapify(minheapnode* elements, int elements_len, int index){
	int smallest = index;
	int left = 2 * index + 1;
	int right = 2 * index + 2;

	/* if left child is smaller than root */
	if (left < elements_len && compare_elements(elements[left].e, elements[smallest].e) < 0){
		smallest = left;
	}

	/* if right child is smaller than smallest so far */
	if (right < elements_len && compare_elements(elements[right].e, elements[smallest].e) < 0){
		smallest = right;
	}

	/* if smallest is not root */
	if (smallest != index){
		/* get the smallest to root */
		swap_mhn(&elements[index], &elements[smallest]);
		/* recursively balance the unbalanced heap */
		minheapify(elements, elements_len, smallest);
	}
}

/* merges the initial runs into one big file */
int merge_files(FILE** in, size_t n_files, FILE* fp_out){
	minheapnode* mhn = NULL;
	size_t count = 0;
	size_t i;
	int j;

	/* verify that arguments are not null */
	if (!in || !fp_out){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	if (!file_opened_for_writing(fp_out)){
		log_error(__FL__, STR_EMODE);
		return -1;
	}

	/* rewind input files so they can be used for reading */
	for (i = 0; i < n_files; ++i){
		rewind(in[i]);
	}

	/* make space for the min heap nodes */
	mhn = malloc(n_files * sizeof(*mhn));
	if (!mhn){
		log_fatal(__FL__, STR_ENOMEM);
		return -1;
	}
	/* get the first element from each file */
	for (i = 0; i < n_files; ++i){
		mhn[i].e = get_next_checksum_element(in[i]);
		mhn[i].i = i;
	}
	/* balance our heap */
	for (j = n_files / 2 - 1; j >= 0; --j){
		minheapify(mhn, n_files, j);
	}

	/* while the counter is less than the number of files */
	/* counter tracks which files are empty */
	while (count < n_files){
		/* write root element (smallest) to file */
		write_element_to_file(fp_out, mhn[0].e);
		free_element(mhn[0].e);

		/* replace root element with next from that file */
		mhn[0].e = get_next_checksum_element(in[mhn[0].i]);
		/* if file is empty */
		if (!mhn[0].e){
			/* raise the counter */
			count++;
		}
		/* rebalance our heap */
		minheapify(mhn, n_files, 0);
	}

	/* cleanup */
	free_minheapnodes(mhn, n_files);
	if (fflush(fp_out) != 0){
		log_warning(__FL__, "Failed to flush checksum file buffer. Data corruption possible.");
	}
	return 0;
}

int search_file(FILE* fp, const char* key, char** checksum){
	element* tmp;
	int c;
	int res;
	__off_t size;
	__off_t low;
	__off_t high;
	const int end_bsearch_threshold = 128;

	/* check null arguments */
	if (!fp || !key || !checksum){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	/* start at half of file */
	size = get_file_size(fp);
	low = 0;
	high = size - 1;

	/* 512 bytes or below switch to linear search
	 *
	 * since fseek is a rather slow operation,
	 * it's more efficient just to linearly search
	 * at that point */
	while ((high - low) >= end_bsearch_threshold){
		__off_t mid = (high + low) >> 1;
		/* go to pivot */
		fseek(fp, mid, SEEK_SET);
		fgetc(fp);
		/* move back to beginning of element */
		do{
			if (ftell(fp) <= 1){
				fseek(fp, 0, SEEK_SET);
				break;
			}
			else{
				fseek(fp, -2, SEEK_CUR);
			}
			c = fgetc(fp);
			if (ftell(fp) <= 0){
				break;
			}
		}while (c != '\n');

		if (ferror(fp) != 0){
			log_error(__FL__, STR_EFREAD);
			return -1;
		}

		/* get the element */
		tmp = get_next_checksum_element(fp);
		if (!tmp){
			log_info(__FL__, "get_next_checksum_element() returned NULL");
			return -1;
		}
		/* check if it matches our key */
		res = strcmp(key, tmp->file);
		if (res == 0){
			*checksum = malloc(strlen(tmp->checksum) + 1);
			if (!(*checksum)){
				log_fatal(__FL__, STR_ENOMEM);
				return -1;
			}
			strcpy(*checksum, tmp->checksum);
			free_element(tmp);
			return 0;
		}
		/* if key is before tmp */
		else if (res < 0){
			high = mid - 1;
		}
		else{
			low = mid + 1;
		}
		free_element(tmp);
	}

	/* go back 512 bytes or to beginning of file to make sure we don't miss anything */
	if (ftell(fp) < end_bsearch_threshold){
		fseek(fp, 0, SEEK_SET);
	}
	else{
		fseek(fp, -end_bsearch_threshold - 1, SEEK_CUR);
	}
	/* go to beginning of element */
	do{
		if (ftell(fp) <= 1){
			fseek(fp, 0, SEEK_SET);
			break;
		}
		else{
			fseek(fp, -2, SEEK_CUR);
		}
		c = fgetc(fp);
		if (ftell(fp) <= 0){
			break;
		}
	}while (c != '\n');
	/* linearly search for our key */
	do{
		tmp = get_next_checksum_element(fp);
		if (!tmp){
			res = 1;
			break;
		}
		res = strcmp(key, tmp->file);
		if (res == 0){
			break;
		}
		free_element(tmp);
		/* while key is greater than tmp */
	}while (res > 0);

	if (ferror(fp)){
		free_element(tmp);
		log_error(__FL__, STR_EFREAD, fp);
		return -1;
	}

	/* if we found our target */
	if (res == 0){
		/* return it */
		*checksum = malloc(strlen(tmp->checksum) + 1);
		if (!(*checksum)){
			log_fatal(__FL__, STR_ENOMEM);
			return -1;
		}
		strcpy(*checksum, tmp->checksum);
		free_element(tmp);
		return 0;
	}
	/* otherwise we failed */
	else{
		*checksum = NULL;
		return 1;
	}
}
