import numpy as np
import opendaq
import time

input_file = "C:/Users/jakob/OneDrive/Desktop/example.wav"

# Create an openDAQ instace.
instance = opendaq.Instance()

# Add both the reader and writer
# The writer has both the IFunctionBlock and IRecorder interface
reader_fb = instance.add_function_block("AudioDeviceModuleWavReader")
writer_fb = instance.add_function_block("AudioDeviceModuleWavWriter")
writer_re = opendaq.IRecorder.cast_from(writer_fb)

writer_fb.input_ports[0].connect(reader_fb.signals[0])
reader_fb.set_property_value("FilePath", input_file)

# The writer writes output files into the working directory folder
writer_fb.set_property_value("FileName", "output.wav")

# Start reading before writing otherwise the writer fails with no signal
reader_fb.set_property_value("Reading", True)
writer_re.start_recording()

# You can check whether an IRecorder is currently recording, in python this is a member variable, in C++ it's a method
print("\nWriter recording: {}\n".format(writer_re.is_recording))
time.sleep(1)

# Stop writing before reading
writer_re.stop_recording()
reader_fb.set_property_value("Reading", False)
