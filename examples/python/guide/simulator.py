import opendaq

config = opendaq.PropertyObject()
config.add_property(opendaq.StringProperty(opendaq.String(
    'Name'), opendaq.String('Reference device simulator'), opendaq.Boolean(True)))
config.add_property(opendaq.StringProperty(opendaq.String(
    'LocalId'), opendaq.String('RefDevSimulator'), opendaq.Boolean(True)))
config.add_property(opendaq.StringProperty(opendaq.String(
    'SerialNumber'), opendaq.String('sim01'), opendaq.Boolean(True)))
    
# Create a new openDAQ(TM) instance builder
instance_builder = opendaq.InstanceBuilder()

# Add Mdns discovery service available for servers
instance_builder.add_discovery_server("mdns")

# Set reference device as a root
instance_builder.set_root_device('daqref://device0', config)

# until instance builder factory gets updated
instance_builder.module_path = opendaq.OPENDAQ_MODULES_DIR

# Creating a new instance from builder
instance = instance_builder.build()

# Start an openDAQ OpcUa and native streaming servers
servers = instance.add_standard_servers()

# Enable discovery for all servers
for server in servers:
    server.enable_discovery()

input('Press "Enter" to exit the application ...')