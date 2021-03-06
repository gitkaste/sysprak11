#define _XOPEN_SOURCE 600 //http://compgroups.net/comp.unix.programmer/_SVID_SOURCE-and-XOPEN_SOURCE
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

struct array *initArray(size_t itemsize, size_t initial_size, int shmid){
	struct array * arr;
	int totalmemsize = sizeof(struct array) + initial_size;
	fflush(stderr);
	if (shmid == -1){
		arr = malloc(totalmemsize);
		if (!arr) return arr;
	} else {
		arr = shmat(shmid, NULL, 0);
		if (arr == (void *) -1) return NULL;
	}
	arr->mem = (char *)arr + sizeof(struct array);
	arr->memsize = initial_size;
	arr->itemsize = (int)itemsize;
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

void clearArray(struct array *a){
	a->itemcount = 0;
	memset(a->mem, 0, a->memsize);
}

struct array *addArrayItem (struct array *a, void *item){
	if (a->memsize <= a->itemsize * (a->itemcount + 1)){
		if (a->shmid == -1){
			char *newmem=realloc(a, sizeof(struct array)+a->itemsize*(a->itemcount + 10));
			/* the heaps too small - crap, nothing we can do */
			if (!newmem){
				return NULL;
			}else{
				//the allocation was moved to accomodate the new size
				if (newmem != (char *)a){
					a = (struct array *)newmem;
					a->mem = (char *)a + sizeof(struct array);
				}
				a->memsize += a->itemsize * 10;
			}
		} else {
			/* In case the shmem segment is too small, there is nothing we can do */
			return NULL;
		}
	}	
	memmove((char *)a->mem + (a->itemcount * a->itemsize), item, a->itemsize);
	a->itemcount++;
	return a;
}

int remArrayItem (struct array *a, unsigned long num){
	if (num > a->itemcount-1) return -1;
	//memmove(a->mem + (num*a->itemsize), a->mem + ((num + 1) * a->itemsize));
	memcpy((char *)a->mem + (num*a->itemsize), (char *)a->mem + (a->itemcount-1)*a->itemsize, a->itemsize);
	a->itemcount--;
	return 1;
}

void *getArrayItem(struct array *a, unsigned long num) {
	if (!a->itemcount || num > a->itemcount-1) {
		return NULL;
	}
	return (char *)a->mem + (num * a->itemsize);
}

void *iterateArray(struct array *a, unsigned long *i) {
	/* if it's the last item then it doesn't matter if we inc i */
	return getArrayItem(a, (*i)++);
}
