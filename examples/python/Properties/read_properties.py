##
# Simple example that prints out the names and values of all properties of a reference device (simulator) channel
##

import sys
sys.path.append("..")
import daq_utils

if __name__ == "__main__":
    simulator = daq_utils.setup_simulator()
    channel = simulator.channels[0]

    for prop_ in channel.visible_properties:
        print(prop_.name, prop_.value)
