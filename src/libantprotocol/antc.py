import ant_protocol
import ant_transport
import random

class AntC(object):
    def __init__(self,
                 network_key=None,
                 channel=None,
                 period=None,
                 frequency=None,
                 transmit_power=None,
                 search_timeout=None,
                 channel_id=None,
                 transmission_type=None,
                 device_type=None):

        self.transport = None
        self.ant = None

        self.transport = ant_transport.fitbit_usb_alloc(0x10c4, 0x84c4)
        ant_transport.fitbit_usb_init(self.transport)

        self.ant = ant_protocol.ant_handle_alloc(
                ant_transport.coerce(self.transport))

        if network_key is None: self.network_key = "\x00" * 8
        else: self.network_key = network_key

        if channel is None: self.channel = 0
        else: self.channel = channel

        if period is None: self.period = 0x40
        else: self.period = period

        if frequency is None: self.frequency = 0x2
        else: self.frequency = frequency

        if transmit_power is None: self.transmit_power = 0x3
        else: self.transmit_power = transmit_power

        if search_timeout is None: self.search_timeout = 0xff
        else: self.search_timeout = search_timeout

        if transmission_type is None: self.transmission_type = 1
        else: self.transmission_type = transmission_type

        if device_type is None: self.device_type = 1
        else: self.device_type = device_type

    def init_channel(self, channel_id = 0xFFFF):
        self.reset()
        self._set_network_key(0, self.network_key)
        self._assign_channel(self.channel)
        self._set_channel_period(self.period)
        self._set_channel_frequency(self.frequency)
        self._set_transmit_power(self.transmit_power)
        self._set_search_timeout(self.search_timeout)
        self._set_channel_id(channel_id,
                             self.transmission_type,
                             self.device_type)
        self._open_channel()

    def initiate_hop(self):
        self.reset_tracker()
        target_channel = random.randint(0,0xFFFF)
        self.send_tracker_hop(target_channel)
        self._close_channel()

        self.init_channel(target_channel)
        self.wait_for_beacon()
        print "ping"
        self.ping_tracker()
        print "pong"

        ant_protocol.tracker_get_info(self.ant)


    def init_tracker_for_transfer(self):
        self.reset()
        self.init_channel()
        print "waiting for beacon"
        self.wait_for_beacon()
        self.initiate_hop()

    def run_opcode(self, opcode, payload):
        print "run_opcode: ", opcode, payload
        if type(opcode) is list:
            opcode = "".join([chr(c) for c in opcode])
        if type(payload) is list:
            payload = "".join(payload)

        res = ant_protocol.tracker_run_opcode(self.ant,
                                                  opcode,
                                                  payload)

        if type(res) is int:
            raise Exception("execution failed " + str(res))
        elif res[0] != 0:
            raise Exception("execution failed " + str(res))
        else: return [ord(c) for c in list(res[1])]


    def __del__(self):
        if self.ant is not None:
            ant_protocol.ant_handle_free(self.ant)
        if self.transport is not None:
            ant_transport.fitbit_usb_free(self.transport)

    def reset(self):
        ant_protocol.ant_reset(self.ant)

    def close(self):
        self._close_channel()

    def _assign_channel(self, channel):
        ant_protocol.ant_assign_channel(self.ant, channel)

    def _set_channel_frequency(self, freq):
        ant_protocol.ant_set_channel_frequency(self.ant, freq)

    def _set_channel_period(self, period):
        ant_protocol.ant_set_channel_period(self.ant, period)

    def _set_transmit_power(self, power):
        ant_protocol.ant_set_transmit_power(self.ant, power)

    def _set_search_timeout(self, timeout):
        ant_protocol.ant_set_search_timeout(self.ant, timeout)

    def _set_network_key(self, network, key):
        if len(key) != 8:
            raise ValueError("Key must be 8 bytes long")
        ant_protocol.ant_set_network_key(self.ant, network, key)

    def _set_channel_id(self, device_number, device_type, transmission_type):
        ant_protocol.ant_set_channel_id(self.ant,
                                        device_number,
                                        device_type,
                                        transmission_type)

    def _open_channel(self):
        ant_protocol.ant_open_channel(self.ant)

    def _close_channel(self):
        ant_protocol.ant_close_channel(self.ant)

    def wait_for_beacon(self):
        for tries in range(75):
            if ant_protocol.ant_check_for_beacon(self.ant) == 0: return

    def reset_tracker(self):
        ant_protocol.ant_reset_tracker(self.ant)

    def send_tracker_hop(self, channel_id):
        ant_protocol.ant_send_tracker_hop(self.ant, channel_id)

    def ping_tracker(self):
        ant_protocol.ant_ping_tracker(self.ant)

    #def receive(self):

