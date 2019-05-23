CC = gcc
CFLAGS = -I -Wall
PROGRAMS = client2 server2
DEPS = header.h
OBJ = client.o server.o functions.o
LDFLAGS = -lm -lpthread -lrt

ALL: ${PROGRAMS}

client2: client.c
	${CC} ${CFLAGS} -o client client.c functions.c header.h $(LDFLAGS)

server2: server.c
	${CC} ${CFLAGS} -o server server.c functions.c header.h $(LDFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

server: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

functions: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f ${OBJ}
