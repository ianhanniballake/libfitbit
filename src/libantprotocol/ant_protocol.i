%module ant_protocol
%import "stdint.i"
%import "cstring.i"
%{
#include "ant_protocol.h"
%}

ant_handle_t* ant_handle_alloc(ant_transport_t *transport);
void ant_handle_free(ant_handle_t *ant);

int ant_reset(ant_handle_t *ant);
int ant_set_channel_frequency(ant_handle_t *ant, uint8_t freq);
int ant_set_transmit_power(ant_handle_t *ant, uint8_t power);
int ant_set_search_timeout(ant_handle_t *ant, uint8_t timeout);
int ant_set_channel_period(ant_handle_t *ant, uint16_t period);
int ant_set_channel_id(ant_handle_t *ant,
                           uint16_t device_number,
                           uint8_t device_type,
                           uint8_t transmission_type);
int ant_open_channel(ant_handle_t *ant);
int ant_close_channel(ant_handle_t *ant);
int ant_assign_channel(ant_handle_t *ant, uint8_t channel);
int ant_set_network_key(ant_handle_t *ant, uint8_t network, const char *key);

int ant_check_for_beacon(ant_handle_t *ant);
int ant_wait_for_beacon(ant_handle_t *ant, uint8_t retries);
int ant_reset_tracker(ant_handle_t *ant);
int ant_ping_tracker(ant_handle_t *ant);

//%cstring_output_allocate_size(parm, szparm)
%apply (char *STRING, int LENGTH) { (char *data, int data_len) };
int ant_send_acknowledged_data(ant_handle_t *ant, char *data, int data_len);
