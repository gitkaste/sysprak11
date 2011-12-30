#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "config.h"

#define LOG(x) do {puts(x); fflush(stdout);} while(0)

int main(int argc, char * argv[]){
	if (argc < 2) {
		puts("Need more Arguments");
		exit(1);
	}
	struct config conf;
	confDefaults(&conf);
	LOG("done with defaulting");
	int fd = open (argv[1],O_RDONLY);
	parseConfig(fd, &conf);
	fflush(stdout);
	writeConfig(1, &conf);
}
