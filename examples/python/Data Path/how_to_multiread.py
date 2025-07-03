import opendaq as daq
import numpy as np

ctx = daq.NullContext()


def packets_for_signal(signal, packet_size, offset):
    '''
    Creates a data packet and time packet with a sequence of numbers from offset to offset + packet_size.
    '''
    signal = daq.ISignal.cast_from(signal)  # ISignalConfig has no getters
    time_packet = daq.DataPacket(
        signal.domain_signal.descriptor, packet_size, offset)
    data_packet = daq.DataPacketWithDomain(
        time_packet, signal.descriptor, packet_size, 0)
    raw = np.frombuffer(data_packet.raw_data, np.float64)
    np.copyto(raw, np.arange(offset, offset + packet_size, dtype=np.float64))
    return (data_packet, time_packet)


def send_packet_to_signal(signal: daq.ISignalConfig, packet_size, offset):
    '''
    Creates and sends a data packet and time packet with a sequence of numbers from offset to offset + packet_size to the given signal.
    '''
    signal = daq.ISignal.cast_from(signal)  # ISignalConfig has no getters
    data, time = packets_for_signal(signal, packet_size, offset)
    domain = daq.ISignalConfig.cast_from(signal.domain_signal)
    domain.send_packet(time)
    signal = daq.ISignalConfig.cast_from(signal)
    signal.send_packet(data)


def demo_signal(id, epoch):
    '''
    Creates a signal with nested domain signal and the given id and origin.
    '''
    signal = daq.Signal(ctx, None, id + '_values', None)
    domain = daq.Signal(ctx, None, id + '_domain', None)

    vals_desc_bldr = daq.DataDescriptorBuilder()
    vals_desc_bldr.sample_type = daq.SampleType.Float64

    domain_desc_bldr = daq.DataDescriptorBuilder()
    domain_desc_bldr.sample_type = daq.SampleType.Int64
    domain_desc_bldr.tick_resolution = daq.Ratio(1, 1000)
    domain_desc_bldr.rule = daq.LinearDataRule(1, 0)
    domain_desc_bldr.unit = daq.Unit(-1, "s", "second", "time")
    domain_desc_bldr.origin = epoch

    domain.descriptor = domain_desc_bldr.build()
    signal.descriptor = vals_desc_bldr.build()
    signal.domain_signal = domain
    return signal


sig0 = demo_signal('sig0', '2022-09-27T00:02:03+00:00')
sig1 = demo_signal('sig1', '2022-09-27T00:02:04+00:00')
sig2 = demo_signal('sig2', '2022-09-27T00:02:04.123+00:00')

signals = [sig0, sig1, sig2]
reader = daq.MultiReader(signals)
timed_reader = daq.TimeMultiReader(reader)

send_packet_to_signal(sig0, 523, 0)
send_packet_to_signal(sig1, 732, 0)
send_packet_to_signal(sig2, 843, 0)

r = reader.read_with_domain(0)  # status changed
avail = reader.available_count  # 0

send_packet_to_signal(sig0, 523, 523)
send_packet_to_signal(sig1, 732, 732)
send_packet_to_signal(sig2, 843, 843)

send_packet_to_signal(sig0, 523, 1046)
send_packet_to_signal(sig1, 732, 1446)
send_packet_to_signal(sig2, 843, 1686)

print(f'available samples: {reader.available_count}')  # 446

rr = timed_reader.read_with_timestamps(523)
print(f'number of read samples for the first signal: {len(rr[0][0])}')  # 446
print(f'the first three samples of each signal: {rr[0][:, :3]}')
print(f'the first three timestamps of samples of each signal: {rr[1][:, :3]}')
