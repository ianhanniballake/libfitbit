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
