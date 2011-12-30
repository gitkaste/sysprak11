DIRS = $(shell find .. -mindepth 1 -type d)
include ../Makefile

server: $(OBJS) server.o
	$(CC) -o $@ $^
	./$@ client.conf
