DIRS = $(shell find .. -mindepth 1 -type d)

.PRECIOUS: client
client: $(OBJS) client.o
	$(CC) -o $@ $^
	#G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind -v --leak-check=full --log-file=./valgrindlog ./$@ client.conf
	./$@ -i 127.0.0.1 #client.conf

include ../Makefile

client: $(OBJS) client.o
