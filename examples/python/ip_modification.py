import sys
import opendaq

def change_ip_config(device_info: opendaq.IDeviceInfo):
    net_interface = device_info.get_network_interface('enp0s3')

    dhcp4 = False
    addresses4 = opendaq.List()
    addresses4.push_back(opendaq.String('192.168.56.155/24'))
    gateway4 = opendaq.String('192.168.56.1')

    config = net_interface.create_default_configuration()
    config.set_property_value('dhcp4', dhcp4)
    config.set_property_value('addresses4', addresses4)
    config.set_property_value('gateway4', gateway4)

    try:
        print('Submitting new IP configuration parameters ...')
        net_interface.submit_configuration(config)
        print('Done')
    except Exception as e:
        print(f'Failed: {str(e)}', file=sys.stderr)

def main():
    # Create a fresh openDAQ(TM) instance that we will use for all the interactions with the openDAQ(TM) SDK
    instance = opendaq.Instance()

    # Find and connect to a simulator device
    for device_info in instance.available_devices:
        if device_info.name == 'Reference device simulator':
            change_ip_config(device_info)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass

