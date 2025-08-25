##
# Discovers remote and local devices and prints server information. Remote devices are those that are running
# their own openDAQ instance with a server. To a simulator of such a device, the example in
# "Integration Examples/simulator.py" can be used.
#
# Servers in openDAQ can either allow use to configure the device, stream its data, or both. They can be
# differentiated between using the server capability field "Protocol Type" as shown in this example.
##

import opendaq as daq
import sys
sys.path.append("..")
import daq_utils

if __name__ == "__main__":
    server = daq_utils.setup_simulator()
    instance = daq.Instance()

    # Discover devices
    remote_device_info = []
    available_devices = instance.available_devices
    for device_info in available_devices:
        if len(device_info.server_capabilities):
            remote_device_info.append(device_info)

    streaming_connection_strings = []
    configuration_connection_strings = []

    # Find streaming and configuration connection strings of
    # devices with server capabilities
    for device_info in remote_device_info:
        for capability in device_info.server_capabilities:
            print('\n' + device_info.name + ': ' + capability.protocol_id)
            daq_utils.print_property_object(capability, 2)

            if capability.protocol_type == daq.ProtocolType.Streaming:
                streaming_connection_strings.append(capability.connection_string)
            elif capability.protocol_type in [daq.ProtocolType.Configuration, daq.ProtocolType.ConfigurationAndStreaming]:
                configuration_connection_strings.append(capability.connection_string)

    print('\nStreaming-only connection strings: ', streaming_connection_strings)
    print('Configuration connection strings: ', configuration_connection_strings)

    # Connect via streaming-only connection strings
    devices = []
    print('\n\nConnecting via streaming-only connections.')
    for connection_string in streaming_connection_strings:
        daq_utils.connect_and_append_device(instance, connection_string, devices)

    print('\nConnected streaming-only devices:')
    for device in devices:
        print(device.name)
        instance.remove_device(device)

    devices.clear()

    # Connect via configuration connection strings
    # Note that this also establishes streaming connections when available on device
    print('\n\nConnecting via configuration connections.')
    for connection_string in configuration_connection_strings:
        daq_utils.connect_and_append_device(instance, connection_string, devices)

    print('\nConnected devices:')
    for device in devices:
        print(device.name)

    # Discover devices and check connected clients
    print('\nConnected client information of discoverable devices:')
    remote_device_info = []
    available_devices = instance.available_devices
    for device_info in available_devices:
        print(device_info.name + ':')
        for connected_client in device_info.connected_clients_info:
            print('Client ' + connected_client.host_name + ':')
            daq_utils.print_property_object(connected_client, 2)

    # Cleanup
    for device in devices:
        instance.remove_device(device)
    devices.clear()
