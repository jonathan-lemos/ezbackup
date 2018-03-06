#include "checksumsort.h"
#include "error.h"
#include "readfile.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_RUN_SIZE (1 << 24)

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

int write_element_to_file(FILE* fp, element* e){
	if (!fp || !e || !e->file || !e->checksum){
		return err_regularerror(ERR_ARGUMENT_NULL);
	}

	fprintf(fp, "%s%c%s%c", e->file, '\0', e->checksum, '\0');
	return 0;
}

element* get_next_checksum_element(FILE* fp){
	long pos_origin;
	long pos_file;
	long pos_checksum;
	long pos_temp;
	int c;
	int counter = 0;
	size_t len_file;
	size_t len_checksum;
	element* e;

	e = malloc(sizeof(*e));
	if (!fp || !e){
		return NULL;
	}

	pos_origin = ftell(fp);
	while (counter < 2){
		c = fgetc(fp);
		if (c == EOF){
			free(e);
			return NULL;
		}
		if (c == '\0'){
			counter++;
			if (counter & 1){
				pos_file = ftell(fp);
			}
			else{
				pos_checksum = ftell(fp);
			}
		}
	}
	len_file = pos_file - pos_origin;
	len_checksum = pos_checksum - pos_file;

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

	fseek(fp, pos_origin, SEEK_SET);
	fread(e->file, 1, len_file, fp);

	pos_temp = ftell(fp);
	assert(ftell(fp) == pos_temp);

	fread(e->checksum, 1, len_checksum, fp);

	return e;
}

element* get_checksum_element_index(FILE* fp, int index){
	long pos_origin;
	long pos_file;
	long pos_checksum;
	size_t len_file;
	size_t len_checksum;
	int c;
	int counter = 0;
	element* e;

	e = malloc(sizeof(*e));
	if (!fp || !e){
		return NULL;
	}

	rewind(fp);
	while (counter < 2 * index){
		c = fgetc(fp);
		if (c == EOF){
			break;
		}
		if (c == '\0'){
			counter++;
			if (!(counter & 1)){
				pos_origin = ftell(fp);
			}
		}
	}
	counter = 0;
	while (counter < 2){
		c = fgetc(fp);
		if (c == EOF){
			break;
		}
		if (c == '\0'){
			counter++;
			if (counter & 1){
				pos_file = ftell(fp);
			}
			else{
				pos_checksum = ftell(fp);
			}
		}
	}
	len_file = pos_file - pos_origin;
	len_checksum = pos_checksum - pos_file;

	e->file = malloc(len_file);
	if (!e->file){
		free(e);
		return NULL;
	}
	fseek(fp, pos_origin, SEEK_SET);
	fread(e->file, 1, len_file, fp);

	assert(pos_file == ftell(fp));
	e->checksum = malloc(len_checksum);
	if (!e->checksum){
		free(e->file);
		free(e);
		return NULL;
	}
	fread(e->checksum, 1, len_checksum, fp);
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

static int partition_m3(element** elements, int low, int high){
	int m3 = median_of_three(elements, low, high);
	swap(&elements[m3], &elements[high]);
	return partition(elements, low, high);
}

void quicksort_elements(element** elements, int low, int high){
	if (low < high){
		int pivot = partition_m3(elements, low, high);
		quicksort_elements(elements, low, pivot - 1);
		quicksort_elements(elements, pivot + 1, high);
	}
}

void free_elements(element** elements, size_t size){
	size_t i;
	for (i = 0; i < size; ++i){
		free(elements[i]->file);
		free(elements[i]->checksum);
	}
	free(elements);
}

void free_minheapnodes(minheapnode* mhn, size_t size){
	size_t i;
	for (i = 0; i < size; ++i){
		free(mhn[i].e->file);
		free(mhn[i].e->checksum);
		free(mhn[i].e);
	}
	free(mhn);
}

/*
static __off_t get_file_size(const char* file){
	struct stat st;

	lstat(file, &st);
	return st.st_size;
}
*/

/* you know this is going to be good when there's a triple pointer */
int create_initial_runs(const char* in_file, char*** out, size_t* n_files){
	FILE* fp;
	element** elems = NULL;
	int elems_len = 0;
	int total_len = 0;

	if (!in_file || !out || !n_files){
		return err_regularerror(ERR_ARGUMENT_NULL);
	}

	fp = fopen(in_file, "r");
	if (!fp){
		return err_regularerror(ERR_FILE_INPUT);
	}

	*out = NULL;
	*n_files = 0;

	while (!feof(fp)){
		/* /var/tmp is mounted to disk, not RAM */
		const char* template = "/var/tmp/merge_XXXXXX";
		char template_tmp[sizeof("/var/tmp/merge_XXXXXX")];
		FILE* fp;
		int i;

		/* make a new temp file */
		(*n_files)++;
		*out = realloc(*out, sizeof(FILE*));
		strcpy(template_tmp, template);
		if (!temp_file(template_tmp)){
			return err_regularerror(ERR_FILE_OUTPUT);
		}
		fp = fopen(template_tmp, "w");
		if (!fp){
			return err_regularerror(ERR_FILE_OUTPUT);
		}

		*out[*n_files - 1] = fp;

		/* read enough elements to fill MAX_RUN_SIZE */
		while (total_len < MAX_RUN_SIZE){
			elems_len++;
			elems = realloc(elems, sizeof(*elems) * elems_len);
			if (!elems){
				return err_regularerror(ERR_OUT_OF_MEMORY);
			}
			elems[elems_len - 1] = get_next_checksum_element(fp);
			if (!elems[elems_len - 1]){
				break;
			}

			/* +2 for the 2 \0's */
			total_len += strlen(elems[elems_len - 1]->file) + strlen(elems[elems_len - 1]->checksum) + 2;
		}

		/* sort elements */
		quicksort_elements(elems, 0, elems_len - 1);
		/* write them to the file */
		for (i = 0; i < elems_len; ++i){
			write_element_to_file(*out[*n_files - 1], elems[i]);
		}
		free_elements(elems, elems_len);
	}
	return 0;
}

void minheapify(minheapnode* elements, int elements_len, int index){
	int largest = elements_len;
	int left = 2 * index + 1;
	int right = 2 * index + 2;

	if (left < elements_len && compare_elements(elements[left].e, elements[largest].e) < 0){
		largest = left;
	}

	if (right < elements_len && compare_elements(elements[right].e, elements[largest].e) < 0){
		largest = right;
	}

	if (largest != index){
		swap_mhn(&elements[index], &elements[largest]);
		minheapify(elements, elements_len, largest);
	}
}

int merge_files(FILE** in, size_t n_files, const char* out_file){
	minheapnode* mhn = NULL;
	size_t count = 0;
	size_t i;
	int j;
	FILE* fp;

	if (!in || !n_files || !out_file){
		return err_regularerror(ERR_ARGUMENT_NULL);
	}

	fp = fopen(out_file, "w");
	if (!fp){
		return err_regularerror(ERR_FILE_OUTPUT);
	}

	mhn = malloc(n_files * sizeof(*mhn));
	if (!mhn){
		return err_regularerror(ERR_OUT_OF_MEMORY);
	}
	for (i = 0; i < n_files; ++i){
		mhn[i].e = get_next_checksum_element(in[i]);
		mhn[i].i = i;
	}
	for (j = n_files / 2 - 1; j >= 0; --j){
		minheapify(mhn, n_files, j);
	}

	while (count <= i){
		write_element_to_file(fp, mhn[0].e);
		mhn[0].e = get_next_checksum_element(in[i]);
		if (!mhn[0].e){
			count++;
		}
		minheapify(mhn, n_files, 0);
	}

	free_minheapnodes(mhn, n_files);
	return 0;
}

