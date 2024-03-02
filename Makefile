CFLAGS=-Wall -Wextra -g3 -Wno-unused-parameter -Wno-unused-function -O2
#CFLAGS+=-fsanitize=address
#LDFLAGS+=-fsanitize=address

all: brom.o vbus.o alu.o disasm.o utils.o display.o key.o scom.o ram.o ram2.o print.o lib.o
	$(CC) $^ -o main $(LDFLAGS)

clean:
	rm *.o
