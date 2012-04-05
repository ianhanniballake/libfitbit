#include <stdlib.h>
#include <stdio.h>

#include "ant_protocol.h"
#include "ant_transport.h"

const int ANT_STARTUP_MAX_RETRIES = 8;
#define ANT_MAX_SERIAL_LENGTH 8
#define ANT_HEADER_LENGTH 3
#define ANT_CHECKSUM_LENGTH 1

struct ant_handle {
    uint8_t channel;
    uint8_t debug;
    uint8_t state;

    ant_transport_t *transport;
};

struct ant_serial_message {
    uint8_t _sync;
    uint8_t length;
    uint8_t id;
    uint8_t data[ANT_MAX_SERIAL_LENGTH + ANT_CHECKSUM_LENGTH];
} __attribute__((__packed__));
typedef struct ant_serial_message ant_serial_message_t;

struct ant_response_message {
    uint8_t _sync;
    uint8_t channel;
    uint8_t id;
    uint8_t code;
    uint8_t _cksum;
} __attribute__((__packed__));
typedef struct ant_response_message ant_response_message_t;

static uint8_t ant_checksum_message(ant_serial_message_t *msg) {
    uint8_t cksum = msg->_sync ^ msg->length ^ msg->id;
    for (int i = 0; i < msg->length; ++i) cksum ^= msg->data[i];
    return cksum;
}

static uint8_t ant_checksum_response(ant_response_message_t *msg) {
    uint8_t cksum = msg->_sync ^ msg->channel ^ msg->id
                               ^ msg->code ^ msg->_cksum;
    return cksum;
}

ant_handle_t* ant_handle_alloc(ant_transport_t *transport) {
    ant_handle_t *ant;
    ant_handle_init(&ant, transport);
    return ant;
}

int ant_handle_init(ant_handle_t **ant, ant_transport_t *transport) {
    *ant = malloc(sizeof(ant_handle_t));
    (*ant)->transport = transport;
    return 0;
}

void ant_handle_free(ant_handle_t *ant) {
    if (ant != NULL) {
        free(ant);
    }
}

static int _ant_send_message(ant_handle_t *ant, ant_serial_message_t *msg) {
    msg->_sync = ANT_SYNC;

    msg->data[msg->length] = ant_checksum_message(msg);

    ant->transport->send(ant->transport,
                         ANT_HEADER_LENGTH + msg->length + ANT_CHECKSUM_LENGTH,
                         msg);
    return 0;
}

static int _ant_receive_message(ant_handle_t *ant, ant_serial_message_t *msg) {
    ant->transport->recv(ant->transport,
                         sizeof(ant_serial_message_t),
                         msg);
    return 0;
}

static int _ant_wait_for_response(ant_handle_t *ant, uint8_t status) {
    ant_serial_message_t msg;
    ant_response_message_t *response;

    _ant_receive_message(ant, &msg);

    response = (ant_response_message_t*)msg.data;

    if (response->id == ANT_CHANNEL_RESPONSE
        && response->code == status) return 0;
    else return -1;
}

static int _ant_wait_for_ok(ant_handle_t *ant) {
    return _ant_wait_for_response(ant, RESPONSE_NO_ERROR);
}

static int _ant_wait_for_startup(ant_handle_t *ant, uint8_t status) {
    ant_serial_message_t msg;
    // read data
    for (int retry = 0; retry < ANT_STARTUP_MAX_RETRIES; ++retry) {
        _ant_receive_message(ant, &msg);
        printf("status: %x != %x\n", msg.data[0], status);
        if (msg.id == ANT_STARTUP_MESSAGE && msg.data[0] == status) {
            return 0;
        }
    }
    return -1;
}

static int _ant_send_command_0(ant_handle_t *ant, uint8_t cmd, uint8_t channel) {
    ant_serial_message_t msg = {.length = 1,
                                .id = cmd,
                                .data = {channel}};
    _ant_send_message(ant, &msg);
    return _ant_wait_for_ok(ant);
}

static int _ant_send_command_1(ant_handle_t *ant, uint8_t cmd,
                               uint8_t channel, uint8_t param0) {
    ant_serial_message_t msg = {.length = 2,
                                .id = cmd,
                                .data = {channel, param0}};
    _ant_send_message(ant, &msg);
    return _ant_wait_for_ok(ant);
}

static int _ant_send_command_2(ant_handle_t *ant, uint8_t cmd,
                               uint8_t channel, uint8_t param0,
                               uint8_t param1) {
    ant_serial_message_t msg = {.length = 3,
                                .id = cmd,
                                .data = {channel, param0, param1}};
    _ant_send_message(ant, &msg);
    return _ant_wait_for_ok(ant);
}

int ant_reset(ant_handle_t *ant) {
    ant_serial_message_t msg = {.length = 1,
                                .id = ANT_RESET_SYSTEM,
                                .data = {0x00}};

    int status = _ant_send_message(ant, &msg);
    if (status < 0) return status;
    // sleep
    _ant_wait_for_startup(ant, 0x20);
    // return
    //
    return 0;
}

int ant_set_channel_frequency(ant_handle_t *ant, uint8_t freq) {
    return _ant_send_command_1(ant,
                             ANT_SET_CHANNEL_RF_FREQ,
                             ant->channel,
                             freq);
}

int ant_set_set_transmit_power(ant_handle_t *ant, uint8_t power) {
    return _ant_send_command_1(ant,
                             ANT_SET_TRANSMIT_POWER,
                             0,
                             power);
}

int ant_set_set_search_timeout(ant_handle_t *ant, uint8_t timeout) {
    return _ant_send_command_1(ant,
                             ANT_SET_CHANNEL_SEARCH_TIMEOUT,
                             ant->channel,
                             timeout);
}

int ant_set_set_channel_period(ant_handle_t *ant, uint8_t period) {
    return _ant_send_command_1(ant,
                             ANT_SET_CHANNEL_PERIOD,
                             ant->channel,
                             period);
}

int ant_set_set_channel_id(ant_handle_t *ant, uint8_t id) {
    return _ant_send_command_1(ant,
                             ANT_SET_CHANNEL_ID,
                             ant->channel,
                             id);
}

int ant_open_channel(ant_handle_t *ant) {
    return _ant_send_command_0(ant, ANT_OPEN_CHANNEL, ant->channel);
}

int ant_close_channel(ant_handle_t *ant) {
    return _ant_send_command_0(ant, ANT_CLOSE_CHANNEL, ant->channel);
}

int ant_assign_channel(ant_handle_t *ant) {
    /* XXX */
    return _ant_send_command_2(ant, ANT_ASSIGN_CHANNEL, ant->channel, 0, 0);
}
