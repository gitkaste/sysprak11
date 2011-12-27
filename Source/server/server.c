#include "writef.h"
#include "util.h"
#include <unistd.h>
#include <stdio.h>

#define LOG(x) do {puts(x); fflush(stdout);} while(0)
void test_util(){
	int fd = STDIN_FILENO; /*open(filename,mode);*/
	struct buffer buf;
	struct buffer buf2;
	createBuf(&buf, 256);
	LOG("created first buffer");
	createBuf(&buf2, 256);
	LOG("created second buffer");
	puts("Please provide input");
	readToBuf(fd, &buf);
	LOG("read input into buffer");
	copyBuf(&buf2, &buf);
	LOG("duplicated buf");
	writeBuf(STDOUT_FILENO, &buf);
	LOG("wrote buffer to output");
	writeBuf(STDOUT_FILENO, &buf);
	LOG("wrote empty buf");
	copyBuf(&buf, &buf2);
	writeBuf(STDOUT_FILENO, &buf2);
	LOG("wrote buf2");
	freeBuf(&buf2);
	LOG("freed buf2");
	writef(STDOUT_FILENO, "testing writef with 1st buffer: %s", buf.buf);
}
int main (){
	test_util();
	return 0;
}
