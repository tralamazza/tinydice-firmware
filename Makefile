PROG= tinydice
CC= avr-gcc
CXX= avr-g++
CFLAGS+= -mmcu=attiny13a -Wall -Wextra -std=gnu99 -Os -flto -fwhole-program
PORT= /dev/ttyACM0
PROGRAMMER= avrispv2

all: ${PROG}

flash: ${PROG}
	avrdude -c ${PROGRAMMER} -P ${PORT} -p t13 -v -V -U hfuse:w:$^:e -U lfuse:w:$^:e -U flash:w:$^:e

clean:
	-rm -f ${PROG}

.PHONY: clean flash