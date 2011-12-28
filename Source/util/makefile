ifndef CC
CC = gcc
endif
ifndef CFLAGS
CFLAGS = -Wall -pedantic -Werror -std=c99 
endif

OBJS = util.o writef.o shmem.o

LIB = libutil.a
LIBS+=$(LIB)

# This is an intrinsic rule but hey let's be explicit
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB): $(OBJS)
	ar crus $@ $?

.PHONY: clean
clean:
	rm -f *.o *.a test

test: test.o shmem.o
	$(CC) $(LDFLAGS) -o $@ $^
