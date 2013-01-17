BUILDDIR      = build
SRCDIR        = src
DOCDIR        = doc
PREFER_CLANG  = yes
CFLAGS        = --std=gnu99 -Wall -Werror -Wextra -pedantic -ggdb

SRCS          = $(shell grep -L 'int main' $(SRCDIR)/*.c | sort)
OBJS          = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o, $(SRCS))
ALL_H         = $(wildcard $(SRCDIR)/*.h) 

TARGETSRCS    = $(shell grep -l 'int main' $(SRCDIR)/*.c | sort)
TARGETS       = $(patsubst $(SRCDIR)/%.c, %, $(TARGETSRCS))
BUILD_TARGETS = $(patsubst %,$(BUILDDIR)/%, $(TARGETS))

CC            = gcc
ifeq ($(PREFER_CLANG),yes)
    ifneq ($(shell which clang 2>/dev/null),)
        CC = clang
    endif
endif

ifeq ($(CC),gcc)
  CFLAGS += -save-temps=obj -Wa,-adhln=$(@:.o=.lst)
else ifeq ($(CC),clang)
  CFLAGS += 
endif

.DEFAULT_GOAL: all
all: client server

$(TARGETS): %: $(BUILDDIR)/%
	mv $^ $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	# no use pretending - these flags only work with gcc anyway
	gcc -MM -MQ $@ $(CFLAGS) -o $(BUILDDIR)/$(*F).d $<
	$(COMPILE.c) -o $@ $<

$(BUILD_TARGETS): % : %.o $(OBJS)

.PHONY: clean distclean debug comments
clean:
	-find $(BUILDDIR) -maxdepth 1 -iname '*.o' -delete -or -iname '*.a' -delete\
	 	-or -type f -executable -delete
	rm -rf $(DOCDIR)/html/* $(DOCDIR)/latex/*

distclean:
	rm -r $(BUILDDIR)/*
	rm -rf $(DOCDIR)/html/* $(DOCDIR)/latex/*
	rm -f $(TARGETS)
	rm -f .fixed-rbg
	rm -f include/*
	rm -f lib/*

comments:
	@ ./util/cntcmt.sh $(ALL_C) $(ALL_H)

-include $(SRCS:src/%.c=build/%.d)
-include $(TARGETS:%=build/%.d)
