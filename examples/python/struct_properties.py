import opendaq as daq

# connecting to reference device
inst = daq.Instance()
device = inst.add_device('daqref://device0')

# getting custom range property of device's channel
ch: daq.IChannel = device.channels[0]
custom_range = ch.get_property_value('CustomRange')

# printing all fields (attributes) and their values
for field in dir(custom_range):
    print(f'{field}: {getattr(custom_range, field)}')

# accessing field by name
print(f'HighValue: {custom_range.HighValue}')

# structs are immutable, so we need to create new struct
bldr = daq.StructBuilderFromStruct(custom_range)
bldr.set('HighValue', 15.0)
bldr.set('LowValue', -1.0)
struct = bldr.build()

# CustomRange is a read-only property, so we make a copy of it in root device
inst.add_property(daq.StructProperty('CustomRange', custom_range, True))

# updating struct property and getting new value back
inst.set_property_value('CustomRange', bldr.build())
custom_range = inst.get_property_value('CustomRange')

print(f'Updated LowValue: {custom_range.LowValue}')
print(f'Updated HighValue: {custom_range.HighValue}')
