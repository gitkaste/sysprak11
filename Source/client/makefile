DIRS = $(shell find .. -mindepth 1 -type d)

.PRECIOUS: client
client:
	$(CC) -o $@ $^

test: client
	rm -f client.log
	G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind -v --leak-check=full --log-file=./valgrindlog ./client client.conf
	#./$@ -i 127.0.0.1 2> client.err

include ../Makefile

client: $(OBJS) $(CLIENTOBJS) client.o
