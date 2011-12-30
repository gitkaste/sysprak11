#include <unistd.h>
#include "logger.h"

int main(int argc, char * argv[]){
	logger(STDIN_FILENO, STDOUT_FILENO);
	logmsg (0, STDOUT_FILENO, 0, "%s %d %s täterä\n", "muh", 1, "maeh");
	return 0;
}
