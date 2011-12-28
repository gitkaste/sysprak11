ifndef CC
CC = gcc
endif
ifndef CFLAGS
CFLAGS = -Wall -pedantic -Werror -std=c99 
endif
CFLAGS += -I ../util/
LDFLAGS += -L ../util/ -L.

.DEFAULT: $(OBJS)
-include ../util/makefile
VPATH=../util

OBJS = tokenizer.o

LIB = libtokenizer.a
#LIBS += $(LIB)

# This is an intrinsic rule but hey let's be explicit
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB): $(OBJS)
	ar cru $@ $?
	-ranlib $@

test: test.o $(OBJS) $(LIBS)
	$(CC) -static $(LDFLAGS) -o $@ test.o $(OBJS) -lutil
