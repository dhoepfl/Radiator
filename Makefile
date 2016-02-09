CFLAGS=-funsigned-char -std=c99

OBJECTS=main.o device.o debug.o serial.o

.PHONY: clean

all: radiator

clean:
	rm -f ${OBJECTS}
	rm -f radiator

%.o: %.c %.h
	cc -c -o "$@" ${CFLAGS} "$*.c"

radiator: ${OBJECTS}
	cc -o "$@" $^
