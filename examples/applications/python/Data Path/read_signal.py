##
# This example showcases how the openDAQ stream reader can be used to read signal data. It
# transforms the queue of packets that's received by an input port into a stream of data.
# Here we read the data in three ways:
# 1. Without timestamps, as floating point values
# 2. Without timestamps, as integer values
# 3. With timestamps, as integer values.
##

import sys
sys.path.append("..")

import opendaq as daq
import daq_utils
import time

if __name__ == "__main__":
    # Setup simulator and get 1st simulated analog input signal
    device = daq_utils.setup_simulator()
    signal = device.get_signals_recursive(daq.LocalIdSearchFilter("AI1"))[0]

    # Create stream reader object used to read data
    reader = daq.StreamReader(signal)

    # Read 100 samples every 0.125 seconds. The default Sample rate of the simulator is 1000Hz
    for i in range(10):
        time.sleep(0.125)
        samples = reader.read(125)

        # Print last sample
        if len(samples) > 0:
            print(samples[-1])

    # Create stream reader that converts read data to the Int64 type
    reader = daq.StreamReader(signal, value_type=daq.SampleType.Int64)

    # Read and print 10 values
    time.sleep(0.5)
    int_values = reader.read(500)
    print(int_values[-10:])

    # Read and print 10 timestamped samples
    # The TimeStreamReader is a reader wrapper that outputs timestamps in the datetime format
    time_reader = daq.TimeStreamReader(reader)
    time.sleep(0.5)
    int_values, timestamps = time_reader.read_with_timestamps(500)
    print(list(zip(int_values[-10:], timestamps[-10:])))
