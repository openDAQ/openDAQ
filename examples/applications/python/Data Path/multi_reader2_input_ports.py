##
# MultiReader2, examples E4 and E5 from an application: inputs built from input ports.
#
# The reader is the only listener of the ports it reads, so the Disconnected state arrives through read like every
# other input state. Two patterns:
#   - two fixed ports that both have to deliver (E5): a pinned main port, Invalidate, reading in a loop,
#   - ports added one by one (E4): the application connects a signal to a new port, adds the port to the reader
#     and lets the reader judge the descriptor through accepts_descriptor; a port that loses its signal is removed.
##

import time

import opendaq as daq

instance = daq.Instance()
device = instance.add_device('daqref://device0')
signals = [s for s in device.get_signals_recursive() if s.domain_signal is not None]
context = instance.context

# --- E5: two fixed ports, pinned main, loop ------------------------------------------------------------

voltage_port = daq.InputPort(context, None, 'Voltage', False)
current_port = daq.InputPort(context, None, 'Current', False)
params = daq.MultiReader2Params()
params.inputs = [voltage_port, current_port]
params.main_input = voltage_port                        # pinned: its error invalidates the reader
reader = daq.MultiReader2(params)

voltage_port.connect(signals[0])
deadline = time.time() + 2
while time.time() < deadline:
    count = reader.available_count
    values, offset, status = reader.read(count)
    count = len(values[0])  # the count read; 0 when changes are reported first
    if count > 0:
        print(f'power sample {offset}: {values[0][0] * values[1][0]:.3f}')
    if status.has_changes:
        for s in status.inputs:
            print(f'  {s.input.name}: {s.error}')      # Current shows Disconnected until it connects
        if not status.valid:
            print('  waiting for both ports')
    elif count == 0:
        time.sleep(0.01)
    if time.time() > deadline - 1 and current_port.signal is None:
        current_port.connect(signals[1])                 # the error clears and the reader recovers

# --- E4: ports added one by one, descriptors judged by the reader --------------------------------------

ports = []
params = daq.MultiReader2Params()
params.inputs = []                                       # a reader without inputs is valid and reads nothing
params.accepts_descriptor = lambda port, descriptor, domain: descriptor.sample_type == daq.SampleType.Float64
reader = daq.MultiReader2(params)


def attach(signal):
    """Connects the signal to a new port and hands the port to the reader; the reader judges the descriptor."""
    port = daq.InputPort(context, None, f'Input{len(ports) + 1}', False)
    port.connect(signal)
    ports.append(port)
    params.inputs = ports
    reader.configure(params)


def on_changes(status):
    """Returns True when the reader was reconfigured."""
    for s in status.inputs:
        if s.error == daq.MultiReader2InputError.Disconnected:
            print(f'{s.input.name} lost its signal')
            ports.remove(next(p for p in ports if p.global_id == s.input.global_id))
            params.inputs = ports
            reader.configure(params)
            return True
        if s.error != daq.MultiReader2InputError.None_:
            print(f'{s.input.name}: set aside, {s.error}')
    return False


def drain():
    while True:
        count = reader.available_count
        values, offset, status = reader.read(count)
        count = len(values[0]) if values else 0  # the count read; 0 when changes are reported first
        if count > 0:
            used = [v for v, s in zip(values, status.inputs) if s.error == daq.MultiReader2InputError.None_]
            print(f'sum at {offset}: {sum(v[0] for v in used):.3f} over {len(used)} inputs')
        if status.has_changes:
            if on_changes(status):
                return
        elif count == 0:
            return


for signal in signals[0:3]:
    attach(signal)
    time.sleep(0.3)
    drain()                                              # sees the new input, judged on the way in
time.sleep(0.3)
drain()
ports[1].disconnect()
time.sleep(0.3)
drain()                                                  # sees the disconnect, removes the port
