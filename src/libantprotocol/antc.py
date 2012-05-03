import ant_protocol
import ant_transport

class AntC(object):
    def __init__(self):
        self.transport = None
        self.ant = None

        self.transport = ant_transport.fitbit_usb_alloc(0x10c4, 0x84c4)
        ant_transport.fitbit_usb_init(self.transport)

        self.ant = ant_protocol.ant_handle_alloc(
                ant_transport.coerce(self.transport))

    def __del__(self):
        if self.ant is not None:
            ant_protocol.ant_handle_free(self.ant)
        if self.transport is not None:
            ant_transport.fitbit_usb_free(self.transport)

    def reset(self):
        ant_protocol.ant_reset(self.ant)

    def assign_channel(self, channel):
        ant_protocol.ant_assign_channel(self.ant, channel)

    def set_channel_frequency(self, freq):
        ant_protocol.ant_set_channel_frequency(self.ant, freq)

    def set_channel_period(self, period):
        ant_protocol.ant_set_channel_period(self.ant, period)

    def set_transmit_power(self, power):
        ant_protocol.ant_set_transmit_power(self.ant, power)

    def set_search_timeout(self, timeout):
        ant_protocol.ant_set_search_timeout(self.ant, timeout)

    def set_network_key(self, network, key):
        if len(key) != 8:
            raise ValueError("Key must be 8 bytes long")
        ant_protocol.ant_set_network_key(self.ant, network, key)

    def set_channel_id(self, device_number, device_type, transmission_type):
        ant_protocol.ant_set_channel_id(self.ant,
                                        device_number,
                                        device_type,
                                        transmission_type)

    def open_channel(self):
        ant_protocol.ant_open_channel(self.ant)

    def receive(self):
