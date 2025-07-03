import opendaq

instance = opendaq.Instance()
device = instance.add_device('daqref://device0')
channel = device.channels[0]

channel_properties = channel.visible_properties
for property in channel_properties:
    print(property.name)
