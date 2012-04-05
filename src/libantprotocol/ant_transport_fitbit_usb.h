#ifndef ANT_TRANSPORT_FITBIT_USB_H
#define ANT_TRANSPORT_FITBIT_USB_H 1

#include <libusb-1.0/libusb.h>
#include <stdint.h>

typedef struct ant_transport_fitbit_usb {
  int (*send)(void*, int, void*);
  int (*recv)(void*, int, void*);

  libusb_context *context;
  libusb_device_handle *handle;
} ant_transport_fitbit_usb_t;

ant_transport_fitbit_usb_t* ant_transport_fitbit_usb_alloc(uint16_t vid,
                                                           uint16_t pid);

void ant_transport_fitbit_usb_free(ant_transport_fitbit_usb_t *fb);

int ant_transport_fitbit_usb_init(ant_transport_fitbit_usb_t *fb);

int ant_transport_fitbit_usb_init(ant_transport_fitbit_usb_t *fb);

int ant_transport_fitbit_usb_send(void* arg, int len, void* buf);
int ant_transport_fitbit_usb_recv(void* arg, int len, void* buf);

#endif  // ANT_TRANSPORT_FITBIT_USB_H
