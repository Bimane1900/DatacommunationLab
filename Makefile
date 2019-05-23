CC = gcc
CFLAGS = -I -Wall
DEPS = header.h
OBJ = client.o server.o functions.o
#LDFLAGS = -lm -lpthread -lrt

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
