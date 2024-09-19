import opendaq as daq

instance = daq.Instance()

dev = instance.add_device('daqref://device0')
dev.set_property_value('EnableCANChannel', True)

# filter signals by name
can_signal = list(
    filter(
        lambda s: s.name == 'CAN',
        list(
            dev.signals_recursive)))[0]

reader = daq.StreamReader(can_signal, value_type=daq.SampleType.Undefined)

# fetching descriptors
values, status = reader.read(0, return_status=True)

# getting data arrays
values, status = reader.read(4, return_status=True)
for i in range(len(values)):
    length = values['Length'][i]
    arbId = values['ArbId'][i]
    print(f'ArbId: {arbId} Length: {length} Data: {
          values['Data'][i][:length]}')
