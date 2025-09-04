
#!/usr/bin/env python3
#

import opendaq as daq

if __name__ == "__main__":
    instance: daq.IInstance = daq.Instance()
    root: daq.IDevice = instance.root_device
    devices = []
    for num, device in enumerate(instance.available_devices):
        device: daq.IDeviceInfo
        config: daq.IPropertyObject = root.create_default_add_device_config()
        for property in config.visible_properties:
            print(f'{device.name}: {property.name}')
        device_config = config.get_property_value('Device')
        daqref_config = device_config.get_property_value('daqref')
        for property in device_config.visible_properties:
            print(f'  {property.name}')
            daqref_config.set_property_value('Name', f'{device.name}_{num}')
            daqref_config.set_property_value('NumberOfChannels', num + 1)
        devices.append(instance.add_device(device.connection_string, config))

    for device in devices:
        device: daq.IDevice
        print(f'Device {device.name} number of channels: {len(device.channels_recursive)}')
        for channel in device.channels_recursive:
            channel: daq.IChannel
            print(f'Channel {channel.name}: {channel.global_id}')

    for fb in instance.available_function_block_types:
        print(f'Function block: {fb} {type(fb)}')

    fb: daq.IFunctionBlock = instance.add_function_block('RefFBModuleRenderer')

    for device in devices:
        device: daq.IDevice
        for channel in device.channels_recursive:
            channel: daq.IChannel
            input_port: daq.IInputPort = fb.input_ports[-1]
            input_port.connect(channel.signals_recursive[0])

    #wait input to exit
    input('Press Enter to exit...')

    with open('serialized_config.json', 'w+', encoding='utf-8') as f:
        f.write(instance.save_configuration())

    # print('biba')
