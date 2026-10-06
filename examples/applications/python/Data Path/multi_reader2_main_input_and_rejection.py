##
# MultiReader2, example E3: constant inputs, Exclude error policy, reading from the callback, built from
# signals, value descriptors judged by unit, main input chosen through a property.
#
# The owner states its criterion once, as accepts_descriptor on the params: the reader asks it whenever a
# descriptor takes effect, inside read and configure on the owner's thread. A rejected input is in error
# (ValueDescriptorInvalid) like an unreadable one: under Exclude it is set aside and delivers nothing, and it is
# asked again when its descriptor changes. The judgement must not call the reader. A settings object lets the
# user pick the main input; a pinned main input that the owner rejects invalidates the reader.
##

import threading
import time

import opendaq as daq

instance = daq.Instance()
device = instance.add_device('daqref://device0')
signals = [s for s in device.get_signals_recursive() if s.domain_signal is not None][0:3]


def accepts(signal, descriptor, domain_descriptor):
    return descriptor.unit is not None and descriptor.unit.symbol == 'V'


params = daq.MultiReader2Params()
params.inputs = signals
params.error_policy = daq.MultiReader2ErrorPolicy.Exclude   # a rejected or erroring input is set aside
params.accepts_descriptor = accepts
reader = daq.MultiReader2(params)
lock = threading.Lock()

settings = daq.PropertyObject()
settings.add_property(daq.SelectionProperty('MainInput', [s.name for s in signals], 0, True))


def apply_settings(sender, args):
    with lock:
        params.main_input = signals[settings.get_property_value('MainInput')]   # pinned: its error invalidates
        reader.configure(params)


settings.get_on_property_value_write('MainInput') + daq.EventHandler(apply_settings)


def handle(status):
    if not status.valid:
        print('waiting: ' + ', '.join(f'{s.input.global_id} {s.error}' for s in status.inputs if s.error != daq.MultiReader2InputError.None_))
    else:
        for s in status.inputs:
            if s.descriptor_changed:
                print(f'using {s.input.global_id}, unit {s.descriptor.unit}')
            elif s.error != daq.MultiReader2InputError.None_:
                print(f'set aside {s.input.global_id}: {s.error}')
        if status.domain_descriptor_changed:
            print(f'main input is {reader.main_input}')


def on_data_available(sender, args):
    with lock:
        while True:
            count = reader.available_count
            values, offset, status = reader.read(count)
            count = len(values[0])  # the count read; 0 when changes are reported first
            if status.has_changes:
                handle(status)
            elif count == 0:
                return


reader.on_data_available + daq.EventHandler(on_data_available)

time.sleep(1)
settings.set_property_value('MainInput', 1)              # pin the second channel as the main input
time.sleep(1)
