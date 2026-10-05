##
# Connects to an LT streaming server over the secure (TLS) channel and reads a Signal from it.
#
# The example first looks the server up through mDNS discovery and prints the Server Capabilities it
# advertises. Capabilities carry a protocol group ID and a protocol security level, so a client can
# pick the most secure channel a server offers on its own: the LT streaming channels share the
# "LTStreaming" group, and the secure one ("OpenDAQLTStreamingSecure", "daq.lts") has the higher
# security level. The client then connects to the selected capability with its own TLS configuration
# and reads the Signal for a few seconds.
#
# Start "lt_tls_server.py" in another terminal first, and use the certificates created by
# "generate_certificates.sh":
#
#     python lt_tls_client.py --ca secrets/ca.crt --cert secrets/client.crt --key secrets/client.key
#
# Pass --host to skip discovery and connect to a known address instead. Pass --demo-negative to run
# a set of deliberately broken TLS configurations and see how each of them is rejected.
##

import argparse
import os
import re
import sys
import time

import opendaq as daq

SECURE_PROTOCOL_ID = 'OpenDAQLTStreamingSecure'
LT_PROTOCOL_GROUP_ID = 'LTStreaming'
SECURE_PREFIX = 'daq.lts'
PLAIN_PREFIX = 'daq.lt'
DEFAULT_TLS_PORT = 7415

# The server publishes its Signals asynchronously right after the connection is established, so they
# are not necessarily there the moment add_device() returns.
SIGNAL_WAIT_TIMEOUT = 5.0


def parse_arguments():
    parser = argparse.ArgumentParser(
        description='Connects to an openDAQ LT streaming server over the secure (TLS) channel.')
    parser.add_argument('--ca', metavar='PATH',
                        help='CA certificate the client trusts when verifying the server')
    parser.add_argument('--cert', metavar='PATH',
                        help='client certificate in PEM format, used for mutual TLS')
    parser.add_argument('--key', metavar='PATH',
                        help='client private key in PEM format, used for mutual TLS')
    parser.add_argument('--no-mutual-tls', dest='mutual_tls', action='store_false',
                        help='do not send a client certificate '
                             '(mutual TLS is enabled by default)')
    parser.add_argument('--no-verify-server-cert', dest='verify_server_cert', action='store_false',
                        help='encrypt the connection without authenticating the server; '
                             'the client sends no certificate of its own either')
    parser.add_argument('--host', metavar='HOST[:PORT]',
                        help='connect to this address directly instead of discovering the server')
    parser.add_argument('--duration', type=float, default=10.0, metavar='SECONDS',
                        help='how long to read the signal (default: %(default)s)')
    parser.add_argument('--demo-negative', action='store_true',
                        help='run a matrix of failing TLS configurations instead of reading data')
    parser.add_argument('--other-ca', metavar='PATH',
                        help='an unrelated CA certificate used by --demo-negative '
                             '(default: "other-ca.crt" next to --ca)')
    return parser.parse_args()


def set_module_path(builder):
    # "import opendaq" resolves to the Python package when it is installed, and to the bare
    # extension module when this example is run from a build's bin directory. Only the package
    # defines OPENDAQ_MODULES_DIR; next to the extension the modules sit in the current directory.
    try:
        builder.module_path = daq.OPENDAQ_MODULES_DIR
    except AttributeError:
        builder.module_path = '.'


def create_instance():
    builder = daq.InstanceBuilder()
    set_module_path(builder)
    return builder.build()


def existing_file(path, description):
    if path is None:
        return None
    if not os.path.isfile(path):
        raise FileNotFoundError(f'{description} not found: {path}')
    return os.path.abspath(path)


def create_client_config(instance, ca=None, cert=None, key=None,
                         mutual_tls=True, verify_server_cert=True):
    if SECURE_PROTOCOL_ID not in instance.available_device_types:
        raise RuntimeError(f'The "{SECURE_PROTOCOL_ID}" device type is not available. '
                           'The LT streaming client module is either missing or too old to '
                           'support the secure channel.')

    device_type = daq.IDeviceType.cast_from(instance.available_device_types[SECURE_PROTOCOL_ID])
    config = device_type.create_default_config()

    # With verification turned off the connection is still encrypted, but the server is not
    # authenticated and the client sends no certificate of its own
    config.set_property_value('VerifyServerCertificate', verify_server_cert)
    if not verify_server_cert:
        return config

    config.set_property_value('EnableMutualTls', mutual_tls)
    if ca is not None:
        config.set_property_value('CaCertificateFilePath', existing_file(ca, 'CA certificate'))
    if mutual_tls:
        if cert is not None:
            config.set_property_value('CertificateFilePath',
                                      existing_file(cert, 'Client certificate'))
        if key is not None:
            config.set_property_value('KeyFilePath', existing_file(key, 'Client private key'))

    return config


def error_message(error):
    first_line = str(error).split('\n')[0]
    return re.sub(r'\s*\[ [^\]]*\]\s*$', '', first_line).strip()


def to_int(value, default=-1):
    if value is None:
        return default
    return value.value if hasattr(value, 'value') else int(value)


def print_capability(capability, indent='    '):
    print(f'{indent}{capability.protocol_id}')
    print(f'{indent}  protocol group id:      {capability.protocol_group_id}')
    print(f'{indent}  protocol security level:{to_int(capability.protocol_security_level):>3}')
    print(f'{indent}  prefix:                 {capability.prefix}')
    print(f'{indent}  port:                   {to_int(capability.port)}')
    print(f'{indent}  connection string:      {capability.connection_string}')


def lt_capabilities(device_info):
    capabilities = []
    for capability in device_info.server_capabilities:
        try:
            group_id = capability.protocol_group_id
        except AttributeError:
            raise RuntimeError(
                'These Python bindings predate the protocol group ID / security level API. '
                'Rebuild them from this branch (ninja py_opendaq_daq), or pass --host to connect '
                'without inspecting the capabilities.') from None
        if group_id == LT_PROTOCOL_GROUP_ID:
            capabilities.append(capability)
    return capabilities


def discover_capability(instance):
    """Finds an LT streaming server through mDNS and returns its most secure capability."""
    print('Discovering LT streaming servers...')

    selected = None
    for device_info in instance.available_devices:
        capabilities = lt_capabilities(device_info)
        if not capabilities:
            continue

        print(f'\n  {device_info.name} (serial number: {device_info.serial_number})')
        for capability in capabilities:
            print_capability(capability)

        if selected is None:
            # Ordering the capabilities of a protocol group by their security level is what makes a
            # client prefer the secure channel without knowing anything about the protocols
            selected = max(capabilities,
                           key=lambda capability: to_int(capability.protocol_security_level))

    if selected is None:
        raise RuntimeError('No LT streaming server was discovered. Start "lt_tls_server.py" first, '
                           'or pass --host to connect to a known address.')

    if selected.protocol_id != SECURE_PROTOCOL_ID:
        raise RuntimeError(f'The most secure LT capability offered is "{selected.protocol_id}", '
                           'not the TLS one. Make sure the server was started with the secure '
                           'channel enabled.')

    print(f'\nSelected "{selected.protocol_id}" (security level '
          f'{to_int(selected.protocol_security_level)})')
    return selected.connection_string


def connection_string_from_host(host):
    remainder = host[host.rindex(']') + 1:] if ']' in host else host
    if ':' not in remainder:
        host = f'{host}:{DEFAULT_TLS_PORT}'
    return f'{SECURE_PREFIX}://{host}/'


def wait_for_signals(device, timeout=SIGNAL_WAIT_TIMEOUT):
    deadline = time.monotonic() + timeout
    while True:
        signals = [signal for signal in device.signals_recursive
                   if signal.domain_signal is not None]
        if signals or time.monotonic() >= deadline:
            return signals
        time.sleep(0.1)


def active_streaming_source(signal, timeout=SIGNAL_WAIT_TIMEOUT):
    mirrored_signal = daq.IMirroredSignalConfig.cast_from(signal)
    deadline = time.monotonic() + timeout
    while True:
        source = mirrored_signal.active_streaming_source
        if source is not None or time.monotonic() >= deadline:
            return source
        time.sleep(0.1)


def read_signal(signal, duration):
    print(f'\nReading "{signal.name}" for {duration:g} seconds\n')

    reader = daq.StreamReader(signal)
    total = 0
    end_time = time.monotonic() + duration
    while time.monotonic() < end_time:
        time.sleep(0.25)
        samples = reader.read(1000)
        if len(samples) > 0:
            total += len(samples)
            print(f'  {len(samples):5d} samples, last value: {samples[-1]}')

    return total


def connect_and_read(instance, connection_string, config, args):
    print(f'\nConnecting to {connection_string}')
    device = instance.add_device(connection_string, config)

    connection_info = device.info.configuration_connection_info
    print(f'Connected over "{connection_info.protocol_id}" '
          f'(port {to_int(connection_info.port)})')

    signals = wait_for_signals(device)
    if not signals:
        raise RuntimeError('The device published no signals within '
                           f'{SIGNAL_WAIT_TIMEOUT:g} seconds.')

    print(f'Signals available: {", ".join(signal.name for signal in signals)}')

    signal = signals[0]
    source = active_streaming_source(signal)
    if source is None:
        raise RuntimeError(f'Signal "{signal.name}" has no active streaming source.')

    print(f'Active streaming source: {source}')
    if not source.startswith(f'{SECURE_PREFIX}://'):
        raise RuntimeError(f'The data is not streamed over the secure channel: {source}')

    total = read_signal(signal, args.duration)

    print(f'\nRead {total} samples over the secure channel.')
    instance.remove_device(device)


def negative_cases(args, connection_string):
    """The attempts made by --demo-negative, as (title, connection string, config kwargs, expected).

    A config of None means connecting without any TLS configuration at all.
    """
    other_ca = args.other_ca
    if other_ca is None and args.ca is not None:
        other_ca = os.path.join(os.path.dirname(os.path.abspath(args.ca)), 'other-ca.crt')

    valid = {'ca': args.ca, 'cert': args.cert, 'key': args.key}
    plain_connection_string = connection_string.replace(
        f'{SECURE_PREFIX}://', f'{PLAIN_PREFIX}://', 1)

    cases = [
        ('Valid CA, certificate and key',
         connection_string, dict(valid), True),
        ('Untrusted CA: the server certificate is signed by another authority',
         connection_string, dict(valid, ca=other_ca), False),
        ('Mutual TLS disabled on the client: no certificate is sent to the server',
         connection_string, dict(valid, mutual_tls=False), False),
        ('Server verification disabled: encrypted, but neither peer is authenticated',
         connection_string, dict(valid, verify_server_cert=False), False),
        ('No CA certificate configured: rejected before any network traffic',
         connection_string, dict(valid, ca=None), False),
        (f'Plaintext {PLAIN_PREFIX}:// connection to the TLS port',
         plain_connection_string, None, False),
    ]

    if other_ca is None or not os.path.isfile(other_ca):
        # Without an unrelated CA there is nothing to present as untrusted
        cases.pop(1)

    return cases


def run_negative_demo(args, connection_string):
    print('\nTrying a set of client TLS configurations against the server.')
    print('The expectations below assume a server with mutual TLS enabled (the default).\n')

    results = []
    for title, target, config_kwargs, expect_success in negative_cases(args, connection_string):
        print(f'* {title}')
        try:
            # A fresh instance per attempt keeps a failed connection from affecting the next one
            instance = create_instance()
            config = None
            if config_kwargs is not None:
                config = create_client_config(instance, **config_kwargs)
            device = instance.add_device(target, config)
            instance.remove_device(device)
            succeeded, detail = True, 'the connection was established'
        except Exception as error:
            succeeded, detail = False, error_message(error)

        as_expected = succeeded == expect_success
        print(f'    {"connected" if succeeded else "rejected"}: {detail}')
        print(f'    {"as expected" if as_expected else "UNEXPECTED"}\n')
        results.append((title, as_expected))

    print('Summary:')
    for title, as_expected in results:
        print(f'  [{"OK  " if as_expected else "FAIL"}] {title}')

    return 0 if all(as_expected for _, as_expected in results) else 1


def main():
    args = parse_arguments()

    try:
        instance = create_instance()

        if args.host is not None:
            connection_string = connection_string_from_host(args.host)
        else:
            connection_string = discover_capability(instance)

        if args.demo_negative:
            return run_negative_demo(args, connection_string)

        config = create_client_config(instance,
                                      ca=args.ca,
                                      cert=args.cert,
                                      key=args.key,
                                      mutual_tls=args.mutual_tls,
                                      verify_server_cert=args.verify_server_cert)
        connect_and_read(instance, connection_string, config, args)
    except KeyboardInterrupt:
        print('\nInterrupted')

    return 0


if __name__ == '__main__':
    sys.exit(main())
