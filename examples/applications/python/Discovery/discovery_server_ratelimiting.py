##
# Creates a temporary json file to be used as a Json config provider for creating the openDAQ
# instance. The file contains reference configuration parameters for mDNS server ratelimiting.
#
# Starts an openDAQ simulator. The simulator uses the reference device as its root and
# starts up the OPC UA and Native servers with "mdns" discovery enabled. The serial number
# and local ID of the reference device are overridden to "sim01" and "RefDevSimulator"
# respectively.
#
# IMPORTANT: Only 1 simulator should be active at any given time to avoid serial number
# clashes during discovery and connection! openDAQ devices should generally have a globally
# unique combination of manufacturer + serial number, and a globally unique local ID.
##

import json
from pathlib import Path
import opendaq

CONFIG_FILE = Path("mdns-discovery-ratelimiting.json")

config = opendaq.PropertyObject()
config.add_property(opendaq.StringProperty(opendaq.String(
    'Name'), opendaq.String('Reference device simulator'), opendaq.Boolean(True)))
config.add_property(opendaq.StringProperty(opendaq.String(
    'LocalId'), opendaq.String('RefDevSimulator'), opendaq.Boolean(True)))
config.add_property(opendaq.StringProperty(opendaq.String(
    'SerialNumber'), opendaq.String('sim01'), opendaq.Boolean(True)))

# Create a new openDAQ(TM) Json config provider file
# Fail immediately if file exists
if CONFIG_FILE.exists():
    print(f"Error: file already exists: {CONFIG_FILE}")
    exit(1)

try:
    with CONFIG_FILE.open("w", encoding="utf-8") as f:
        json.dump(
            {
                "MdnsDiscoveryServer":
                {
                    "EnableDiscoveryRateLimit": True,
                    "SingleQuerierRateLimitPerSecond": 30,
                    "MaxActiveQueriers": 200,
                    "MaxQueryCountPerSecond": 80
                }
            },
            f,
            indent=2
        )

except Exception as e:
    print(f"Error: failed to create config file: {e}")
    exit(1)

# Read from file and create config provider
config_provider = opendaq.JsonConfigProvider(opendaq.String(str(CONFIG_FILE)))

# Create a new openDAQ(TM) instance builder
instance_builder = opendaq.InstanceBuilder()

# Add Mdns discovery service available for servers
instance_builder.add_discovery_server("mdns")

# Set config provider
instance_builder.add_config_provider(config_provider)

# Set reference device as a root
instance_builder.set_root_device('daqref://device0', config)

# until instance builder factory gets updated
instance_builder.module_path = opendaq.OPENDAQ_MODULES_DIR

# Creating a new instance from builder
instance = instance_builder.build()

# cleanup created file
CONFIG_FILE.unlink()

# Start an openDAQ OpcUa and native streaming servers
servers = instance.add_standard_servers()

# Enable discovery for all servers
for server in servers:
    server.enable_discovery()

input('Press "Enter" to exit the application ...')
