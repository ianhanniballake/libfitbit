CFLAGS = -W -Wall -Wno-unused-parameter \
	 -std=gnu99 \
	 -I/usr/include/python2.6 -fPIC

all: _ant_protocol.so _ant_transport.so

_ant_protocol.so: ant_protocol_wrap.o ant_protocol.o
	gcc -shared -o $@ $^

_ant_transport.so: ant_transport_wrap.o ant_transport_fitbit_usb.o
	gcc -shared -o $@ $^ -lusb-1.0

ant_protocol.o: ant_protocol.c ant_protocol.h

ant_protocol.h: ant_transport.h

ant_protocol.i: ant_protocol.h ant_protocol.c
ant_transport.i: ant_transport.h

ant_protocol_wrap.c: ant_protocol.i
	swig -python ant_protocol.i

ant_transport_wrap.c: ant_transport.i
	swig -python ant_transport.i

clean:
	rm -f *.o *.a *.so *.pyc ant_protocol.py ant_protocol_wrap.c ant_transport.py ant_transport_wrap.c
