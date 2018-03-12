/* prototypes */
#include "checksumsort.h"
/* errors */
#include "error.h"
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

#define MAX_RUN_SIZE (1024)

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
	int c;

	if (!fp || !e || !e->file || !e->checksum){
		return err_regularerror(ERR_ARGUMENT_NULL);
	}

	c = fprintf(fp, "%s%c%s\n", e->file, '\0', e->checksum);
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
		return NULL;
	}

	e = malloc(sizeof(*e));
	if (!e){
		return NULL;
	}

	pos_origin = ftell(fp);
	/* read two \0's */
	while ((c = fgetc(fp)) != '\0'){
		/* end of file, no more checksums to read */
		if (c == EOF){
			free(e);
			return NULL;
		}
	}
	pos_file = ftell(fp);
	while ((c = fgetc(fp)) != '\n'){
		if (c == EOF){
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
		free(e);
		return NULL;
	}
	e->checksum = malloc(len_checksum);
	if (!e){
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

	return e;
}

void swap(element** e1, element** e2){
	element* buf = *e1;
	*e1 = *e2;
	*e2 = buf;
}

void swap_mhn(minheapnode* mhn1, minheapnode* mhn2){
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

void free_element(element* e){
	free(e->file);
	free(e->checksum);
	free(e);
}

void free_element_array(element** elements, size_t size){
	size_t i;
	for (i = 0; i < size; ++i){
		free_element(elements[i]);
	}
	free(elements);
}

void free_minheapnodes(minheapnode* mhn, size_t size){
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
		fclose(elements[i]);
	}
	free(elements);
}

/* you know this is going to be good when there's a triple pointer */

/* reads MAX_RUN_SIZE bytes worth of elements into ram,
 * sorts them, and writes them to a file */
/* out is char*** and not FILE***, since we need to reopen
 * them in reading mode and also remove() them when we're
 * done */
/* TODO: refactor */
int create_initial_runs(const char* in_file, char*** out, size_t* n_files){
	FILE* fp_in;
	element** elems = NULL;
	element* tmp = NULL;
	int elems_len = 0;
	int total_len = 0;
	int end_of_file = 0;

	if (!in_file || !out || !n_files){
		return err_regularerror(ERR_ARGUMENT_NULL);
	}

	fp_in = fopen(in_file, "r");
	if (!fp_in){
		return err_regularerror(ERR_FILE_INPUT);
	}

	*out = NULL;
	*n_files = 0;

	while (!end_of_file){
		/* /var/tmp is mounted to disk, not RAM */
		const char* template = "/var/tmp/merge_XXXXXX";
		FILE* fp;
		int i;

		/* make a new temp file */
		(*n_files)++;
		/* make space for new string */
		*out = realloc(*out, sizeof(**out) * *n_files);
		if (!*out){
			return err_regularerror(ERR_OUT_OF_MEMORY);
		}
		(*out)[*n_files - 1] = malloc(sizeof("/var/tmp/merge_XXXXXX"));
		if (!(*out)[*n_files - 1]){
			return err_regularerror(ERR_OUT_OF_MEMORY);
		}
		/* copy template to new string */
		strcpy((*out)[*n_files - 1], template);
		/* make a temp file using the template */
		/* new string now should be a valid temp file */
		if (temp_file((*out)[*n_files - 1]) != 0){
			return err_regularerror(ERR_FILE_OUTPUT);
		}
		fp = fopen((*out)[*n_files - 1], "w");
		if (!fp){
			return err_regularerror(ERR_FILE_OUTPUT);
		}

		/* read enough elements to fill MAX_RUN_SIZE */
		/* TODO: this reads one element above MAX_RUN_SIZE
		 * make it not do that */
		while (total_len < MAX_RUN_SIZE &&
				(tmp = get_next_checksum_element(fp_in)) != NULL){
			elems_len++;
			elems = realloc(elems, sizeof(*elems) * elems_len);
			if (!elems){
				return err_regularerror(ERR_OUT_OF_MEMORY);
			}
			/* +2 for the 2 \0's */
			total_len += strlen(tmp->file) + strlen(tmp->checksum) + 2;
			elems[elems_len - 1] = tmp;
		}
		if (!elems_len){
			fclose(fp);
			remove((*out)[(*n_files) - 1]);
			free((*out)[(*n_files) - 1]);
			(*n_files)--;
			*out = realloc(*out, *n_files * sizeof(**out));
			if (!(*out)){
				return err_regularerror(ERR_OUT_OF_MEMORY);
			}
			end_of_file = 1;
			continue;
		}
		/* sort elements */
		quicksort_elements(elems, 0, elems_len - 1);
		/* write them to the file */
		for (i = 0; i < elems_len; ++i){
			write_element_to_file(fp, elems[i]);
		}
		/* cleanup */
		free_element_array(elems, elems_len);
		fclose(fp);
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
int merge_files(char** files, size_t n_files, const char* out_file){
	minheapnode* mhn = NULL;
	size_t count = 0;
	size_t i;
	int j;
	FILE** in;
	FILE* fp_out;

	/* verify that arguments are not null */
	if (!files || !out_file){
		return err_regularerror(ERR_ARGUMENT_NULL);
	}

	/* open out_file for writing */
	fp_out = fopen(out_file, "wb");
	if (!fp_out){
		return err_regularerror(ERR_FILE_OUTPUT);
	}

	/* allocate space for our file array */
	in = malloc(sizeof(*in) * n_files);
	if (!in){
		return err_regularerror(ERR_OUT_OF_MEMORY);
	}

	/* open each char* as a file* */
	for (i = 0; i < n_files; ++i){
		in[i] = fopen(files[i], "rb");
		if (!in[i]){
			return err_regularerror(ERR_FILE_INPUT);
		}
	}

	/* make space for the min heap nodes */
	mhn = malloc(n_files * sizeof(*mhn));
	if (!mhn){
		return err_regularerror(ERR_OUT_OF_MEMORY);
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
	free_filearray(in, n_files);
	fclose(fp_out);
	return 0;
}

static __off_t get_file_size(const char* file){
	struct stat st;

	lstat(file, &st);
	return st.st_size;
}

int search_file(const char* file, const char* key, char** checksum){
	element* tmp;
	FILE* fp;
	__off_t pivot;
	int c;
	int size;
	int res;
	const int end_bsearch_threshold = 16;

	/* check null arguments */
	if (!file || !key || !checksum){
		return err_regularerror(ERR_ARGUMENT_NULL);
	}

	/* open file for reading */
	fp = fopen(file, "rb");
	if (!fp){
		return err_regularerror(ERR_FILE_INPUT);
	}

	/* start at half of file */
	size = get_file_size(file);
	pivot = get_file_size(file) / 2;

	/* 512 bytes or below switch to linear search
	 *
	 * since fseek is a rather slow operation,
	 * it's more efficient just to linearly search
	 * at that point */
	while (size >= end_bsearch_threshold){
		/* go to pivot */
		fseek(fp, pivot, SEEK_SET);
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

		/* get the element */
		tmp = get_next_checksum_element(fp);
		if (!tmp){
			return err_regularerror(ERR_FILE_INPUT);
		}
		/* check if it matches our key */
		res = strcmp(key, tmp->file);
		if (res == 0){
			*checksum = malloc(strlen(tmp->checksum) + 1);
			if (!(*checksum)){
				return err_regularerror(ERR_OUT_OF_MEMORY);
			}
			strcpy(*checksum, tmp->checksum);
			free_element(tmp);
			return 0;
		}
		/* if key is before tmp */
		else if (res < 0){
			/* go to half of left */
			size /= 2;
			pivot = size / 2;
		}
		else{
			/* otherwise go to half of right */
			size /= 2;
			pivot += size / 2;
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
	/* if we found our target */
	if (res == 0){
		/* return it */
		*checksum = malloc(strlen(tmp->checksum) + 1);
		if (!(*checksum)){
			return err_regularerror(ERR_OUT_OF_MEMORY);
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
