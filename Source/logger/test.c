#include <unistd.h>
#include "logger.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

int main(int argc, char * argv[]){
	int semid = semCreate(1);
  int fd[2];

  if (-1 == pipe(fd) || semid == -1){
		perror("pipe");
		exit(1);
	}
  
  switch(fork()){
		case -1:
			perror("fork");
			exit(1);
		case 0:
			fputs("child\n",stderr);
			close(fd[0]);
			printf("%d\n", logmsg (semid, fd[1], 0, "%s %d %s täterä\n", "muh", 1, "maeh"));
			fputs("closing child\n",stderr);
			exit(0);
		default:
			fputs("father\n",stderr);
			close(fd[1]);
			logger(fd[0], STDOUT_FILENO);
			semClose(semid);
			fputs("closing father\n",stderr);
  }

	return 0;
}
