##
# openDAQ devices can allow users to configure their network settings by submitting a mDNS query for the specific
# targeting a specific device. Not all openDAQ devices support this feature - it must be enabled by the vendor.
#
# The openDAQ simulator image provides a device that enables network configuration. The ".ova" image can be found
# on the Downloads page and can be used to create a virtual machine using VirtualBox. A running simulator is required
# to run this example.
##

import sys
import opendaq
import time

def retrieve_ip_config(network_interface: opendaq.INetworkInterface):
    try:
        config = network_interface.request_current_configuration()
        print("Parameters:")
        print(f"- dhcp4: {config.get_property_value('dhcp4')}\n- address4: {config.get_property_value('address4')}\n- gateway4: {config.get_property_value('gateway4')}")
        print(f"- dhcp6: {config.get_property_value('dhcp6')}\n- address6: {config.get_property_value('address6')}\n- gateway6: {config.get_property_value('gateway6')}")
    except Exception as e:
        print(f'Failed: {str(e)}', file=sys.stderr)

def change_ip_config(device_info: opendaq.IDeviceInfo):
    net_interface = device_info.get_network_interface('enp0s3')

    config = net_interface.create_default_configuration()
    config.set_property_value('dhcp4', False)
    config.set_property_value('address4', opendaq.String('192.168.56.166/24'))
    config.set_property_value('gateway4', opendaq.String('192.168.56.1'))

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
            print('Retrieveing old IP configuration parameters ...')
            retrieve_ip_config(device_info.get_network_interface('enp0s3'))

            change_ip_config(device_info)

            print("Wait 5 seconds for new configuration applied ...")
            time.sleep(5)

            print('Retrieveing updated IP configuration parameters ...')
            retrieve_ip_config(device_info.get_network_interface('enp0s3'))

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass

