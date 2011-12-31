DIRS = $(shell find .. -mindepth 1 -type d)
include ../Makefile

client: $(OBJS) client.o
	$(CC) -o $@ $^
	./$@ client.conf
