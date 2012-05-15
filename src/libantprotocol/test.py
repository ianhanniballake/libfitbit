import antc
import time

ant = antc.AntC()
ant.reset()

ant.set_network_key(0, "\x00" * 8)
ant.assign_channel(0)
ant.set_channel_period(0x10)
ant.set_channel_frequency(0x2)
ant.set_transmit_power(0x3)
ant.set_search_timeout(0xFF)
ant.set_channel_id(0xFFFF, 1, 1)
ant.open_channel()
# open channel
ant.wait_for_beacon()
time.sleep(0.5)
ant.reset_tracker()
