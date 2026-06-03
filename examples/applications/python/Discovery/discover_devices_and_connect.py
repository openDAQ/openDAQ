##
# @tags: howto, fundamental, discovery, device
# @title: How to discover devices and connect
##
# openDAQ can discover both local and remote devices. Local devices are
# provided by modules bundled with the SDK (like the reference simulator
# or the audio device). Remote devices are running their own openDAQ
# instance with servers enabled, reachable over the network.
#
# Device discovery returns DeviceInfo objects with everything needed to
# connect: name, connection string, serial number, and server capabilities.
# Remote devices have server capabilities (OPC UA, Native Streaming);
# local devices don't.
#
# This example discovers all available devices, separates them into
# remote and local, prints their info, and connects to each one.
##

import sys
sys.path.append("..")
import opendaq as daq
import Utils.daq_utils as daq_utils

if __name__ == "__main__":
    try:
        instance = daq.Instance()

        available_devices = instance.available_devices
        if not available_devices:
            print("No devices found", file=sys.stderr)
            print("exit 1")
            sys.exit(1)

        # Devices with server_capabilities are remote (running their own
        # openDAQ server). Devices without are local (instantiated by
        # modules in this process, like the reference device or audio).
        remote = []
        local = []
        for device_info in available_devices:
            if len(device_info.server_capabilities):
                remote.append(device_info)
            else:
                local.append(device_info)

        print("Remote devices:")
        for info in remote:
            print(f"\n  {info.name}:")
            daq_utils.print_property_object(info, 2)

        print("\nLocal devices:")
        for info in local:
            print(f"\n  {info.name}:")
            daq_utils.print_property_object(info, 2)

        # Connect to each discovered device. add_device takes the
        # connection string from the DeviceInfo and returns the device.
        # For remote devices the string looks like "daq.opcua://host"
        # or "daq.nd://host". For local ones it's "daqref://device0"
        # or "miniaudio://device0".
        print("\nConnecting to devices...")
        connected = []
        for info in available_devices:
            device = instance.add_device(info.connection_string)
            connected.append(device)
            print(f"  Connected: {device.name}")

        print(f"\n{len(connected)} device(s) connected")

    except Exception as e:
        print(f"Example failed: {e}", file=sys.stderr)
        print("exit 1")
        sys.exit(1)

    print("exit 0")
    sys.exit(0)