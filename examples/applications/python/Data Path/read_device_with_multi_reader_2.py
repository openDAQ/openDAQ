##
# Starting point for reading a device's signals with MultiReader2: connect to the device, pick the signals and read
# them aligned on one time grid, by polling or from the reader's on_data_available handler.
#
# Runs against the reference device by default. Pass your own device's connection string to read from it:
#   python read_device_with_multi_reader_2.py [connection_string] [--handler] [--seconds N]
#
# The places to adapt for your device and application are marked with ADAPT.
##

import argparse
import threading
import time

import numpy as np
import opendaq as daq


def pick_signals(device):
    '''
    ADAPT: the signals to read. Here, the first value signal of every channel; a value signal is one with a domain
    (time) signal. MultiReader2 reads signals of equal sample rate whose domain is linear (implicit time stamps).
    '''
    signals = []
    for channel in device.channels:
        for signal in channel.signals:
            if signal.domain_signal is not None:
                signals.append(signal)
                break
    return signals


def accepts_descriptor(signal, descriptor, domain_descriptor):
    '''
    ADAPT: which inputs the application can handle; a rejected input is reported in the status. Here, scalar values
    only. Runs inside the reader's own calls: keep it quick, and never call the reader from here.
    '''
    return len(descriptor.dimensions) == 0


# ADAPT: the reader's settings, named as the params' fields. Exclude keeps reading the healthy inputs when one is
# rejected or fails; Invalidate (the default) stops all of them until it recovers. The first input is the main input:
# its time stamps are the ones a read returns
SETTINGS = dict(value_read_type=daq.SampleType.Float64,
                error_policy=daq.MultiReader2ErrorPolicy.Exclude,
                accepts_descriptor=accepts_descriptor)


def process(r):
    '''
    ADAPT: what the application does with one read.
    '''
    # A status with changes says that something about the inputs changed: descriptors, errors, a resynchronization.
    # The samples of this read still follow the previous descriptors
    if r.status.has_changes:
        print('status:', r.status)
        for signal, error in r.status.errors.items():
            print(f'  {signal.local_id}: {error}')

        # New descriptors apply from the next read on. ADAPT: keep what the application needs from them, such as
        # units for labels or a scale
        if r.status.domain_descriptor_changed:
            domain = r.status.domain_descriptor
            print(f'  time domain: tick resolution {domain.tick_resolution}, origin {domain.origin}')
        for input_status in r.status.inputs:
            if input_status.descriptor_changed:
                unit = input_status.descriptor.unit
                symbol = unit.symbol if unit is not None else 'none'
                print(f'  {input_status.input.local_id}: {input_status.descriptor.sample_type}, unit {symbol}')

    # One array per input, in the reader's input order; None for an excluded input. Sample i of every array belongs
    # to the same time stamp, r.times[i]
    if r.count > 0:
        means = ', '.join(f'{signal.local_id} {np.mean(values):.3f}'
                          for signal, values in zip(r.inputs, r.values) if values is not None)
        print(f'{r.count} samples from {r.times[0]}; mean {means}')


def read_by_polling(signals, seconds):
    '''
    Reads everything available every 500 ms. drain() reads until it has caught up; it never waits.
    '''
    reader = daq.MultiReader2(signals, **SETTINGS)
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        for r in reader.drain():
            process(r)
        time.sleep(0.5)


def read_from_handler(signals, seconds):
    '''
    Reads whenever the reader announces data, on a scheduler thread. The handler is set at construction: a later
    configure is a fresh start that drops the queued data. Every reader call goes under the same lock as the
    handler's drain, so a configure can't land in the middle of it.
    '''
    lock = threading.Lock()
    reader = None

    def on_data_available():
        with lock:
            for r in reader.drain():
                process(r)

    with lock:  # a handler call that comes early waits here until reader is set
        reader = daq.MultiReader2(signals, on_data_available=on_data_available, **SETTINGS)
    on_data_available()  # the first status: the reader announces data, not the status it owes from construction
    time.sleep(seconds)
    with lock:
        reader.configure(on_data_available=None)  # removes the handler


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Reads a device\'s signals aligned with MultiReader2.')
    parser.add_argument('connection_string', nargs='?', default='daqref://device0',
                        help='the device to read from (default: the reference device)')
    parser.add_argument('--handler', action='store_true', help='read from on_data_available instead of polling')
    parser.add_argument('--seconds', type=float, default=10.0, help='how long to read (default: 10)')
    args = parser.parse_args()

    instance = daq.Instance()
    device = instance.add_device(args.connection_string)
    signals = pick_signals(device)
    print('Reading', ', '.join(signal.local_id for signal in signals))

    if args.handler:
        read_from_handler(signals, args.seconds)
    else:
        read_by_polling(signals, args.seconds)
