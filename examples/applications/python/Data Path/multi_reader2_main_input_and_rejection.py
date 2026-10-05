##
# MultiReader2, example E3: constant inputs, Invalidate error policy, reading from the callback, built from
# signals, value descriptors rejected by unit, main input chosen through a property.
#
# Rejecting a descriptor is the owner's job: the reader reports every descriptor change, unused inputs included,
# and the application sets an input unused when it rejects the descriptor and used again when a later descriptor
# is acceptable. A settings object lets the user pick the main input; a pinned main input is left alone by the
# rejection logic and its error invalidates the reader.
##

import threading
import time

import opendaq as daq

instance = daq.Instance()
device = instance.add_device('daqref://device0')
signals = [s for s in device.get_signals_recursive() if s.domain_signal is not None][0:3]

params = daq.MultiReader2Params()
params.inputs = signals
reader = daq.MultiReader2(params)
lock = threading.Lock()

settings = daq.PropertyObject()
settings.add_property(daq.SelectionProperty('MainInput', [s.name for s in signals], 0, True))


def apply_settings(sender, args):
    with lock:
        params.main_input = signals[settings.get_property_value('MainInput')]   # pinned: its error invalidates
        reader.configure(params)


settings.get_on_property_value_write('MainInput') + daq.EventHandler(apply_settings)


def accepts(descriptor):
    return descriptor.unit is not None and descriptor.unit.symbol == 'V'


def apply_rejections(status):
    """A rejected input goes unused and comes back when its descriptor changes to one we accept.
    Returns True when the reader was reconfigured."""
    if not status.valid:
        return False
    main = params.main_input
    changed = False
    for s in status.inputs:
        if not s.descriptor_changed or (main is not None and s.input.global_id == main.global_id):
            continue
        want_used = accepts(s.descriptor)
        if s.used != want_used:
            print(f'{"using" if want_used else "rejecting"} {s.input.global_id}, unit {s.descriptor.unit}')
            params.set_input_used(s.input, want_used)
            changed = True
    if changed:
        reader.configure(params)
    return changed


def handle(status):
    if not status.valid:
        print('waiting: ' + ', '.join(f'{s.input.global_id} {s.error}' for s in status.inputs if s.error != daq.MultiReader2InputError.None_))
    elif status.domain_descriptor_changed:
        print(f'main input is {reader.main_input}')


def on_data_available(sender, args):
    with lock:
        while True:
            count = reader.available_count
            values, offset, status = reader.read(count)
            count = len(values[0])  # the count read; 0 when changes are reported first
            if status.has_changes:
                if apply_rejections(status):
                    return                               # reconfigured; start over on the next wake
                handle(status)
            elif count == 0:
                return


reader.on_data_available + daq.EventHandler(on_data_available)

time.sleep(1)
settings.set_property_value('MainInput', 1)              # pin the second channel as the main input
time.sleep(1)
