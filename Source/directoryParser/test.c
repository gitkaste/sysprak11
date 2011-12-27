#include "directoryParser.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char * argv[]){
	if (argc < 2) {
		puts("Need more Arguments");
		exit(1);
	}
	return parseDirToFD(STDOUT_FILENO, argv[1], ".");
}
