CFLAGS=-Wall -Wextra -g3

all: brom.o vbus.o alu.o disasm.o utils.o display.o key.o scom_reg.o scom_const.o
	$(CC) $^ -o main

clean:
	rm *.o
