##
# MultiReader2 trial from Python: connects to the openDAQ Raspberry Pis on the network, reads each Pi's ADC with its own
# reader and both ADCs with one reader, and prints what every status reports and how much each reader delivered.
#
# Usage: python mr2_rpi_read.py [address ...] [--seconds N]
#
# Without addresses the Pis are found over mDNS. Each is connected with the first protocol that works: native
# configuration, OPC UA, native streaming. The trial only reads; nothing on the Pis is changed.
##

import argparse
import time

import opendaq as daq

PROTOCOLS = [('OpenDAQNativeConfiguration', 'daq.nd://'), ('OpenDAQOPCUAConfiguration', 'daq.opcua://'),
             ('OpenDAQNativeStreaming', 'daq.ns://')]


def connection_strings(instance, addresses):
    '''The connection strings to try for each Pi, best protocol first, keyed by a name for the log.'''
    if addresses:
        return {address: [prefix + address for _, prefix in PROTOCOLS] for address in addresses}
    pis = {}
    for info in instance.available_devices:
        if 'Raspberry Pi' not in info.model:
            continue
        by_protocol = {cap.protocol_id: cap.connection_string for cap in info.server_capabilities}
        pis[info.serial_number] = [by_protocol[protocol] for protocol, _ in PROTOCOLS if protocol in by_protocol]
    return pis


def connect(instance, name, strings):
    for string in strings:
        try:
            device = instance.add_device(string)
            print(f'{name}: connected with {string}')
            return device
        except Exception as e:
            print(f'{name}: {string} failed: {e}')
    return None


def adc_signal(device):
    '''The ADS1256 AI output; a streaming pseudo device flattens the remote path into the local ID with "*".'''
    for signal in device.signals_recursive:
        if signal.global_id.replace('*', '/').endswith('/IO/AI/Sig/AI'):
            return signal
    return None


def describe(status):
    inputs = ', '.join(f'{i.input.local_id}: {i.error.name}' + (' (new descriptor)' if i.descriptor_changed else '')
                       for i in status.inputs)
    flags = [name for name in ('resynchronized', 'domain_descriptor_changed', 'synchronizing') if getattr(status, name)]
    message = status.status_message or 'no message'
    return f'{status.status.name}, {message}; {", ".join(flags) or "no flags"}; inputs: {inputs}'


def run(name, signals, seconds, policy):
    '''Reads the signals with one MultiReader2 for the given time; returns the samples read per input.'''
    params = daq.MultiReader2Params()
    params.inputs = signals
    params.error_policy = policy
    reader = daq.MultiReader2(params)
    print(f'\n[{name}] {len(signals)} input(s), policy {policy.name}')

    total = 0
    first_tick = last_tick = None
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        count = reader.available_count
        ticks, values, status = reader.read_with_domain(count)
        if status.has_changes:
            print(f'[{name}] status: {describe(status)}')
        if len(ticks):
            first_tick = ticks[0] if first_tick is None else first_tick
            last_tick = ticks[-1]
            total += len(ticks)
        time.sleep(0.1)

    print(f'[{name}] read {total} aligned samples per input in {seconds} s', end='')
    if first_tick is not None:
        print(f', main ticks {first_tick} to {last_tick}')
    else:
        print()
    return total


def main():
    parser = argparse.ArgumentParser(description='Read the openDAQ Raspberry Pis with MultiReader2')
    parser.add_argument('addresses', nargs='*', help='Pi addresses; found over mDNS when none are given')
    parser.add_argument('--seconds', type=float, default=10, help='how long each reader reads')
    args = parser.parse_args()

    instance = daq.Instance()
    adcs = []
    for name, strings in connection_strings(instance, args.addresses).items():
        device = connect(instance, name, strings)
        signal = adc_signal(device) if device else None
        if signal is None:
            print(f'{name}: no ADC signal')
            continue
        descriptor = signal.domain_signal.descriptor
        info = descriptor.reference_domain_info
        clock = info.reference_domain_id if info else None
        print(f'{name}: ADC {signal.global_id}, resolution {descriptor.tick_resolution}, origin {descriptor.origin}, clock {clock}')
        adcs.append((name, signal))

    if not adcs:
        print('No Pi with an ADC answered')
        return 1
    for name, signal in adcs:
        run(name, [signal], args.seconds, daq.MultiReader2ErrorPolicy.Invalidate)
    if len(adcs) > 1:
        # Two Pis do not share a clock unless both follow the same PTP grandmaster; the status tells what the reader makes of it
        run('both', [signal for _, signal in adcs], args.seconds, daq.MultiReader2ErrorPolicy.Exclude)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
