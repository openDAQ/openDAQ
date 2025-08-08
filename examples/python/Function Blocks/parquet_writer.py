#!/usr/bin/env python

##
# This script demonstrates how to use the ParquetRecorder function block
# in Python to record data from a device's signal to a Parquet
# file.
##

import opendaq as daq
import time
import os

if __name__ == '__main__':
    instance: daq.IInstance = daq.Instance()
    # Here we use a reference device as a data source
    device: daq.IDevice = instance.add_device(
        instance.available_devices[0].connection_string)
    print(f'Connected to device: {device.name}')

    # We create a ParquetRecorder function block through the instance
    # as a shortcut as the instance has module manager to load the module containing the function block
    # and also has a scheduler that is neccessary for the function block to work
    # The recorder will post writting tasks to the scheduler
    # and will write the data to the Parquet file in the background
    recoder_fb: daq.IFunctionBlock = instance.add_function_block(
        'ParquetRecorder')
    recoder: daq.IRecorder = daq.IRecorder.cast_from(recoder_fb)
    print(f'Created function block: {recoder_fb.name}')

    # Here we connect an input port of the recorder to a signal of the device
    # New input port is created every time we connect the signal to the recorder
    input_port: daq.IInputPort = recoder_fb.input_ports[0]
    signal: daq.ISignal = device.channels_recursive[0].signals[0]
    input_port.connect(signal)
    print(f'Connected input port: {input_port.name} to signal: {signal.name}')

    # The recorder has the only one property now - 'Path'
    # which is the directory where the Parquet file will be saved
    recoder_fb.set_property_value('Path', '.')
    print(f'Set recording path to: {recoder_fb.get_property_value("Path")}')

    # We call the start_recording method of the IRecorder interface that is implemented by the ParquetRecorder function block
    recoder.start_recording()
    print('Started recording...')

    # wait for 5 sec
    print('Recording for 5 seconds...')
    time.sleep(5)

    recoder.stop_recording()
    print('Stopped recording.')

    # find and list all the parquet files in the recording path
    parquet_files = [f for f in os.listdir(recoder_fb.get_property_value('Path')) if f.endswith('.parquet')]
    if not parquet_files:
        print('No parquet files found in the recording path.')
    else:
      print(f'Found parquet files:')
      for file in parquet_files:
          print(f' - {file}')
