##
# Each connected-to remote device (a device running an openDAQ server) can have a set of enumeration-type connection
# statuses stored in its connection status container. These give end-users the information on the state of the
# connection, informing them of connection loss, whether we are reconnecting...
#
# This example requires a running openDAQ device with a Native Protocol server on the local machine.
# The "Integration Examples/simulator.py" can be run to provide such a device.
##

import opendaq

def print_status_fields(status_, status_name):
    print(status_name + ':')

    # Print string enumeration value of current status
    print('Current status:', status_.name)

    # Print int value value of current status
    print('Current status (integer):', status_.value)

    # Print all possible statuses
    print('Possible states:', status_.enumeration_type.enumerator_names, '\n')

if __name__ == "__main__":
    instance = opendaq.Instance()

    # Connects to a Native Protocol device on the local network.
    device = instance.add_device('daq.nd://127.0.0.1')

    connection_status_container = device.connection_status_container

    print('Connection statuses:', list(connection_status_container.statuses.keys()), '\n')

    # Each connected-to remote device has only 1 ConfigurationStatus
    configurationStatus = connection_status_container.get_status('ConfigurationStatus')

    # Devices can have multiple streaming statuses
    streamingStatus = connection_status_container.get_status('StreamingStatus_OpenDAQNativeStreaming_1')

    print_status_fields(configurationStatus, 'ConfigurationStatus')
    print_status_fields(streamingStatus, 'StreamingStatus_OpenDAQNativeStreaming_1')
