#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ant_protocol.h"
#include "ant_transport.h"

const int ANT_STARTUP_MAX_RETRIES = 8;
const int ANT_NETWORK_KEY_LENGTH = 8;
#define ANT_MAX_SERIAL_LENGTH 16
#define ANT_HEADER_LENGTH 3
#define ANT_CHECKSUM_LENGTH 1
#define ANT_CHANNEL_LENGTH 1

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
    uint8_t channel;
    uint8_t id;
    uint8_t code;
} __attribute__((__packed__));
typedef struct ant_response_message ant_response_message_t;

struct ant_burst_message {
    uint8_t sequence: 3;
    uint8_t channel:  5;
    uint8_t burst_data[ANT_MAX_SERIAL_LENGTH];
} __attribute__((__packed__));
typedef struct ant_burst_message ant_burst_message_t;

static int burst_data_length(ant_serial_message_t *msg) {
    return msg->length - offsetof(ant_burst_message_t, burst_data);
}

static uint8_t ant_checksum_message(ant_serial_message_t *msg) {
    uint8_t cksum = msg->_sync ^ msg->length ^ msg->id;
    for (int i = 0; i < msg->length; ++i) cksum ^= msg->data[i];
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

    /* add space for checksum */
    msg->data[msg->length] = ant_checksum_message(msg);

    ant->transport->send(ant->transport,
                         ANT_HEADER_LENGTH + msg->length + ANT_CHECKSUM_LENGTH,
                         msg);
    return 0;
}

static int _ant_receive_message(ant_handle_t *ant, ant_serial_message_t *msg) {
    /* TODO sync */
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

static int _ant_send_command_id_custom(ant_handle_t *ant, uint8_t cmd,
                               uint8_t channel, uint16_t param0,
                               uint8_t param1, uint8_t param2) {
    ant_serial_message_t msg = {.length = 5,
                                .id = cmd,
                                .data = {channel,
                                         ((param0 >> 8) & 0x00FF),
                                         (param0 & 0x00FF),
                                         param1, param2}};
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

int ant_set_transmit_power(ant_handle_t *ant, uint8_t power) {
    return _ant_send_command_1(ant,
                             ANT_SET_TRANSMIT_POWER,
                             0,
                             power);
}

int ant_set_search_timeout(ant_handle_t *ant, uint8_t timeout) {
    return _ant_send_command_1(ant,
                             ANT_SET_CHANNEL_SEARCH_TIMEOUT,
                             ant->channel,
                             timeout);
}

int ant_set_network_key(ant_handle_t *ant, uint8_t network,
                        const char *key) {
    ant_serial_message_t msg = {.length = 1 + ANT_NETWORK_KEY_LENGTH,
                                .id = ANT_SET_NETWORK_KEY,
                                .data = {}};
    msg.data[0] = network;
    memcpy(msg.data + 1, key, ANT_NETWORK_KEY_LENGTH);

    _ant_send_message(ant, &msg);
    return _ant_wait_for_ok(ant);
}

int ant_set_channel_period(ant_handle_t *ant, uint16_t period) {
    return _ant_send_command_2(ant,
                             ANT_SET_CHANNEL_PERIOD,
                             ant->channel,
                             (period >> 8) & 0xFF, period & 0xFF);
}

int ant_set_channel_id(ant_handle_t *ant,
                       uint16_t device_number,
                       uint8_t device_type,
                       uint8_t transmission_type) {
    return _ant_send_command_id_custom(ant,
                             ANT_SET_CHANNEL_ID,
                             ant->channel,
                             device_number,
                             device_type,
                             transmission_type);
}

int ant_open_channel(ant_handle_t *ant) {
    return _ant_send_command_0(ant, ANT_OPEN_CHANNEL, ant->channel);
}

int ant_close_channel(ant_handle_t *ant) {
    return _ant_send_command_0(ant, ANT_CLOSE_CHANNEL, ant->channel);
}

int ant_assign_channel(ant_handle_t *ant, uint8_t channel) {
    ant->channel = channel;
    return _ant_send_command_2(ant, ANT_ASSIGN_CHANNEL, ant->channel, 0, 0);
}

int ant_check_for_beacon(ant_handle_t *ant) {
    ant_serial_message_t msg;
    int res = _ant_receive_message(ant, &msg);

    if (res == 0 && msg.id == ANT_SEND_BROADCAST_DATA) return 0;
    else return -1;
}

int ant_wait_for_beacon(ant_handle_t *ant, uint8_t retries) {
    for (int retry = 0; retry < retries; ++retry) {
        if (ant_check_for_beacon(ant) == 0) return 0;
    }
    return -1;
}

int ant_reset_tracker(ant_handle_t *ant) {
    uint8_t reset_data[] = { 0x78, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return ant_send_acknowledged_data(ant, reset_data, sizeof(reset_data));
}

int ant_ping_tracker(ant_handle_t *ant) {
    uint8_t reset_data[] = { 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return ant_send_acknowledged_data(ant, reset_data, sizeof(reset_data));
}

int ant_receive_acknowledged_reply(ant_handle_t *ant) {
    ant_serial_message_t msg;

    for (int i = 0; i < 30; ++i) {
        int res = _ant_receive_message(ant, &msg);
        if (msg.id == ANT_SEND_ACK_DATA) {
            ant_response_message_t *response;
            response = (ant_response_message_t*)msg.data;
            return response->code;
        }
    }

    return -1;
}

int ant_send_burst_data(ant_handle_t *ant,
                        uint8_t *data,
                        int data_len,
                        float sleep_ms) {
    ant_serial_message_t msg;
    msg.id = ANT_SEND_BURST_TRANSFER_PACKET;
    msg.data[0] = ant->channel;

    /* Burst data and ids currently handled at higher layer */
    /* TODO Move lower in stack */


    for (int retry = 0; retry < 2; ++retry) {
        for (int i = 0; i < data_len; i += 9) {
            int end = (i + 9 < data_len - 1 ? i + 9 : data_len - 1);
            memcpy(msg.data + 1, data + i + 1, end - i);
            msg.length = end - i + ANT_CHANNEL_LENGTH;
            printf("copy %d\n", end - i);

            _ant_send_message(ant, &msg);
            //int res = _ant_wait_for_ok(ant);

            if (sleep_ms > 0) {
                unsigned int usec = sleep_ms * 1000;
                usleep(usec);
            }
        }
    }

    return -1;
}

int _ant_check_burst_response(ant_handle_t *ant,
                              uint8_t **response,
                              int *response_len) {
    ant_serial_message_t msg;
    ant_burst_message_t *burst;

    int result = 0;

    int response_available = 512;

    if (response == NULL || response_len == NULL) return -1;
    *response_len = 0;

    *response = realloc(*response, response_available);

    for (int retry = 0; retry < 128; ++retry) {
        int res = _ant_receive_message(ant, &msg);
        if (res != 0) continue;

        burst = (ant_burst_message_t*)msg.data;
        int burst_len = burst_data_length(&msg);

        if (msg.id == ANT_CHANNEL_RESPONSE) {
            ant_response_message_t *response;
            response = (ant_response_message_t*)msg.data;

            if (response->code == EVENT_TRANSFER_RX_FAILED) {
                break;
            }
        } else if (msg.id == ANT_SEND_ACK_DATA) {
            /* Accumulate data */
            if (*response_len + burst_len > response_available) {
                *response = realloc(*response, response_available * 2);
                response_available *= 2;
            }

            memcpy(burst->burst_data, *response + *response_len, burst_len);
            *response_len += burst_len;
        } else if (msg.id == ANT_SEND_BURST_TRANSFER_PACKET) {
            /* Accumulate data */
            if (*response_len + burst_len > response_available) {
                *response = realloc(*response, response_available * 2);
                response_available *= 2;
            }

            memcpy(burst->burst_data, *response + *response_len, burst_len);
            *response_len += burst_len;
        }
    }

    if (result < 0) {
        if (*response) {
            free(*response);
            *response = NULL;
            *response_len = 0;
        }
    }

    return result;
}

int ant_send_acknowledged_data(ant_handle_t *ant,
                               char *data,
                               int data_len) {
    /* TODO Bounds Check */
    int res = -1;

    ant_serial_message_t msg;

    msg.id = ANT_SEND_ACK_DATA;
    msg.length = data_len + ANT_CHANNEL_LENGTH;
    msg.data[0] = ant->channel;
    memcpy(msg.data + 1, data, data_len);

    for (int retry = 0; res != 0 && retry < 8; ++retry) {
        _ant_send_message(ant, &msg);
        for (int wait_retry = 0; wait_retry < 2; ++wait_retry) {
            usleep(100000);
            res = _ant_wait_for_ok(ant);
        }
    }

    return res;
}

int _ant_check_tx_response(ant_handle_t *ant) {
    ant_serial_message_t msg;

    for (int retry = 0; retry < 30; ++retry) {
        int res = _ant_receive_message(ant, &msg);
        if (res != 0) continue;

        if (msg.id == ANT_CHANNEL_RESPONSE) {
            ant_response_message_t *response;
            response = (ant_response_message_t*)msg.data;
            return response->code;
        }
    }

    return -1;
}
