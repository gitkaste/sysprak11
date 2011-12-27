ifndef CC
CC = gcc
endif
ifndef CFLAGS
CFLAGS = -Wall -pedantic -Werror -std=c99 
endif

OBJS = util.o writef.o

LIBS += util.a

# This is an intrinsic rule but hey let's be explicit
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

util.a: $(OBJS)
	ar cru $@ $?
	-ranlib $@

.PHONY: clean
clean:
	rm -f *.o *.a
