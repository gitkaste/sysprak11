#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LOG(x) do {puts(x); fflush(stdout);} while(0)

int main(int argc, char * argv[]){
	unsigned long i;
	unsigned long * pi;
	unsigned long counter;

	struct array *a = initArray(sizeof(i), sizeof(i) * 8, -1);
	LOG("created array");

	for (counter = 1; counter< 7; counter++){
		if (!addArrayItem( a, &counter)){
			printf("failed adding %ldth item", counter);
			break;
		}
	}
	
	LOG("done adding safe elements");
	counter=0;
	while( (pi = iterateArray(a, &counter)) ){
		printf("\t%ld: %lu\n", counter, *pi);
		fflush(stdout);
	}	i = 7;
	pi = (unsigned long *) addArrayItem(a, (void *)&i);
	printf("pointer of seventh elmeent %p\n", (void *) pi);
	fflush(stdout);
	counter=0;
	while( (pi = iterateArray(a, &counter)) ){
		printf("\t%ld: %lu\n", counter, *pi);
		fflush(stdout);
	}
	pi = (unsigned long *) getArrayItem(a,3);
	printf("fourth element: %lu\n", *pi);
	LOG("removed fourth element...");
	printf("%ssuccessfully\n", (remArrayItem(a,3)==1)? "": "un");
	fflush(stdout);
	counter =0;
	while( (pi = iterateArray(a, &counter)) ){
		printf("\t%ld: %lu\n", counter, *pi);
		fflush(stdout);
	}
	freeArray(a);
	LOG("freed Array");
}
