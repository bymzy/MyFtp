CC=gcc
TARGET=server
LIB=
CFLAGS=-g
OBJS=${patsubst %.c,%.o,${wildcard *.c}}

${TARGET}: DEPEND ${OBJS} ${DEPENDOBJS}
	${CC} ${LIB} -o bin/$@ objs/${OBJS}

DEPEND:
	mkdir -p objs/ bin/

%.o:%.c
	${CC} ${CFLAGS} -c $< -o objs/$@

output:
	echo objs/${OBJS}

clean:
	rm -rf objs/ bin/
