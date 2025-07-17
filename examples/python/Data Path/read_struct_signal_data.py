##
# The openDAQ simulated device contains a rough approximation of a CAN channel that contains
# Structured data with the fields "Length" and "ArbId". This example uses the stream reader
# to read struct-type data from the simulated CAN channel.
##

import sys
sys.path.append("..")

import opendaq as daq
import daq_utils

if __name__ == "__main__":
    device = daq_utils.setup_simulator()
    device.set_property_value('EnableCANChannel', True)

    # filter signals by local ID
    can_signal = device.get_signals_recursive(daq.LocalIdSearchFilter('CAN'))[0]

    # Create a stream reader that reads data without conversion. Undefined-type readers can not skip events.
    reader = daq.StreamReader(can_signal, skip_events=False, value_type=daq.SampleType.Undefined)

    # When not skipping events, the 1st read will always return an Event status that contains the
    # value and domain signal descriptors.
    _, status = reader.read(0, return_status=True)

    # Print out the value and domain data descriptor fields
    for id_, descriptor in status.event_packet.parameters.items():
        print(id_, descriptor)

    # Read out struct data. An openDAQ simulated CAN signal has the fields "Length" and "ArbId".
    values = reader.read(4)
    for i in range(len(values)):
        length = values['Length'][i]
        arbId = values['ArbId'][i]
        print(f'ArbId: {arbId} Length: {length} Data: {values['Data'][i][:length]}')
