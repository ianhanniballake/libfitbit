CFLAGS = -W -Wall -std=c99

all: libantprotocol.a
libantprotocol.a: ant_protocol.o
	ar -rs $@ $^


ant_protocol.o: ant_protocol.c ant_protocol.h

clean:
	rm -f *.o *.a
