DIRS = $(shell find .. -mindepth 1 -type d)
include ../Makefile

.PRECIOUS: server
server: $(OBJS) server.o
	echo $(OBJS)
	$(CC) -o $@ $^
	#G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind -v --leak-check=full --log-file=./valgrindlog ./$@ server.conf
	./$@ server.conf
