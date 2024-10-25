import opendaq
import matplotlib.pyplot as plt
import time

instance = opendaq.Instance()
device = instance.add_device('daqref://device0')
signal = device.signals_recursive[0]

builder = opendaq.StreamReaderBuilder()
builder.signal = signal
builder.skip_events = True
reader = builder.build()

time.sleep(1.0)
samples = reader.read(1000)
plt.plot(samples)
plt.show()
