CC=gcc
TARGET=server
LIB=
CFLAGS=-g
OBJS=${patsubst %.c,%.o,${wildcard *.c}}

${TARGET}:${OBJS} ${DEPENDOBJS}
	${CC} ${LIB} -o bin/$@ objs/${OBJS}

%.o:%.c
	${CC} ${CFLAGS} -c $< -o objs/$@

output:
	echo objs/${OBJS}

clean:
	rm -rf objs/${OBJS} ${TARGET}
