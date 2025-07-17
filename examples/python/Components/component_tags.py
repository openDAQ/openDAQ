##
# Each component in openDAQ has 0 to n tags that can be used to group and filter components
# The tags can be accessed via the `component.tags` attribute.
#
# This example showcases how we can query for the existence of a given tag.
##

import opendaq as daq
import sys
sys.path.append("..")
import daq_utils

if __name__ == "__main__":
    instance = daq.Instance()
    simulator = daq_utils.setup_simulator()
    device = daq_utils.add_simulator(instance)

    # We find the device's time signal as it has a tag.
    time_sig = device.get_signals()[0]
    tags = time_sig.tags

    print('Printing time signal tags...')
    for tag in tags.list:
        print(tag)

    # We can check for whether the tags object contains a tag
    print('Printing 1 if time signal tags contain \"Apple\", 0 otherwise:', tags.contains('Apple'))

    # A query string can be provided to query the list of tags according to the specified expression
    print('Query result for \"Apple || DeviceDomain\":', tags.query('Apple || DeviceDomain'))
    print('Query result for \"Apple && DeviceDomain\":', tags.query('Apple && DeviceDomain'))