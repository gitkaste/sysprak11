#include <unistd.h>
#include <fcntl.h> /* fcntl() */

int setFdNonblock(int fd) {
	int fdflags;
	
	if((fdflags = fcntl(fd, F_GETFL, 0)) == -1) return -1;
	if(fcntl(fd, F_SETFL, fdflags | O_NONBLOCK) == -1) return -1;
	
	return 1;
}
