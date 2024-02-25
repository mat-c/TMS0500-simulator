CFLAGS=-Wall -Wextra -g3 -Wno-unused-parameter -Wno-unused-function

all: brom.o vbus.o alu.o disasm.o utils.o display.o key.o scom.o
	$(CC) $^ -o main

clean:
	rm *.o
