%module ant_transport
%import "stdint.i" 
%{
#include "ant_transport.h"
#include "ant_transport_fitbit_usb.h"

ant_transport_t* coerce(void* arg) { return (ant_transport_t*)arg; }
%}

ant_transport_t* coerce(void* arg);

%rename(fitbit_usb_alloc) ant_transport_fitbit_usb_alloc;
ant_transport_fitbit_usb_t* ant_transport_fitbit_usb_alloc(uint16_t vid,
                                                           uint16_t pid);

%rename(fitbit_usb_free) ant_transport_fitbit_usb_free;
void ant_transport_fitbit_usb_free(ant_transport_fitbit_usb_t *fb);

%rename(fitbit_usb_init) ant_transport_fitbit_usb_init;
int ant_transport_fitbit_usb_init(ant_transport_fitbit_usb_t *fb);
