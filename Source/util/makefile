CC = gcc
# I don't know why -D here has a different effect than in the file but hey
CFLAGS = -Wall -pedantic -Werror -std=c99 
#-D_XOPEN_SOURCE=700
TARGETS = util.o writef.o
all : $(TARGETS)

# This is an intrinsic rule but hey let's be explicit
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f *.o
test: $(TARGETS) test.o
	$(CC) $(LDFLAGS) -o $@ $^
