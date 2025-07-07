##
# Uses the Python matplotlib package to plot the 1st 1000 samples of a simulated sine wave signal.
# The data is obtained by using the Stream Reader.
##

import sys
sys.path.append("..")

import opendaq as daq
import daq_utils
import time
import matplotlib.pyplot as plt

if __name__ == "__main__":
    device = daq_utils.setup_simulator()

    # Get first visible signal of the simulator
    signal = device.signals_recursive[0]

    # Create a stream reader that unwraps openDAQ packets and provides them as a stream of data
    reader = daq.StreamReader(signal)

    # Wait 1s for data, then read up to 1000 samples
    time.sleep(1)
    samples = reader.read(1000)

    plt.plot(samples)
    plt.show()
