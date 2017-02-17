CC=gcc
TARGET=server
LIB=-pthread
CFLAGS=-g
OBJS=${patsubst %.c, %.o,${wildcard *.c}}

${TARGET}: DEPEND ${OBJS}
	${CC} ${LIB} -o $@ ${OBJS}

DEPEND:

%.o:%.c
	${CC} ${CFLAGS} -c $< -o $@

clean:
	rm -rf ${OBJS} ${TARGET}
