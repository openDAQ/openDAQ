##
# Simple example that changes the amplitude property of a reference device (simulator) channel
##

import sys
sys.path.append("..")
import daq_utils

if __name__ == "__main__":
    simulator = daq_utils.setup_simulator()
    channel = simulator.channels[0]

    print(channel.get_property_value('Amplitude'))
    channel.set_property_value('Amplitude', 7)
    print(channel.get_property_value('Amplitude'))
