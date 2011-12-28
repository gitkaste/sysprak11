ifndef CC
CC = gcc
endif
ifndef CFLAGS
CFLAGS = -Wall -pedantic -Werror -std=gnu99
endif
CFLAGS += -I ../util/ -I ../tokenizer/
LDFLAGS += -L ../util/ -L. -L../tokenizer/

.DEFAULT: $(OBJS)
# This includes the util makefile also
-include ../tokenizer/makefile
VPATH=../tokenizer

OBJS = config.o

#LIB = libtokenizer.a
#LIBS += $(LIB)

# This is an intrinsic rule but hey let's be explicit
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

#$(LIB): $(OBJS)
#	ar cru $@ $?
#	-ranlib $@
#

test: test.o $(OBJS) $(LIBS) tokenizer.o
	$(CC) -static $(LDFLAGS)  -lutil -o $@ $(OBJS) test.o tokenizer.o
