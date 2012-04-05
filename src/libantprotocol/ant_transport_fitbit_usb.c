#include <stdlib.h>
#include <stdio.h>

#include "ant_transport_fitbit_usb.h"

ant_transport_fitbit_usb_t* ant_transport_fitbit_usb_alloc(uint16_t vid,
                                                           uint16_t pid) {
    ant_transport_fitbit_usb_t* fb;
    fb = malloc(sizeof(ant_transport_fitbit_usb_t));

    int res = libusb_init(&fb->context);
    if (res != 0) {
        /* Fail */
        printf("Failed to initialize libusb\n");
        fb->context = NULL;
    } else {
        libusb_set_debug(fb->context, 3);
        libusb_device **list;
        libusb_device *found = NULL;
        struct libusb_device_descriptor desc;

        ssize_t cnt = libusb_get_device_list(fb->context, &list);
        ssize_t i = 0;
        int err = 0;

        if (cnt < 0) goto error_out;

        /* Could also use libusb_open_device_with_vid_pid */
        for (i = 0; i < cnt; ++i) {
            libusb_device *device = list[i];

            if (libusb_get_device_descriptor(device, &desc) == 0) {
                if (desc.idVendor == 0x10c4 && desc.idProduct == 0x84c4) {
                    printf("found!\n");
                    printf("numconfig %d\n", desc.bNumConfigurations);
                    found = device;
                    break;
                }
            }
        }

        if (found) {
            err = libusb_open(found, &fb->handle);

            /* Do config reset */
            if (!err) err = libusb_reset_device(fb->handle);
            if (!err) err = libusb_set_configuration(fb->handle, 1);

            /* Claim interface */
            if (!err) err = libusb_claim_interface(fb->handle, 0);

        }

        libusb_free_device_list(list, 1);

        if (err) {
            goto error_out;
        }
    }

    fb->send = &ant_transport_fitbit_usb_send;
    fb->recv = ant_transport_fitbit_usb_recv;

    return fb;
error_out:
    printf("Failed to initialize libusb\n");
    ant_transport_fitbit_usb_free(fb);
    fb = NULL;
    return NULL;
}

void ant_transport_fitbit_usb_free(ant_transport_fitbit_usb_t *fb) {
    if (fb != NULL) {
        if (fb->handle != NULL) {
            libusb_release_interface(fb->handle, 0);
            libusb_close(fb->handle);
            fb->handle = NULL;
        }
        if (fb->context != NULL) {
            libusb_exit(fb->context);
            fb->context = NULL;
        }
        free(fb);
    }
}

int ant_transport_fitbit_usb_init(ant_transport_fitbit_usb_t *fb) {
    uint8_t status_buf;
    int res = 0;
    char buf[4096];

    uint8_t init_buf[] = {
        0x08, 0x00, 0x00, 0x00,
        0x40, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    // Should dirs be 0x00 and 0x80?

    // Device setup
    res = libusb_control_transfer(fb->handle, 0x40, 0x00, 0xFFFF, 0x00,
                                  NULL, 0, 0);
    if (res < 0) return -1;

    res = libusb_control_transfer(fb->handle, 0x40, 0x01, 0x2000, 0x00,
                                  NULL, 0, 0);
    if (res < 0) return -1;

    // 4096 buffer without explicit receive?
    ant_transport_fitbit_usb_recv(fb, sizeof(buf), buf);

    res = libusb_control_transfer(fb->handle, 0x40, 0x00, 0x0000, 0x00,
                                  NULL, 0, 0);
    if (res < 0) return -1;
    res = libusb_control_transfer(fb->handle, 0x40, 0x00, 0xFFFF, 0x00,
                                  NULL, 0, 0);
    if (res < 0) return -1;
    res = libusb_control_transfer(fb->handle, 0x40, 0x01, 0x2000, 0x00,
                                  NULL, 0, 0);
    if (res < 0) return -1;
    res = libusb_control_transfer(fb->handle, 0x40, 0x01, 0x004A, 0x00,
                                  NULL, 0, 0);
    if (res < 0) return -1;
    // Receive 1 byte, should be 0x2
    res = libusb_control_transfer(fb->handle, 0xC0, 0xFF, 0x370B, 0x00,
                                  &status_buf, 1, 0);
    if (res < 0) return -1;
    printf("status_buf: %x == 0x2?\n", status_buf);
    res = libusb_control_transfer(fb->handle, 0x40, 0x03, 0x0800, 0x00,
                                  NULL, 0, 0);
    res = libusb_control_transfer(fb->handle, 0x40, 0x13, 0x0000, 0x00,
                                  init_buf, sizeof(init_buf), 0);
    if (res < 0) return -1;
    res = libusb_control_transfer(fb->handle, 0x40, 0x12, 0x000C, 0x00,
                                  NULL, 0, 0);
    if (res < 0) return -1;

    // READ HERE to snarf initial stuff?
    ant_transport_fitbit_usb_recv(fb, sizeof(buf), buf);

    return 0;
}

int ant_transport_fitbit_usb_send(void* arg, int len, void* buf) {
    ant_transport_fitbit_usb_t *fb = (ant_transport_fitbit_usb_t*)arg;

    printf("sending: ");
    for (int i = 0; i < len; ++i) {
        printf(" 0x%x", 0x00ff & ((char*)buf)[i]);
    }
    printf("\n");

    int transferred = 0;
    int res = 0;
    res = libusb_bulk_transfer(fb->handle, LIBUSB_ENDPOINT_OUT | 0x01, buf, len,
                               &transferred, 0);

    if (res < 0) return -1;
    else return transferred;
}

int ant_transport_fitbit_usb_recv(void* arg, int len, void* buf) {
    ant_transport_fitbit_usb_t *fb = (ant_transport_fitbit_usb_t*)arg;

    int transferred = 0;
    int res = 0;
    res = libusb_bulk_transfer(fb->handle, LIBUSB_ENDPOINT_IN | 0x01, buf, len,
                               &transferred, 100);

    printf("received (%d==%d): ", len, res);
    for (int i = 0; i < transferred; ++i) {
        printf(" 0x%x", 0x00ff & ((char*)buf)[i]);
    }
    printf("\n");

    if (res < 0) return -1;
    else return transferred;
}
