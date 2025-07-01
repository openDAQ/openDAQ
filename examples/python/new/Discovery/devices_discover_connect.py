import opendaq as daq
import sys
sys.path.append("..")
import daq_utils

if __name__ == "__main__":
    instance = daq.Instance()

    remote_device_info = []
    local_device_info = []
    available_devices = instance.available_devices

    for device_info in available_devices:
        if len(device_info.server_capabilities):
            remote_device_info.append(device_info)
        else:
            local_device_info.append(device_info)

    connection_strings = []
    print('\nRemote discovered devices:')
    for device_info in remote_device_info:
        print('\n' + device_info.name + ':')
        daq_utils.print_property_object(device_info, 1)
        connection_strings.append(device_info.connection_string)

    print('\nLocal discovered devices:')
    for device_info in local_device_info:
        print('\n' + device_info.name + ':')
        daq_utils.print_property_object(device_info, 1)
        connection_strings.append(device_info.connection_string)

    print('\nConnecting to devices...')
    connected_devices = []
    for connection_string in connection_strings:
        daq_utils.connect_and_append_device(instance, connection_string, connected_devices)

    print('\nConnected devices:')
    for device in connected_devices:
        print(device.name)

