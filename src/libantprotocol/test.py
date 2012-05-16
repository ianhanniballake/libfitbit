import antc
import time

ant = antc.AntC()
ant.init_tracker_for_transfer()
print ant.run_opcode("\x24\x00\x00\x00\x00\x00\x00", "")
print ant.run_opcode("\x24\x00\x00\x00\x00\x00\x00", "foobar")
print "all done"
