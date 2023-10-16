import opendaq

# Create openDAQ instance
instance = opendaq.Instance()

# Add a reference device and set it as root
instance.set_root_device('daqref://device0')

# Start an openDAQ OpcUa and native streaming server
instance.add_standard_servers()

print("\n")
input('Press "Enter" to exit the application ...')
