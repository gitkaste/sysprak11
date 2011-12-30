DIRS = $(shell find .. -mindepth 1 -type d)
include ../Makefile

.PRECIOUS: test
test: $(OBJS) test.o
	$(CC) -o $@ $^
	./$@ 
