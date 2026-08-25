##
# Starts an openDAQ(TM) LT streaming server that publishes the Signals of a reference Device over
# the secure (TLS) channel only. The plaintext "daq.lt://" channel is left disabled, so clients can
# reach the Device through "daq.lts://" alone.
#
# The server needs its own certificate and private key. With mutual TLS enabled (the default) it
# also needs the CA certificate that signed the client certificates. The "generate_certificates.sh"
# script next to this example creates a self-signed set of all of them:
#
#     ./generate_certificates.sh
#     python lt_tls_server.py --cert secrets/server.crt --key secrets/server.key --ca secrets/ca.crt
#
# Connect to the running server with "lt_tls_client.py" from another terminal.
#
# IMPORTANT: The reference Device identity is overridden to a fixed serial number and local ID, so
# only 1 instance of this example should be active at any given time to avoid discovery clashes.
##

import argparse
import os
import sys
import time

import opendaq as daq

# The LT streaming server serves both the plaintext and the secure channel. Which of the two are
# actually opened is decided by the EnableStreamingPort / EnableTlsStreamingPort properties.
SERVER_TYPE_ID = 'OpenDAQLTStreaming'
DEFAULT_TLS_PORT = 7415

DEVICE_NAME = 'LT TLS streaming server'
DEVICE_LOCAL_ID = 'LtTlsServer'
DEVICE_SERIAL_NUMBER = 'lt-tls-01'


def parse_arguments():
    parser = argparse.ArgumentParser(
        description='openDAQ LT streaming server serving the secure (TLS) channel only.')
    parser.add_argument('--cert', required=True, metavar='PATH',
                        help='server certificate in PEM format')
    parser.add_argument('--key', required=True, metavar='PATH',
                        help='server private key in PEM format')
    parser.add_argument('--ca', metavar='PATH',
                        help='CA certificate used to verify the client certificates, '
                             'required unless --no-mutual-tls is given')
    parser.add_argument('--no-mutual-tls', dest='mutual_tls', action='store_false',
                        help='do not request a certificate from the client '
                             '(mutual TLS is enabled by default)')
    return parser.parse_args()


def existing_file(path, description):
    # The TLS secrets are opened by the streaming module itself, deep inside add_server(). Checking
    # them here turns a mangled OpenSSL error into a readable one, and absolute paths keep the
    # module independent of the working directory.
    if not os.path.isfile(path):
        raise FileNotFoundError(f'{description} not found: {path}')
    return os.path.abspath(path)


def set_module_path(builder):
    # "import opendaq" resolves to the Python package when it is installed, and to the bare
    # extension module when this example is run from a build's bin directory. Only the package
    # defines OPENDAQ_MODULES_DIR; next to the extension the modules sit in the current directory.
    try:
        builder.module_path = daq.OPENDAQ_MODULES_DIR
    except AttributeError:
        builder.module_path = '.'


def create_instance():
    root_device_config = daq.PropertyObject()
    root_device_config.add_property(daq.StringProperty(
        daq.String('Name'), daq.String(DEVICE_NAME), daq.Boolean(True)))
    root_device_config.add_property(daq.StringProperty(
        daq.String('LocalId'), daq.String(DEVICE_LOCAL_ID), daq.Boolean(True)))
    root_device_config.add_property(daq.StringProperty(
        daq.String('SerialNumber'), daq.String(DEVICE_SERIAL_NUMBER), daq.Boolean(True)))

    instance_builder = daq.InstanceBuilder()
    set_module_path(instance_builder)
    # mDNS discovery lets lt_tls_client.py find this server without being told its address
    instance_builder.add_discovery_server('mdns')
    instance_builder.set_root_device('daqref://device0', root_device_config)
    return instance_builder.build()


def create_server_config(instance, args):
    if SERVER_TYPE_ID not in instance.available_server_types:
        raise RuntimeError(f'The "{SERVER_TYPE_ID}" server type is not available. '
                           'The LT streaming server module is either missing or too old to '
                           'support the secure channel.')

    server_type = daq.IServerType.cast_from(instance.available_server_types[SERVER_TYPE_ID])
    config = server_type.create_default_config()

    # Serve the secure channel only. The plaintext streaming port stays closed, and so must the
    # control port: the module rejects a configuration that enables the control port without the
    # plaintext streaming port.
    config.set_property_value('EnableTlsStreamingPort', True)
    config.set_property_value('EnableStreamingPort', False)
    config.set_property_value('EnableControlPort', False)
    config.set_property_value('TlsWebsocketStreamingPort', DEFAULT_TLS_PORT)

    config.set_property_value('CertificateFilePath', existing_file(args.cert, 'Server certificate'))
    config.set_property_value('KeyFilePath', existing_file(args.key, 'Server private key'))

    # With mutual TLS the client is authenticated as well, and its certificate is verified against
    # this CA. Without it the client stays anonymous, but the channel is still encrypted.
    config.set_property_value('EnableMutualTls', args.mutual_tls)
    if args.mutual_tls:
        if args.ca is None:
            raise ValueError('Mutual TLS is enabled but no CA certificate was given. '
                             'Pass --ca <file>, or turn mutual TLS off with --no-mutual-tls.')
        config.set_property_value('CaCertificateFilePath', existing_file(args.ca, 'CA certificate'))

    return config


def print_summary(instance, args, config):
    print('LT streaming server started')
    print(f'  device:            {instance.root_device.name}')
    port = config.get_property_value('TlsWebsocketStreamingPort')
    print(f'  TLS channel:       daq.lts:// on port {port}')
    print('  plaintext channel: disabled')
    print(f'  mutual TLS:        {"enabled" if args.mutual_tls else "disabled"}')
    print(f'  certificate:       {config.get_property_value("CertificateFilePath")}')
    print(f'  private key:       {config.get_property_value("KeyFilePath")}')
    if args.mutual_tls:
        print(f'  CA certificate:    {config.get_property_value("CaCertificateFilePath")}')
    print(f'  signals served:    {len(instance.root_device.signals_recursive)}')
    print('\nPress Ctrl+C to stop the server.')


def main():
    args = parse_arguments()

    instance = create_instance()
    server_config = create_server_config(instance, args)
    # enable_discovery() advertises the secure channel as the "_streaming-lts._tcp" mDNS service
    instance.add_server(SERVER_TYPE_ID, server_config).enable_discovery()

    print_summary(instance, args, server_config)

    try:
        while True:
            time.sleep(0.5)
    except KeyboardInterrupt:
        print('\nStopping the server')

    return 0


if __name__ == '__main__':
    sys.exit(main())
