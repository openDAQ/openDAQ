##
# Example that reads three synthetic signals with MultiReader2 and shows each operation of its Python binding:
# construction with keyword arguments, the read result, the main domain as ticks and times, reading into your own
# buffers, skipping, draining, the status, reconfiguring with keyword arguments and rejecting inputs.
#
# MultiReader2 reads equal-rate signals aligned onto the main input's tick grid: sample i of every input's array is
# the same tick. Everything the reader has to say comes back on the read, as one status per read.
##

import threading
import time

import numpy as np
import opendaq as daq

ORIGIN = '2026-11-11T00:00:00+00:00'
RATE = 1000  # Hz; ticks of 1 ms, delta 1


def make_context():
    '''
    MultiReader2 needs a scheduler in the inputs' context. In an application, use instance.context.
    '''
    logger = daq.Logger([], daq.LogLevel.Warn)
    return daq.Context(daq.Scheduler(logger, 1), logger, daq.TypeManager(), None, None, {}, {})


def make_signal(ctx, name, unit='V'):
    '''
    Creates a Float64 signal with a linear 1 kHz time domain.
    '''
    domain_bldr = daq.DataDescriptorBuilder()
    domain_bldr.sample_type = daq.SampleType.Int64
    domain_bldr.tick_resolution = daq.Ratio(1, RATE)
    domain_bldr.rule = daq.LinearDataRule(1, 0)
    domain_bldr.unit = daq.Unit(-1, 's', 'second', 'time')
    domain_bldr.origin = ORIGIN

    value_bldr = daq.DataDescriptorBuilder()
    value_bldr.sample_type = daq.SampleType.Float64
    value_bldr.unit = daq.Unit(-1, unit, '', '')

    domain = daq.Signal(ctx, None, name + '_time', None)
    domain.descriptor = domain_bldr.build()
    signal = daq.Signal(ctx, None, name, None)
    signal.descriptor = value_bldr.build()
    signal.domain_signal = domain
    return daq.ISignal.cast_from(signal)


def send(signal, first_tick, values):
    '''
    Sends one packet of values starting at the given tick.
    '''
    values = np.asarray(values, dtype=np.float64)
    domain_packet = daq.DataPacket(signal.domain_signal.descriptor, len(values), first_tick)
    packet = daq.DataPacketWithDomain(domain_packet, signal.descriptor, len(values), 0)
    np.copyto(np.frombuffer(packet.raw_data, np.float64), values)
    daq.ISignalConfig.cast_from(signal).send_packet(packet)


ctx = make_context()
signals = [make_signal(ctx, name) for name in ('ch0', 'ch1', 'ch2')]

# The reader calls on_data_available on a scheduler thread when data becomes readable or a status awaits a read
data_ready = threading.Event()

# Construct: the inputs plus keyword arguments named as the params' properties
reader = daq.MultiReader2(signals,
                          value_read_type=daq.SampleType.Float64,
                          on_data_available=data_ready.set)

# The first read after construction reports the descriptors; drain() reads until nothing is left
for r in reader.drain():
    print('first status:', r.status)                      # descriptors of every input, resynchronized

# Send 100 samples per signal: ch1 = ch0 + 1000, ch2 = ch0 + 2000
for i, signal in enumerate(signals):
    send(signal, 0, np.arange(100) + 1000 * i)


def wait_for(count, timeout=2.0):
    deadline = time.monotonic() + timeout
    while reader.available_count < count and time.monotonic() < deadline:
        data_ready.wait(0.05)
        data_ready.clear()


wait_for(100)
print('available:', reader.available_count)              # 100

# Read: one result object. The count is required; read never waits
r = reader.read(60)
print('read', r.count, 'samples at offset', r.offset)     # 60 samples at offset 0
print('ch0 first values:', r[0][:3])                      # by slot index
print('ch1 first values:', r[signals[1]][:3])             # or by input
print('one array per input:', [v.shape for v in r.values])   # [(60,), (60,), (60,)]
print('inputs in slot order:', [s.local_id for s in r.inputs])
print('stacked:', np.stack(r.values).shape)              # (3, 60): a copy; read_into fills a matrix you own
print('ticks:', r.ticks[:3])                              # start + offset + i * delta
print('times:', r.times[:3])                              # datetime64[ns] from origin and tick resolution
print('status:', r.status)                                # no changes

# Read into your own buffers: one per input, no allocation per read. A 2D array works row by row, so equal-rate
# inputs fill a matrix you own; with inputs of different rates, pass a list of arrays sized per input
matrix = np.empty((3, 32))
r = reader.read_into(matrix, 32)
print('read_into', r.count, 'samples; r[0] is a view of row 0:', np.shares_memory(r[0], matrix[0]))

# Skip: returns (count, offset, status)
skipped, offset, status = reader.skip_samples(8)
print('skipped', skipped, 'samples at offset', offset)    # 8 at offset 92

# Reconfigure: name only what changes, the rest of the reader's params is kept. A configure is a fresh start, so its
# status reports the descriptors again and the queued data is dropped
status = reader.configure(value_read_type=daq.SampleType.Int32)
print('after configure:', status)
print('applied params:', reader.params.value_read_type, reader.params.error_policy)

for i, signal in enumerate(signals):
    send(signal, 100, np.arange(100, 150) + 1000 * i)
wait_for(50)
r = reader.read(reader.available_count)
print('Int32 read:', r[2].dtype, r[2][:3])               # int32 [2100 2101 2102]


# Reject inputs: only voltage channels are read. Under Exclude, a rejected input is set aside and the rest flows
def accepts(signal, descriptor, domain_descriptor):
    return descriptor.unit is not None and descriptor.unit.symbol == 'V'


status = reader.configure(error_policy=daq.MultiReader2ErrorPolicy.Exclude, accepts_descriptor=accepts)
print('accept all volts:', status.health, status.errors)

amps = make_signal(ctx, 'ch3', unit='A')
status = reader.configure(inputs=signals + [amps])
print('with an ampere channel:', status)                 # Warning: ch3 rejected by the owner
for inp, error in status.errors.items():
    print('  ', inp.local_id, error)

for signal in signals + [amps]:
    send(signal, 150, np.arange(150, 170))
wait_for(20)
r = reader.read(reader.available_count)
print('read:', r.count, 'samples;', [None if v is None else v.shape for v in r.values])   # ch3: None


# Drain from the handler: the samples of each read first, then its status
lock = threading.Lock()
received = []


def on_data():
    with lock:
        for r in reader.drain():
            if r.count:
                received.append(np.stack([v for v in r.values if v is not None]))   # ch3 is set aside
            if r.status.has_changes:
                print('handler saw a status:', r.status)


reader.configure(on_data_available=on_data)
for signal in signals + [amps]:
    send(signal, 170, np.arange(170, 200))
deadline = time.monotonic() + 2.0
while sum(a.shape[1] for a in received) < 30 and time.monotonic() < deadline:
    time.sleep(0.01)
print('handler received', sum(a.shape[1] for a in received), 'samples per input')

reader.configure(on_data_available=None)                  # removes the handler
