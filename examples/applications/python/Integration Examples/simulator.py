##
# @tags: integration, servers, simulator
# @title: Start an AI Signal Simulator
##
# The simulator device module creates a configurable signal generator that
# other openDAQ applications can discover and connect to over the network.
#
# Device identity (name, manufacturer, serial number) comes from module
# options on the context. Defaults: Name="Simulator", Manufacturer="openDAQ",
# SerialNumber=<your hostname>.
#
# Channel count is configurable through the device type's default config.
# Default is 8 channels.
#
# Only run one simulator at a time. Two simulators with the same
# manufacturer + serial number pair will confuse discovery.
##

import os
import opendaq

if not hasattr(opendaq, "OPENDAQ_MODULES_DIR"):
    opendaq.OPENDAQ_MODULES_DIR = os.getcwd()

# module_path tells the SDK where to find compiled module DLLs
# (protocol servers, device modules, function blocks). The mDNS
# discovery server lets clients find this simulator on the network.
instance_builder = opendaq.InstanceBuilder()
instance_builder.module_path = opendaq.OPENDAQ_MODULES_DIR
instance_builder.add_discovery_server("mdns")
instance = instance_builder.build()

# Check which simulator is available and get its default config.
device_types = instance.available_device_types
has_simulator = any(
    opendaq.IDeviceType.cast_from(dt).connection_string_prefix == "daq.simulator"
    for dt in device_types.values()
)

if has_simulator:
    sim_type = opendaq.IComponentType.cast_from(device_types["SimulatorDevice"])
    config = sim_type.create_default_config()
    config.set_property_value("NumberOfChannels", opendaq.Integer(4))
    instance.set_root_device("daq.simulator://", config)
else:
    # Fall back to the reference device if the simulator module
    # isn't built or loaded.
    instance.set_root_device("daqref://device0")

# Start OPC UA and Native Streaming servers so clients can connect
# with either protocol. enable_discovery hooks each server into
# the mDNS service; without it, the simulator runs but nobody
# can find it automatically.
servers = instance.add_standard_servers()
for server in servers:
    server.enable_discovery()

print("Simulator running. Clients can discover and connect.")
input('Press "Enter" to exit the application ...')