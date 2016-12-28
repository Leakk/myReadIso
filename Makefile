CC = gcc
CFLAGS = -std=c99 -pedantic -Wextra -Wall -Werror -g

SRC=src/my_read_iso.c
OBJS=src/my_read_iso.o

all:src/my_read_iso
	mv src/my_read_iso .

clean:
	rm -f $(OBJS) *~ src/*~ my_read_iso *.iso *.MP3
check:src/my_read_iso
	mv src/my_read_iso .  
	./my_read_iso tests/example.iso < tests/cmd

src/my_read_iso:$(OBJS)

