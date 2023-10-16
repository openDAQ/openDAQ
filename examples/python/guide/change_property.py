import opendaq

instance = opendaq.Instance()
device = instance.add_device('daqref://device0')
channel = device.channels[0]

print(channel.get_property_value('Amplitude'))
channel.set_property_value('Amplitude', 5)
print(channel.get_property_value('Amplitude'))
