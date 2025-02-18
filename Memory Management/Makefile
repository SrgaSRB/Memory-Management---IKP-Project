CC = gcc
CFLAGS = -pthread -Wall -Wextra -I.

OBJECTS = main.o Buffer/circular_buffer.o HeapManager/heap_manager.o Server/server.o Client/client.o

all: main

main: $(OBJECTS)
	$(CC) $(CFLAGS) -o main $(OBJECTS)  # Ova linija mora koristiti TAB

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@         # I ovde mora biti TAB

clean:
	rm -f $(OBJECTS) main               # Takođe ovde TAB
