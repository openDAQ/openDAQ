import opendaq

instance = opendaq.Instance()
device = instance.add_device('daqref://device0')

signals = device.signals_recursive
for signal in signals:
    print(signal.descriptor.name)
