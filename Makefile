CC=gcc
CFLAGS=$(shell pkg-config fuse --cflags) -Wall -I. -ggdb
LDFLAGS=$(shell pkg-config fuse --libs)

exfuse: exfuse.c ex.o
	${CC} ${CFLAGS} ${LDFLAGS} ex.o -o $@ $<

ex.o: ex.c ex.h
	${CC} ${CFLAGS} ${LDFLAGS} -c $<

ex: ex.c ex.h
	${CC} ${CFLAGS} ${LDFLAGS} -I. -D_MAIN -o $@ $<

mount: exfuse
	if [ ! -d mp ]; then\
		mkdir mp;\
	fi
	./exfuse -f mp

umount:
	fusermount -u mp

clean:
	${RM} exfuse ex *.o

