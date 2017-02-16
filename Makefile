CC=gcc
TARGET=server
LIB=-pthread
CFLAGS=-g
OBJS=${patsubst %.c, %.o,${wildcard *.c}}

${TARGET}: DEPEND ${OBJS}
	${CC} ${LIB} -o bin/$@ objs/*.o

DEPEND:
	mkdir -p objs/ bin/

%.o:%.c
	${CC} ${CFLAGS} -c $< -o objs/$@

clean:
	rm -rf objs/ bin/
