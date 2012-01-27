DIRS = $(shell find .. -mindepth 1 -maxdepth 1 -type d)
include ../Makefile

.PRECIOUS: server
server: $(OBJS) $(SERVEROBJS) server.o
	rm server.log	
	$(CC) -o $@ $^

test: server
	G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind -v --read-var-info=yes  --leak-check=full --show-reachable=yes --log-file=./valgrindlog ./server server.conf
	#./$@ server.conf
