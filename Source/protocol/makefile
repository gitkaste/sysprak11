DIRS = $(shell find .. -mindepth 1 -maxdepth 2 -type d)
include ../Makefile

.PRECIOUS: test
test: $(OBJS) test.o
	$(CC) -o $@ $^
	./$@ 
