CC = gcc
CFLAGS = -Wall -pedantic -Werror -std=gnu99

DIRS ?= $(shell find . -mindepth 1 -maxdepth 1 -type d)
MAKEFILES = $(foreach dir, $(DIRS), $(dir)/Makefile)
include $(MAKEFILES)

VPATH = $(DIRS)

INCLUDES = $(foreach dir, $(DIRS), -I./$(dir))
CFLAGS += $(INCLUDES)

.PHONY: clean
clean:
	find . -maxdepth 1 -iname '*.o' -delete -or -iname '*.a' -delete -or -type f -executable -delete

foo:
	echo $(MAKEFILES)
	echo $(OBJS)

#This should work anywhere and it doesn't hurt to have unused object files except for speed
#test: test.o $(OBJS)
#	$(CC) $(LDFLAGS) -o $@ $^
#	./$@
