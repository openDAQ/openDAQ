##
# MultiReader2, example E2: constant inputs, Exclude error policy, reading from the onDataAvailable callback,
# built from signals, automatic main input.
#
# Under Exclude the reader sets an erroring input aside, keeps delivering the others, and takes the input back when
# it is healthy again. When the main input errors the reader moves the main input itself; the status then shows a
# domain change and a resynchronization. The callback reads until nothing is deliverable and nothing changed.
# A consumer reads in a loop or from the callback, never both.
##

import threading
import time

import opendaq as daq

instance = daq.Instance()
device = instance.add_device('daqref://device0')
signals = [s for s in device.get_signals_recursive() if s.domain_signal is not None][0:3]

params = daq.MultiReader2Params()
params.inputs = signals
params.error_policy = daq.MultiReader2ErrorPolicy.Exclude
reader = daq.MultiReader2(params)

samples_read = 0
lock = threading.Lock()


def handle(status):
    if not status.valid:
        print('no used input is healthy')
        return
    for s in status.inputs:
        if s.error != daq.MultiReader2InputError.None_:
            print(f'set aside: {s.input.global_id} is {s.error}')  # the reader stays valid
    if status.resynchronized:
        print(f'resynchronized; main input is now {reader.main_input}')


def on_data_available(sender, args):
    global samples_read
    with lock:
        while True:
            count = reader.available_count
            values, offset, status = reader.read(count)
            count = len(values[0])  # the count read; 0 when changes are reported first
            if count > 0:
                samples_read += count
            if status.has_changes:
                handle(status)
            elif count == 0:
                return


reader.on_data_available + daq.EventHandler(on_data_available)

time.sleep(2)
# Pulling the main channel out of the reader's reach: the reader excludes it and moves the main input
channel = device.channels[0]
channel.set_property_value('UseGlobalSampleRate', False)
channel.set_property_value('SampleRate', 500)                # another rate: DomainDescriptorInvalid on channel 0
time.sleep(2)
channel.set_property_value('UseGlobalSampleRate', True)      # back at the common rate: it rejoins
time.sleep(2)
print(f'samples read: {samples_read}')
