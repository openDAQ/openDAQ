import opendaq
import time
import numpy as np
import matplotlib.pyplot as plt

instance = opendaq.Instance()

# discover devices select one to connect to
available_devices_info = instance.available_devices
print("Available devices- Select one to connect to:")
i = 0
for device_info in available_devices_info:
    print(i, ': ', 'Name:', device_info.name, 'Connection string:', device_info.connection_string)
    i += 1
count = len(available_devices_info)
device_index = int(input("Enter the index of the device you want to connect to: "))
if device_index >= count:
    print("Invalid index")
    exit()
device_info = available_devices_info[device_index]
device = instance.add_device(device_info.connection_string)
print('\n')
print("Connected to device:", device.info.name)

# display signals
signals = device.signals_recursive
print("Signals:")
for signal in signals:
    print(signal.descriptor.name)
count = len(signals)
if count == 0:
    print("No signals available")
    exit()
signal_index = int(input("Enter the index of the signal you want to read: "))
if signal_index >= count:
    print("Invalid index")
    exit()
signal = signals[signal_index]
print("Reading signal:", signal.descriptor.name, '\n')

# fill signal_values with samples from signal
signal_values = np.array([])
reader = opendaq.StreamReader(signal)
start_time = time.time()
while time.time() - start_time <= 10:
    # trys to read 100 samples every 0.1 second meaning around 10000 samples in 10 seconds
    samples = reader.read(100, 100)
    signal_values = np.concatenate((signal_values, samples))

# plot signal values over time graph
time_axis = np.linspace(0, 10, len(signal_values))
plt.figure()
plt.plot(time_axis, signal_values)
plt.xlabel('Time (s)')
plt.ylabel('Signal value')
plt.title(f'Signal values over 10 seconds: {signal.descriptor.name}')
plt.show()

# calculate avg, min, max, rms
avg = np.mean(signal_values)
min_val = np.min(signal_values)
max_val = np.max(signal_values)
rms = np.sqrt(np.mean(signal_values ** 2))
print(f'Average value: {avg}')
print(f'Maximum value: {max_val}')
print(f'Minimum value: {min_val}')
print(f'Mean root square value: {rms}')

# close the device
instance.remove_device(device)
