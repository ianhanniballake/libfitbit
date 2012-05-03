#ifndef ANT_PROTOCOL_H
#define ANT_PROTOCOL_H 1

#include <stdint.h>

#include "ant_transport.h"

/*
 * http://www.thisisant.com/images/Resources/PDF/1204662412_ant_message_protocol_and_usage.pdf
 */

/* Table 5-1 */
enum ant_channel_type {
  BIDIRECTIONAL_SLAVE = 0x00,
  BIDIRECTIONAL_MASTER = 0x10,
  SHARED_BIDIRECTIONAL_SLAVE_CHANNEL = 0x20,
  SLAVE_RECEIVE_ONLY_CHANNEL = 0x40
};

enum ant_message_code {
  RESPONSE_NO_ERROR = 0,
  EVENT_RX_SEARCH_TIMEOUT        = 1,
  EVENT_RX_FAIL                  = 2,
  EVENT_TX                       = 3,
  EVENT_TRANSFER_RX_FAILED       = 4,
  EVENT_TRANSFER_TX_COMPLETE     = 5,
  EVENT_TRANSFER_TX_FAILED       = 6,
  EVENT_CHANNEL_CLOSED           = 7,
  EVENT_RX_FAIL_GO_TO_SEARCH     = 8,
  EVENT_CHANNEL_COLLISION        = 9,
  EVENT_TRANSFER_TX_START        = 10,
  CHANNEL_IN_WRONG_STATE         = 21,
  CHANNEL_NOT_OPENED             = 22,
  CHANNEL_ID_NOT_SET             = 24,
  CLOSE_ALL_CHANNELS             = 25,
  TRANSFER_IN_PROGRESS           = 31,
  TRANSFER_SEQUENCE_NUMBER_ERROR = 32,
  TRANSFER_IN_ERROR              = 33,
  INVALID_MESSAGE                = 40,
  INVALID_NETWORK_NUMBER         = 41,
  INVALID_LIST_ID                = 48,
  INVALID_SCAN_TX_CHANNEL        = 49,
  INVALID_PARAMETER_PROVIDED     = 51,
  EVENT_QUEUE_OVERFLOW           = 53,
  NVM_FULL_ERROR                 = 64,
  NVM_WRITE_ERROR                = 65,
  ASSIGN_CHANNEL_ID              = 66,
  SET_CHANNEL_ID                 = 81,
};

enum ant_event {
  ANT_UNASSIGN_CHANNEL               = 0x41,
  ANT_ASSIGN_CHANNEL                 = 0x42,
  ANT_SET_CHANNEL_PERIOD             = 0x43,
  ANT_SET_CHANNEL_SEARCH_TIMEOUT     = 0x44,
  ANT_SET_CHANNEL_RF_FREQ            = 0x45,
  ANT_SET_NETWORK_KEY                = 0x46,
  ANT_SET_TRANSMIT_POWER             = 0x47,
  ANT_SET_CHANNEL_ID                 = 0x51,
  ANT_ADD_CHANNEL_ID                 = 0x59,
  ANT_CONFIG_LIST                    = 0x5A,
  ANT_SET_CHANNEL_TX_POWER           = 0x60,
  ANT_SET_LOW_PRIORITY_CHANNEL_SEARCH_TIMEOUT = 0x63,
  ANT_SET_SERIAL_NUM_CHANNEL_ID               = 0x65,
  ANT_RX_EXT_MESGS_ENABLE                     = 0x66,
  ANT_ENABLE_LED                              = 0x68,
  ANT_CRYSTAL_ENABLE                          = 0x6D,
  ANT_LIB_CONFIG                              = 0x6E,
  ANT_CONFIGURE_FREQUENCY_AGILITY             = 0x70,
  ANT_SET_PROXIMITY_SEARCH                    = 0x71,
  ANT_SET_CHANNEL_SEARCH_PRIORITY             = 0x75,
  //ANT_SET_USB_DESCRIPTOR_STRING               
  ANT_STARTUP_MESSAGE                         = 0x6F,
  ANT_ERROR_MESSAGE                           = 0xAE,
  ANT_RESET_SYSTEM                            = 0x4A,
  ANT_OPEN_CHANNEL                            = 0x4B,
  ANT_CLOSE_CHANNEL                           = 0x4C,
  ANT_OPEN_RX_SCAN_MODE                       = 0x5B,
  ANT_REQUEST_MESSAGE                         = 0x4D,
  ANT_SLEEP_MESSAGE                           = 0xC5,
  ANT_SEND_BROADCAST_DATA                     = 0x4E,
  ANT_SEND_ACK_DATA                           = 0x4F,
  ANT_SEND_BURST_TRANSFER_PACKET              = 0x50,
  ANT_CHANNEL_RESPONSE                        = 0x40,
  ANT_CHANNEL_STATUS                          = 0x52,
  ANT_CHANNEL_ID                              = 0x51,
  ANT_VERSION                                 = 0x3E,
  ANT_CAPABILITIES                            = 0x54,
  ANT_SERIAL_NUMBER                           = 0x61,
  ANT_INIT_CW_TEST_MODE                       = 0x53,
  ANT_SET_CW_TEST_MODE                        = 0x48,
};

#define ANT_SYNC 0xa4
#define ANT_SYNC_ALT 0xa5

struct ant_handle;
typedef struct ant_handle ant_handle_t;

ant_handle_t* ant_handle_alloc(ant_transport_t *transport);
int ant_handle_init(ant_handle_t **ant, ant_transport_t *transport);
void ant_handle_free(ant_handle_t *ant);

int ant_reset(ant_handle_t *ant);
int ant_set_channel_frequency(ant_handle_t *ant, uint8_t freq);
int ant_set_transmit_power(ant_handle_t *ant, uint8_t power);
int ant_set_search_timeout(ant_handle_t *ant, uint8_t timeout);
int ant_set_channel_period(ant_handle_t *ant, uint8_t period);
int ant_set_channel_id(ant_handle_t *ant,
                         uint16_t device_number,
                         uint8_t device_type,
                         uint8_t transmission_type);
int ant_open_channel(ant_handle_t *ant);
int ant_close_channel(ant_handle_t *ant);
int ant_assign_channel(ant_handle_t *ant, uint8_t channel);

int ant_set_network_key(ant_handle_t *ant, uint8_t network, const char *key);

#endif  // ANT_PROTOCOL_H
