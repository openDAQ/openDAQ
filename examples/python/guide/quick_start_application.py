import opendaq
import time

# Create a fresh openDAQ(TM) instance that we will use for all the interactions with the openDAQ(TM) SDK
instance = opendaq.Instance()

# Find and connect to a simulator device
for device_info in instance.available_devices:
    if device_info.name == 'Reference device simulator':
        device = instance.add_device(device_info.connection_string)
        break
else:
    # Exit if no device is found
    exit(0)

# Output the name of the added device
print(device.info.name)

# Get the first signal of the first device's channel
channel = device.channels[0]
signal = channel.signals[0]

# Print out the last value of the signal
print(signal.last_value)

reader = opendaq.StreamReader(signal, value_type=opendaq.SampleType.Float64)

# Output 40 samples using reader
for cnt in range (0, 40):
    time.sleep(0.025)
    # Read up to 100 samples and print the last one
    samples = reader.read(100)
    if len(samples) > 0:
        print(samples[-1])
        
# Get the resolution, origin, and unit
descriptor = signal.domain_signal.descriptor
resolution = descriptor.tick_resolution
origin = descriptor.origin
unit_symbol = descriptor.unit.symbol

print('Origin:', origin)

for i in range (0, 40):
    time.sleep(0.025)

    # Read up to 100 samples
    samples, domain_samples = reader.read_with_domain(100)

    # Scale the domain values to the Signal unit (seconds)
    domain_values = domain_samples * float(resolution)
    if len(samples) > 0:
        print('Value:', samples[-1], ', Domain:', domain_values[-1], unit_symbol)
        
# Create a Time Stream Reader that outputs domain values in the datetime format
time_reader = opendaq.TimeStreamReader(reader)

for i in range (0, 40):
    time.sleep(0.025)
    # Read up to 100 samples and print the last one
    samples, time_stamps = time_reader.read_with_timestamps(100)
    if len(samples) > 0:
        print(f'Value: {samples[-1]}, Domain: {time_stamps[-1]}')

# Create an instance of the renderer function block
renderer = instance.add_function_block('ref_fb_module_renderer')
# Connect the first output signal of the device to the renderer
renderer.input_ports[0].connect(signal)

# Create an instance of the statistics function block
statistics = instance.add_function_block('ref_fb_module_statistics')
# Connect the first output signal of the device to the statistics
statistics.input_ports[0].connect(signal)
# Connect the first output signal of the statistics to the renderer
renderer.input_ports[1].connect(statistics.signals[0])

# List the names of all properties
for prop in channel.visible_properties:
    print(prop.name)

# Set the frequency to 5 Hz
channel.set_property_value('Frequency', 5)
# Set the noise amplitude to 0.75
channel.set_property_value('NoiseAmplitude', 0.75)

# Modulate the signal amplitude by a step of 0.1 every 25 ms.
amplitude_step = 0.1
for i in range (0, 200):
    time.sleep(0.025)
    amplitude = channel.get_property_value('Amplitude')
    if not (1.05 <= amplitude <= 9.95):
        amplitude_step = -amplitude_step
    channel.set_property_value('Amplitude', amplitude + amplitude_step)