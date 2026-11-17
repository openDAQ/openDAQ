#!/usr/bin/env python

import opendaq_test
import opendaq
import unittest

class TestDeviceInfo(opendaq_test.TestCase):

    def test_create(self):
        # TODO
        return

        device_info = opendaq.DeviceInfoConfig('name', 'daq.opcua://127.0.0.1')

class TestDeviceType(opendaq_test.TestCase):

    def test_create(self):
        default_config = opendaq.PropertyObject()
        default_config.add_property(opendaq.StringProperty(opendaq.String(
            'property'), opendaq.String('value'), opendaq.Boolean(True)))
        methods = {'none': opendaq.NoneAuthenticationMethod('none', 'No authentication')}
        device_type = opendaq.DeviceType('id', 'name', 'description', default_config, 'prefix', methods)

        self.assertEqual(device_type.id, 'id')
        self.assertEqual(device_type.name, 'name')
        self.assertEqual(device_type.description, 'description')
        self.assertEqual(device_type.connection_string_prefix, 'prefix')
        self.assertEqual(device_type.create_default_config().get_property_value('property'), 'value')
        self.assertEqual(list(device_type.create_authentication_config().authentication_methods.keys()), ['none'])

    def test_module_types(self):
        instance = opendaq.Instance()
        device_types = instance.available_device_types
        ref_type = opendaq.IDeviceType.cast_from(device_types['daqref'])
        root_type = opendaq.IDeviceType.cast_from(device_types['daqref_root'])

        self.assertEqual(ref_type.device_kind, opendaq.DeviceKind.Adapter)
        self.assertEqual(root_type.device_kind, opendaq.DeviceKind.Root)
        self.assertIsNotNone(ref_type.module_info)

class TestTransportOptions(opendaq_test.TestCase):

    def test_transport_option(self):
        option = opendaq.TransportOption('tls', 7422, opendaq.TransportProtection.Encrypted, True)

        self.assertEqual(option.name, 'tls')
        self.assertEqual(option.port, 7422)
        self.assertEqual(option.protection, opendaq.TransportProtection.Encrypted)
        self.assertTrue(option.certificate_required)

        plain = opendaq.TransportOption('plain', 7420)
        self.assertEqual(plain.protection, getattr(opendaq.TransportProtection, 'None'))
        self.assertFalse(plain.certificate_required)

    def test_native_types_list_options(self):
        instance = opendaq.Instance()
        device_type = opendaq.IDeviceType.cast_from(instance.available_device_types['OpenDAQNativeConfiguration'])
        add_config = instance.create_add_device_config(opendaq.DeviceInfoConfig('dev', 'daq.nd://127.0.0.1'))
        streaming_type = add_config.streaming_options.available_streaming_configs['OpenDAQNativeStreaming'].streaming_type

        for component_type in (device_type, streaming_type):
            options = list(component_type.transport_options)
            self.assertEqual(options[0].name, 'plain')
            self.assertEqual(options[0].port, 7420)
            # a plain type config selects the option by its TransportOption property
            config = component_type.create_default_config()
            self.assertEqual(config.get_property_value('TransportOption'), 'plain')

        # other types list none
        self.assertEqual(len(list(opendaq.IDeviceType.cast_from(instance.available_device_types['daqref']).transport_options)), 0)

    def test_add_device_config_selection(self):
        instance = opendaq.Instance()
        config = instance.create_add_device_config(opendaq.DeviceInfoConfig('dev', 'daq.nd://127.0.0.1'))

        names = [option.name for option in config.transport_options]
        self.assertEqual(names[0], 'plain')
        self.assertEqual(config.selected_transport_option.name, 'plain')
        self.assertEqual(config.port, 7420)

        config.port = 9000
        self.assertEqual(config.port, 9000)
        # selecting an option drops the port set before
        config.set_transport_option(names[-1])
        self.assertEqual(config.selected_transport_option.name, names[-1])
        self.assertEqual(config.port, config.selected_transport_option.port)

        with self.assertRaises(RuntimeError):
            config.set_transport_option('wss')

        streaming_config = config.streaming_options.available_streaming_configs['OpenDAQNativeStreaming']
        self.assertEqual(streaming_config.selected_transport_option.name, 'plain')
        self.assertEqual(streaming_config.port, 7420)

    def test_add_device_config_without_options(self):
        instance = opendaq.Instance()
        config = instance.create_add_device_config(opendaq.DeviceInfoConfig('dev', 'daqref://device0'))

        self.assertEqual(len(list(config.transport_options)), 0)
        with self.assertRaises(RuntimeError):
            config.set_transport_option('plain')


class TestAddDeviceConfig(opendaq_test.TestCase):

    def test_properties(self):
        instance = opendaq.Instance()
        config = instance.create_add_device_config(opendaq.DeviceInfoConfig('dev', 'daqref://device0'))

        self.assertEqual(config.device_type.id, 'daqref')
        self.assertIsNotNone(config.type_config)
        self.assertIsNone(config.streaming_options)
        config.primary_address_type = opendaq.AddressType.IPv6
        self.assertEqual(config.primary_address_type, opendaq.AddressType.IPv6)
        config.scan_on_add = False
        self.assertFalse(config.scan_on_add)

    def test_streaming_options(self):
        instance = opendaq.Instance()
        options = instance.create_add_device_config(opendaq.DeviceInfoConfig('dev', 'daq.nd://127.0.0.1')).streaming_options

        self.assertIn('OpenDAQNativeStreaming', options.available_streaming_configs.keys())
        options.streaming_type = None
        self.assertIsNone(options.streaming_type)
        self.assertIsNone(options.streaming_config)
        options.streaming_type = 'OpenDAQNativeStreaming'
        self.assertEqual(options.streaming_type, 'OpenDAQNativeStreaming')
        self.assertEqual(options.streaming_config.streaming_type.id, 'OpenDAQNativeStreaming')
        self.assertIsNotNone(options.streaming_config.type_config)
        options.streaming_mode = opendaq.StreamingMode.MinConnections
        self.assertEqual(options.streaming_mode, opendaq.StreamingMode.MinConnections)

class TestDeviceAddConfig(opendaq_test.TestCase):

    def test_create_add_device_config(self):
        instance = opendaq.Instance()
        info = opendaq.DeviceInfoConfig('ref', 'daqref://device0')

        config = instance.create_add_device_config(info, 'daqref')
        self.assertEqual(config.device_type.id, 'daqref')

    def test_create_add_streaming_config_of_root_device(self):
        instance = opendaq.Instance()
        with self.assertRaises(RuntimeError):
            instance.create_add_streaming_config()

    def test_root_device(self):
        instance = opendaq.Instance()
        device = instance.root_device

        self.assertIsNone(device.add_device_config)
        self.assertEqual(len(device.available_streaming_types), 0)

if __name__ == '__main__':
    unittest.main()
