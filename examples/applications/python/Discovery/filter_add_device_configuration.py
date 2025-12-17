import opendaq as daq
import os
import sys

py_include = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.append(py_include)

import daq_utils

def compute_supported_protocols(device_info, instance):
    server_capabilities = device_info.server_capabilities

    # Devices can be categorized into local and remote devices.
    # Local devices are devices without server capabilities.
    if len(server_capabilities) > 0:
        # Remote devices have protocol IDs defined in server capabilities
        supported_protocols = list()
        for c in server_capabilities:
            supported_protocols.append(c.protocol_id)
        return supported_protocols

    # Local devices have a single supported protocol that can be deduced from the devices connection string.
    supported_prefix = device_info.connection_string.split("://")[0]

    # Search available device types for protocol ID matching the connection string prefix
    available_device_types = instance.available_device_types
    for protocol_id, device_type in available_device_types.items():
        dt_dict = device_type.as_dictionary

        if dt_dict["Prefix"] == supported_prefix:
            return [protocol_id]
    return []

def filter_device_section(config, supported_protocols):
    # Retrieve Device section of the configuration tree
    device_section = config.get_property_value("Device")

    for property in device_section.visible_properties:
        # Entries in the device section are named after protocol names. Configuration parameters
        # under an unsupported protocol name cannot be used by the device, so we remove them.
        if property.name not in supported_protocols:
            device_section.remove_property(property.name)

def filter_streaming_section(config, supported_protocols):
    # Retrieve Streaming section of the configuration tree
    streaming_section = config.get_property_value("Streaming")
    for property in streaming_section.visible_properties:
        # Entries in the streaming section are named after protocol names. Configuration parameters
        # under an unsupported protocol name cannot be used by the device, so we remove them.
        if property.name not in supported_protocols:
            streaming_section.remove_property(property.name)

def filter_general_section(config, device_info, supported_protocols):
    general_section = config.get_property("General")
    hidden_property_paths = []

    def add_to_hidden(section: daq.IProperty, properties):
        for p in properties:
            if section.value.has_property(p):
                hidden_property_paths.append(f"{section.name}.{p}")

    # These properties can only be used if the device supports OpenDAQNative protocols.
    if "OpenDAQNativeConfiguration" not in supported_protocols:
        add_to_hidden(general_section, ["Username", "Password", "ClientType", "ExclusiveControlDropOthers"])

    # Find TCP/IP and streaming capabilities
    has_tcp_ip = False
    has_streaming = False
    for c in device_info.server_capabilities:
        if c.get_property_value("ConnectionType") == "TCP/IP":
            has_tcp_ip = True
        if c.get_property_value("ProtocolType") in ["Streaming", "ConfigurationAndStreaming"]:
            has_streaming = True

    # Remove if device doesn't provide TCP/IP connections
    if not has_tcp_ip:
        add_to_hidden(general_section, ["PrimaryAddressType"])

    # Remove streaming related settings if device is not capable of streaming connections.
    if not has_streaming:
        add_to_hidden(
            general_section,
            ["StreamingConnectionHeuristic",
                "PrioritizedStreamingProtocols",
                "AllowedStreamingProtocols",
                "AutomaticallyConnectStreaming"])

    return hidden_property_paths

if __name__ == "__main__":
    # Setup a device simulator that can be discovered via opendaq
    device_name = 'Filtering Device Simulator1'
    _sim = daq_utils.setup_simulator(
        name=device_name,
        local_id='FilterDevSimulator',
        serial_number='sim02',
        protocols=['OpenDAQNativeStreaming', 'OpenDAQLTStreaming', 'OpenDAQOPCUA'])

    # Create an OpenDAQ instance and get available devices
    instance = daq.Instance()
    available_devices = instance.available_devices

    # Select the device we created and get its info object
    device_info = None
    for dev_ in available_devices:
        if dev_.name == device_name:
            device_info = dev_
            break

    # From device info extract supported protocols.
    supported_protocols = compute_supported_protocols(device_info, instance)

    # Request the default configuration
    configuration = instance.create_default_add_device_config()

    # Default configuration has three sections: Device, General and Streaming.

    # Device section includes configuration entries for all connection types
    # available in openDAQ modules.
    filter_device_section(configuration, supported_protocols)

    # Streaming section includes configuration entries for all streaming connection types
    # available in openDAQ modules.
    filter_streaming_section(configuration, supported_protocols)

    # General section includes configuration entries from all available modules.
    hidden_properties = filter_general_section(configuration, device_info, supported_protocols)

    print("Filtered configuration:")
    # Inspect the results of the filtering by printing the config to the command line.
    # By changing the list of protocols ins the call to setup_simulator, we can observe how
    # different device capabilities affect filtering of the add_device_config.
    daq_utils.print_property_object(configuration, 0, hidden_properties)
