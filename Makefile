CC = gcc
CFLAGS = -I -Wall
PROGRAMS = client server
DEPS = header.h
OBJ = client server
LDFLAGS = -lm -lpthread -lrt

ALL: ${PROGRAMS}

client: client.c
	${CC} ${CFLAGS} -o client client.c functions.c header.h $(LDFLAGS)

server: server.c
	${CC} ${CFLAGS} -o server server.c functions.c header.h $(LDFLAGS)

clean:
	rm -f ${OBJ}
