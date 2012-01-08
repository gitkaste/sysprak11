#define _XOPEN_SOURCE 600 //http://compgroups.net/comp.unix.programmer/_SVID_SOURCE-and-XOPEN_SOURCE
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

struct array *initArray(size_t itemsize, size_t initial_size, int shmid){
	struct array * arr;
	int totalmemsize = sizeof(struct array) + initial_size;
	if (shmid == -1){
		arr = malloc(totalmemsize);
		if (!arr) return arr;
	} else {
		arr = shmat(shmid, NULL, 0);
		if (arr == (void *) -1) return NULL;
	}
//	fprintf(stderr, "arr: %p itemsize %d size %d shmid %d\n", (void *)arr, (int)itemsize, (int) initial_size, shmid);
	arr->mem = arr + sizeof(struct array);
//	fprintf(stderr, "arr: %p itemsize %d size %d shmid %d\n", (void *)arr, (int)itemsize, (int) initial_size, shmid);
//fflush(stderr);
	arr->memsize = initial_size;
	arr->itemsize = itemsize;
	arr->itemcount = 0;
	arr->shmid = shmid;
	return arr;
}

void freeArray(struct array *a){
	if (a->shmid ==-1){
		free(a);
	}else{
		/* no point in catching the return since we can't do anything with it */
		shmdt(a);
	}
}

struct array *addArrayItem (struct array *a, void *item){
	fprintf(stderr, "Array pointer %p\n", (void *)a);
	if (a->memsize <= a->itemsize * (a->itemcount + 1)){
		if (a->shmid == -1){
			void *newmem = realloc(a->mem, sizeof(struct array)+ a->itemsize*(a->itemcount + 10));
			/* the heaps too small - crap, nothing we can do */
			if (!newmem){
				return NULL;
			}else{
				//the allocation was moved to accomodate the new size
				if (newmem != a){
					a = newmem;
					a->mem = a + sizeof(struct array);
				}
				a->memsize = a->itemsize * (a->itemcount + 10);
			}
		}else{
			/* In case the shmem segment is too small, there is nothing we can do */
			return NULL;
		}
	}	
	a->itemcount++;
	return (memcpy((char *)a->mem + (a->itemcount-1) * a->itemsize, item, a->itemsize)) ? a : NULL;
}

int remArrayItem (struct array *a, unsigned long num){
	if (num > a->itemcount-1) return -1;
	//memmove(a->mem + (num*a->itemsize), a->mem + ((num + 1) * a->itemsize));
	memcpy((char *)a->mem + (num*a->itemsize), (char *)a->mem + (a->itemcount-1)*a->itemsize, a->itemsize);
	a->itemcount--;
	return 1;
}

void *getArrayItem(struct array *a, unsigned long num){
	if (num > a->itemcount-1) return NULL;
	return (char *)a->mem+ num * a->itemsize;
}

void *iterateArray(struct array *a, unsigned long *i){
	/* if it's the last item then it doens't matter if we inc i */
	return getArrayItem(a, (*i)++);
}
