CFLAGS=-Wall -Wextra -g3

all: brom.o vbus.o alu.o disasm.o utils.o
	$(CC) $^ -o main

clean:
	rm *.o
