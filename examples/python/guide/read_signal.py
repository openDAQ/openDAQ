import opendaq
import time

instance = opendaq.Instance()
device = instance.add_device('daqref://device0')
signal = device.signals_recursive[0]

reader = opendaq.StreamReader(signal)

# Output 10 samples using reader
for i in range(10):
    time.sleep(0.5)
    samples = reader.read(100)
    if len(samples) > 0:
        print(samples[-1])
