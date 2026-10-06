##
# MultiReader2 with synthetic signals: alignment on origins and packet offsets, descriptor changes, a gap, and an
# input going invalid and recovering. Everything is deterministic, so this doubles as a walk through the status.
#
# The value at tick t is t, so alignment is visible in the values alone. A NullContext has no scheduler, so the
# wake runs inline and no sleeps are needed.
##

import numpy as np
import opendaq as daq

ctx = daq.NullContext()


def demo_signal(name, origin, sample_type=daq.SampleType.Float64):
    signal = daq.Signal(ctx, None, name, None)
    domain = daq.Signal(ctx, None, name + '_domain', None)
    values = daq.DataDescriptorBuilder()
    values.sample_type = sample_type
    time = daq.DataDescriptorBuilder()
    time.sample_type = daq.SampleType.Int64
    time.tick_resolution = daq.Ratio(1, 1000)                # 1 kHz
    time.rule = daq.LinearDataRule(1, 0)
    time.unit = daq.Unit(-1, 's', 'second', 'time')
    time.origin = origin                                      # ISO 8601 on a full second
    domain.descriptor = time.build()
    signal.descriptor = values.build()
    signal.domain_signal = domain
    return signal


def send(signal, offset, size, dtype=np.float64):
    sig = daq.ISignal.cast_from(signal)
    time_packet = daq.DataPacket(sig.domain_signal.descriptor, size, offset)
    data_packet = daq.DataPacketWithDomain(time_packet, sig.descriptor, size, 0)
    np.copyto(np.frombuffer(data_packet.raw_data, dtype), np.arange(offset, offset + size, dtype=dtype))
    daq.ISignalConfig.cast_from(signal).send_packet(data_packet)


def show(status):
    print(f'  status: valid {status.valid}, changes {status.has_changes}, domain changed {status.domain_descriptor_changed}, '
          f'resynchronized {status.resynchronized}')
    for s in status.inputs:
        print(f'    {s.input.name}: {s.error}, descriptor changed {s.descriptor_changed}')


sig0 = demo_signal('sig0', '2022-09-27T00:02:03+00:00')
sig1 = demo_signal('sig1', '2022-09-27T00:02:04+00:00')   # one second later: its tick 0 is tick 1000 of sig0
sig2 = demo_signal('sig2', '2022-09-27T00:02:03+00:00')

params = daq.MultiReader2Params()
params.inputs = [sig0, sig1, sig2]
reader = daq.MultiReader2(params)

print('first status: everything is new')
show(reader.read(0)[2])

send(sig0, 0, 1500)
send(sig1, 0, 500)
send(sig2, 200, 1300)                                      # starts 200 ticks in
print(f'available: {reader.available_count} (common start is sig1 at main tick 1000, sig0 ends at 1499)')
ticks, values, status = reader.read_with_domain(5)
print(f'  ticks {ticks.tolist()}')
print(f'  sig0 {values[0].tolist()}  sig1 {values[1].tolist()}  sig2 {values[2].tolist()}')

print('a value descriptor change on sig1 with data behind it: the data in front comes first, then the change')
new_descriptor = daq.DataDescriptorBuilderFromExisting(daq.ISignal.cast_from(sig1).descriptor)
new_descriptor.unit = daq.Unit(-1, 'V', 'volt', 'voltage')
sig1.descriptor = new_descriptor.build()
send(sig1, 500, 500)
values, offset, status = reader.read(1000)
print(f'  read {len(values[0])} samples at offset {offset}')
show(status)
print(f'  sig1 unit is now {status.inputs[1].descriptor.unit.symbol}')

print('a gap on sig2: the read reaching it reports a resynchronization and the offset jumps')
send(sig0, 1500, 500)
send(sig2, 1600, 400)                                      # ticks 1500..1599 are missing
values, offset, status = reader.read(1000)
print(f'  read {len(values[0])} samples: sig2 has nothing at the read position, so the gap is reached at once')
show(status)
values, offset, status = reader.read(1000)
print(f'  after the resync: {len(values[0])} samples at offset {offset}')

print('sig2 becomes unreadable: the reader is invalid until it recovers, and everything is new again')
sig2.descriptor = daq.DataDescriptorBuilder().build()     # no sample type: not convertible
show(reader.read(0)[2])
sig2.descriptor = daq.ISignal.cast_from(sig0).descriptor
show(reader.read(0)[2])
