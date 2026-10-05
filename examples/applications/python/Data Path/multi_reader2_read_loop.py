##
# MultiReader2, example E1: constant inputs, Invalidate error policy, reading in a loop, built from signals,
# automatic main input.
#
# An application reads three reference device channels in its own loop and waits out errors. A read never waits
# for data: when nothing is available the loop sleeps. Every read returns one status; the data belongs to the
# descriptors reported on earlier statuses and changes apply from the next read on, so the loop processes the
# samples first and handles the status after.
##

import time

import opendaq as daq

instance = daq.Instance()
device = instance.add_device('daqref://device0')
signals = [s for s in device.get_signals_recursive() if s.domain_signal is not None][0:3]

params = daq.MultiReader2Params()
params.inputs = signals
params.value_read_type = daq.SampleType.Float64         # the default; Invalidate is the default error policy
reader = daq.MultiReader2(params)

cached = {}                                              # value descriptors by global id, as last reported
domain = None


def process(values, offset):
    print(f'{offset}: ' + ', '.join(f'{v[0]:.3f}' for v in values))


def handle(status):
    """On a status with changes: wait while invalid, cache while valid. What is cached applies from the next read."""
    global domain
    if not status.valid:
        for s in status.inputs:
            if s.error != daq.MultiReader2InputError.None_:
                print(f'waiting: {s.input.global_id} is {s.error}')
        return
    if status.resynchronized:
        print('resynchronized: the next offset may jump')
    if status.domain_descriptor_changed:
        domain = status.domain_descriptor
        print(f'domain: origin {domain.origin}, resolution {domain.tick_resolution}')
    for s in status.inputs:
        if s.descriptor_changed:
            cached[s.input.global_id] = s.descriptor
            print(f'descriptor: {s.input.global_id} -> {s.descriptor.sample_type}, unit {s.descriptor.unit}')


deadline = time.time() + 5
while time.time() < deadline:
    count = reader.available_count
    values, offset, status = reader.read(count)
    count = len(values[0])  # the count read; 0 when changes are reported first
    if count > 0:
        process(values, offset)                          # under the cached descriptors; row i is one main tick
    if status.has_changes:
        handle(status)
    elif count == 0:
        time.sleep(0.01)                                 # a read never waits for data
