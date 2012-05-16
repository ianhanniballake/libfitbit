#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ant_protocol.h"
#include "ant_transport.h"

const int ANT_STARTUP_MAX_RETRIES = 8;
const int ANT_NETWORK_KEY_LENGTH = 8;
#define ANT_MAX_SERIAL_LENGTH 64
#define ANT_HEADER_LENGTH 3
#define ANT_CHECKSUM_LENGTH 1
#define ANT_CHANNEL_LENGTH 1

struct ant_handle {
    uint8_t channel;
    uint8_t debug;
    uint8_t state;


    /* For pulling packets off intact */
    uint8_t buffer[4096];
    uint16_t buffer_len;

    /* TODO move to tracker handle */
    uint8_t packet_id;
    uint8_t current_bank_id;

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
    uint8_t channel:  5;
    uint8_t sequence: 3;
    uint8_t burst_data[ANT_MAX_SERIAL_LENGTH];
} __attribute__((__packed__));
typedef struct ant_burst_message ant_burst_message_t;

static int burst_data_length(ant_serial_message_t *msg) {
    /* In theory this is msg->length - offsetof..., but the length field
     * is being abused by the tracker?
     */
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
    (*ant)->packet_id = 0;
    (*ant)->buffer_len = 0;
    (*ant)->current_bank_id = 0;
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
    const int min_buffer_len = 4;

    int retries = 0;

    for (int retries = 0; retries < 10; ++retries) {
        if (ant->buffer_len < min_buffer_len) {
            int transferred = ant->transport->recv(
                                       ant->transport,
                                       sizeof(ant->buffer) - ant->buffer_len,
                                       ant->buffer);
            if (transferred > 0) {
                ant->buffer_len += transferred;
            }
            continue;
        }

        int offset;
        for (offset = 0; offset < ant->buffer_len; ++offset) {
            if (ant->buffer[offset] == ANT_SYNC ||
                ant->buffer[offset] == ANT_SYNC_ALT) break;
        }
        if (offset > 0) {
            memmove(ant->buffer,
                    ant->buffer + offset,
                    ant->buffer_len - offset);
            ant->buffer_len -= offset;
            continue;
        }

        ant_serial_message_t *plausible = (ant_serial_message_t*)ant->buffer;
        int plausible_len = plausible->length \
                                + ANT_HEADER_LENGTH \
                                + ANT_CHECKSUM_LENGTH;

        if (plausible_len <= ant->buffer_len) {
            uint8_t cksum = plausible->data[plausible->length];
            uint8_t cksum_compute = ant_checksum_message(plausible);

            if (cksum == cksum_compute) {
                memcpy(msg, ant->buffer, plausible_len);
            }

            ant->buffer_len -= plausible_len;
            memmove(ant->buffer, ant->buffer + plausible_len, ant->buffer_len);

            if (cksum == cksum_compute) {
                return plausible_len;
            }
        }
    }
    return -1;
}

static int _ant_wait_for_acknowledgement(ant_handle_t *ant, int id) {
    ant_serial_message_t msg;
    ant_response_message_t *response;

    for (int retry = 0; retry < ANT_STARTUP_MAX_RETRIES; ++retry) {
        _ant_receive_message(ant, &msg);

        response = (ant_response_message_t*)msg.data;
        
        if (msg.id == ANT_SEND_ACK_DATA && response->id == id) {
            return 0;
        }
    }
    return -1;
}

static int _ant_wait_for_response(ant_handle_t *ant,
                                  uint8_t id, uint8_t status) {
    ant_serial_message_t msg;
    ant_response_message_t *response;

    for (int retry = 0; retry < ANT_STARTUP_MAX_RETRIES; ++retry) {
        _ant_receive_message(ant, &msg);

        response = (ant_response_message_t*)msg.data;
        
        if (response->id == 1 && 
                (response->code != 0 && response->code != EVENT_TX)) {
            return -1;
        }

        if (response->id == id) {
           if (response->code == status) {
               return 0;
           } else {
               return response->code;
           }
        }
    }

    return -1;
}

static int _ant_wait_for_ok(ant_handle_t *ant, int id) {
    printf("w4_ok %x\n", id);
    return _ant_wait_for_response(ant, id, RESPONSE_NO_ERROR);
}

static int _ant_wait_for_startup(ant_handle_t *ant, uint8_t status) {
    ant_serial_message_t msg;
    // read data
    for (int retry = 0; retry < ANT_STARTUP_MAX_RETRIES; ++retry) {
        _ant_receive_message(ant, &msg);
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
    return _ant_wait_for_ok(ant, cmd);
}

static int _ant_send_command_1(ant_handle_t *ant, uint8_t cmd,
                               uint8_t channel, uint8_t param0) {
    ant_serial_message_t msg = {.length = 2,
                                .id = cmd,
                                .data = {channel, param0}};
    _ant_send_message(ant, &msg);
    return _ant_wait_for_ok(ant, cmd);
}

static int _ant_send_command_2(ant_handle_t *ant, uint8_t cmd,
                               uint8_t channel, uint8_t param0,
                               uint8_t param1) {
    ant_serial_message_t msg = {.length = 3,
                                .id = cmd,
                                .data = {channel, param0, param1}};
    _ant_send_message(ant, &msg);
    return _ant_wait_for_ok(ant, cmd);
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
    return _ant_wait_for_ok(ant, cmd);
}

int _ant_send_acknowledged_data_extra(ant_handle_t *ant,
                                      const uint8_t *data,
                                      int data_len,
                                      const uint8_t *extra,
                                      int extra_len) {

    /* TODO Bounds Check */
    int res = -1;

    ant_serial_message_t msg;

    msg.id = ANT_SEND_ACK_DATA;
    msg.length = extra_len + data_len + ANT_CHANNEL_LENGTH;
    msg.data[0] = ant->channel;
    if (extra_len) memcpy(msg.data + 1, extra, extra_len);
    memcpy(msg.data + 1 + extra_len, data, data_len);

    for (int retry = 0; res != 0 && retry < 8; ++retry) {
        _ant_send_message(ant, &msg);
        if (_ant_check_tx_response(ant, ANT_SEND_ACK_DATA) == 5) {
            return 0;
        } else {
            usleep(100000);
        }
    }

    return res;
}

int ant_send_acknowledged_data(ant_handle_t *ant,
                               const uint8_t *data,
                               int data_len) {
    return _ant_send_acknowledged_data_extra(ant, data, data_len, NULL, 0);
}

int ant_reset(ant_handle_t *ant) {
    ant_serial_message_t msg = {.length = 1,
                                .id = ANT_RESET_SYSTEM,
                                .data = {0x00}};

    int status = _ant_send_message(ant, &msg);
    if (status < 0) return status;
    _ant_wait_for_startup(ant, 0x20);
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
    return _ant_wait_for_ok(ant, ANT_SET_NETWORK_KEY);
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

    if (res >= 0 && msg.id == ANT_SEND_BROADCAST_DATA) return 0;
    else return -1;
}

int ant_wait_for_beacon(ant_handle_t *ant, uint8_t retries) {
    for (int retry = 0; retry < retries; ++retry) {
        if (ant_check_for_beacon(ant) == 0) return 0;
    }
    return -1;
}

int ant_receive_acknowledged_reply_result(ant_handle_t *ant,
                                          ant_serial_message_t *msg) {
    for (int i = 0; i < 64; ++i) {
        int res = _ant_receive_message(ant, msg);
        if (msg->id == ANT_SEND_ACK_DATA) {
            ant_response_message_t *response;
            response = (ant_response_message_t*)msg->data;
            return response->code;
        }
    }

    return -1;
}

int ant_receive_acknowledged_reply(ant_handle_t *ant) {
    ant_serial_message_t msg;

    return ant_receive_acknowledged_reply_result(ant, &msg);
}

int ant_send_burst_data(ant_handle_t *ant,
                        const uint8_t *data,
                        int data_len,
                        uint8_t msg_id) {
    for (int retry = 0; retry < 2; ++retry) {
        ant_serial_message_t msg;
        msg.id = ANT_SEND_BURST_TRANSFER_PACKET;
        msg.length = 9;

        ant_burst_message_t *burst = (ant_burst_message_t*)msg.data;

        burst->channel = ant->channel;

        /* Initial packet */
        burst->sequence = 0;

        uint8_t checksum = 0;
        for (int i = 0; i < data_len; ++i) checksum = checksum ^ data[i];

        memset(burst->burst_data, 0, msg.length - 1);
        burst->burst_data[0] = msg_id;
        burst->burst_data[1] = 0x80;
        burst->burst_data[2] = data_len;
        burst->burst_data[7] = checksum;

        _ant_send_message(ant, &msg);

        if (_ant_check_tx_response(ant,
                                    ANT_SEND_BURST_TRANSFER_PACKET) != 0xa) {
            continue;
        }

        int sequence = 0;
        for (int i = 0; i < data_len; i += 8, ++sequence) {
            int len = (i + 8 < data_len ? 8 : data_len - i - 1);
            if (len < 8) {
                memset(burst->burst_data, 0, msg.length - 1);
            }

            burst->sequence = (sequence % 3) + 1;
            if (len < 8) burst->sequence |= 0x4;

            memcpy(burst->burst_data, data + i, len);

            _ant_send_message(ant, &msg);

            usleep(10000);
        }

        if (_ant_check_tx_response(ant, ANT_SEND_BURST_TRANSFER_PACKET) == 5) {
            return 0;
        } else {
            continue;
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
        printf("ant_check_burst_response %d\n", retry);
        int transferred = _ant_receive_message(ant, &msg);
        if (transferred <= 0) continue;

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

            memcpy(*response + *response_len, burst->burst_data, burst_len);
            *response_len += burst_len;
            break;

        } else if (msg.id == ANT_SEND_BURST_TRANSFER_PACKET) {
            /* Accumulate data */
            if (*response_len + burst_len > response_available) {
                *response = realloc(*response, response_available * 2);
                response_available *= 2;
            }

            memcpy(*response + *response_len, burst->burst_data, burst_len);
            *response_len += burst_len;

            if (burst->sequence & 0x4) break;
        }
    }

    printf("result %d (%d) %p\n", result, *response_len, *response);

    for (int i = 0; i < *response_len; ++i) {
        printf(" %02x", (*response)[i]);
    }
    printf("\n");

    if (result < 0) {
        if (*response) {
            free(*response);
            *response = NULL;
            *response_len = 0;
        }
    }

    return result;
}


int _ant_check_tx_response(ant_handle_t *ant, int id) {
    printf("check_tx_response\n");
    ant_serial_message_t msg;

    for (int retry = 0; retry < 30; ++retry) {
        int res = _ant_receive_message(ant, &msg);
        if (res <= 0) continue;

        if (msg.id == ANT_CHANNEL_RESPONSE) {
            ant_response_message_t *response;
            response = (ant_response_message_t*)msg.data;
            if (1 == response->id || response->id == id) return response->code;
        }
    }

    return -1;
}


int ant_reset_tracker(ant_handle_t *ant) {
    const uint8_t reset_data[] = { 0x78, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return ant_send_acknowledged_data(ant, reset_data, sizeof(reset_data));
}

int ant_send_tracker_hop(ant_handle_t *ant, uint16_t hop) {
    const uint8_t hop_data[] = { 0x78, 0x02, (hop >> 8), hop & 0Xff,
                           0x00, 0x00, 0x00, 0x00};
    return ant_send_acknowledged_data(ant, hop_data, sizeof(hop_data));
}


int ant_ping_tracker(ant_handle_t *ant) {
    const uint8_t ping_data[] = { 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return ant_send_acknowledged_data(ant, ping_data, sizeof(ping_data));
}

uint8_t next_tracker_packet_id(ant_handle_t *ant) {
    ant->packet_id = (ant->packet_id + 1) % 8;
    return ant->packet_id + 0x38;
}


int tracker_run_opcode(ant_handle_t *ant,
                       const uint8_t* opcode, int opcode_len,
                       const uint8_t* payload, int payload_len,
                       uint8_t** data, int* data_len) {
    ant_serial_message_t msg;

    for (int retry = 0; retry < 4; ++retry) {
        uint8_t packet_id = next_tracker_packet_id(ant);
        tracker_send_packet(ant, packet_id, opcode, opcode_len);
        int res = ant_receive_acknowledged_reply_result(ant, &msg);

        ant_response_message_t *response;
        response = (ant_response_message_t*)msg.data;

        printf("!response code: %x\n", response->code);

        for (int sub_retry = 0; sub_retry < 2; ++sub_retry) {
            if (res >= 0 && response->id < packet_id) {
                res = ant_receive_acknowledged_reply_result(ant, &msg);
                response = (ant_response_message_t*)msg.data;

                if (response->id == packet_id) break;
            }
        }

        /* XXX check res */
        if (res < 0) {
            printf("bad result: %d\n", res);
            continue;
        }
        if (response->id != packet_id) {
            printf("packet_id mismatch %x != %x\n",
                   response->id, packet_id);
            continue;
        }

        printf("response code: %x\n", response->code);
        /* Need to code up tracker responses ? */
        if (response->code == 0x42) {
            return tracker_get_data_bank(ant, data, data_len);
        } else if (response->code == 0x61) {
            /* Send payload data to device */
            if (payload_len) {
                return tracker_send_payload(ant, payload, payload_len,
                                           data, data_len);
            } else {
                return -1;
            }
        } else if (response->code == 0x41) {
            *data = realloc(*data, msg.length - 1);
            *data_len = msg.length - 1;
            memcpy(*data, msg.data + 1, *data_len);
            return 0;
        } else {
            printf("bad response code: %d\n", response->code);
            return -1;
        }
    }

    return -1;
}

int tracker_send_packet(ant_handle_t *ant, uint8_t packet_id,
                        const uint8_t* data, int data_len) {
    return _ant_send_acknowledged_data_extra(ant, data, data_len,
                                             &packet_id, sizeof(packet_id));
}

int tracker_get_info(ant_handle_t *ant) {
    uint8_t op[] = {0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t *data = NULL;
    int data_len = 0;
    int res = tracker_run_opcode(ant,
                                 op, sizeof(op), NULL, 0, &data, &data_len);

    uint8_t serial[4];
    memcpy(serial, data, 4);

    uint8_t firmware_version = data[5];
    uint8_t bsl_major_version = data[6];
    uint8_t bsl_minor_version = data[7];
    uint8_t app_major_version = data[8];
    uint8_t app_minor_version = data[9];

    uint8_t in_mode_bsl = data[10];
    uint8_t on_charger = data[11];

    printf("info: %d: ", data_len);
    for (int i = 0; i < data_len; ++i) {
        printf(" %02x", data[i]);
    }
    printf("\n");
    printf("serial: %02x%02x%02x%02x\nfw: %d\nbsl: %d.%d\napp: %d.%d\nbsl: %d\non charger: %d\n",
           serial[0], serial[1], serial[2], serial[3],
           firmware_version,
           bsl_major_version, bsl_minor_version,
           app_major_version, app_minor_version,
           in_mode_bsl,
           on_charger);

    if (data) free(data);
    return res;
}

int tracker_get_data_bank(ant_handle_t *ant, uint8_t** data, int* data_len) {
    int data_available = 512;
    uint8_t cmd = 0x70;

    uint8_t *bank = NULL;
    int bank_len = 0;

    *data = realloc(*data, data_available);

    for (int i = 0;i < 20; ++i) {
        uint8_t packet_id = next_tracker_packet_id(ant);
        uint8_t check_cmd[] = {cmd, 0x00, 0x02,
                               ant->current_bank_id,
                               0x00, 0x00, 0x00};
        ++ant->current_bank_id;
        cmd = 0x60;

        int res = tracker_send_packet(ant, packet_id,
                                      check_cmd, sizeof(check_cmd));
        int res2 = tracker_get_burst(ant, &bank, &bank_len);

        printf("g db id:%d cmd:%x b:%p %d d:%p %d\n",
               ant->current_bank_id,
               cmd,
               bank, bank_len,
               *data, *data_len);

        if (bank_len == 0) {
            if (bank) {
                free(bank);
                bank = NULL;
            }
            return 0;
        }

        while (*data_len + bank_len > data_available) {
            *data = realloc(*data, data_available * 2);
            data_available *= 2;
        }
        memcpy(*data + *data_len, bank, bank_len);
        *data_len += bank_len;
    }

    return -1;
}

int tracker_get_burst(ant_handle_t *ant, uint8_t** data, int* data_len) {
    int res = _ant_check_burst_response(ant, data, data_len);

    if (res != 0) return res;
    else if (*data == NULL) return -1;

    if ((*data)[1] != 0x81) {
        printf("response is not a tracker burst! got %x %x\n", *data[0], *data[1]);
        free(*data);
        *data = NULL;
        *data_len = 0;

        return -1;
    }

    int size = (*data)[3] << 8 | (*data)[2];

    if (size == 0) {
        free(*data);
        *data = NULL;
        *data_len = 0;
        return 0;
    }

    printf("size: %d\n", size);

    *data_len = size;
    memmove(*data, *data + 8, size);

    return 0;
}

int tracker_send_payload(ant_handle_t *ant,
                         const uint8_t* payload, int payload_len,
                         uint8_t** data, int* data_len) {
    int res = ant_send_burst_data(ant,
                               payload, payload_len,
                               next_tracker_packet_id(ant));
    if (res < 0) return res;

    res = _ant_check_burst_response(ant, data, data_len);
    return res;
}
