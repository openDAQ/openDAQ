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

# Output 10 samples with forced type
reader = opendaq.StreamReader(signal, value_type=opendaq.SampleType.Int64)
time.sleep(0.5)
for overriden_type_value in reader.read(10):
    print(overriden_type_value)

# Output 10 timestamped samples with forced type
values, timestamps = opendaq.TimeStreamReader(reader).read_with_timestamps(10)
for value, timestamp in zip(values, timestamps):
    print(timestamp, value)
