import opendaq

instance = opendaq.Instance()

available_devices_info = instance.available_devices
for device_info in available_devices_info:
    print('Name:', device_info.name, 'Connection string:', device_info.connection_string)
