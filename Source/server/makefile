CC = gcc
CFLAGS = -Wall -pedantic -Werror -std=c99 -I ../util/
#LDFLAGS = -l util.a

# this helps make find all the .c files
VPATH =../util/

# this inserts all the rules from that makefile 
# even clean!
include ../util/makefile

server: server.o util.a
	$(CC) $(LDFLAGS) -o $@ $^  
