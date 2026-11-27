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
        methods = {'Anonymous': opendaq.AuthenticationMethod('Anonymous', {}, 'No authentication')}
        device_type = opendaq.DeviceType('id', 'name', 'description', default_config, 'prefix', methods)

        self.assertEqual(device_type.id, 'id')
        self.assertEqual(device_type.name, 'name')
        self.assertEqual(device_type.description, 'description')
        self.assertEqual(device_type.connection_string_prefix, 'prefix')
        self.assertEqual(device_type.create_default_config().get_property_value('property'), 'value')
        self.assertEqual(list(device_type.create_authentication_config().authentication_methods.keys()), ['Anonymous'])

    def test_module_types(self):
        instance = opendaq.Instance()
        device_types = instance.available_device_types
        ref_type = opendaq.IDeviceType.cast_from(device_types['daqref'])
        root_type = opendaq.IDeviceType.cast_from(device_types['daqref_root'])

        self.assertEqual(ref_type.device_kind, opendaq.DeviceKind.Adapter)
        self.assertEqual(root_type.device_kind, opendaq.DeviceKind.Root)
        self.assertIsNotNone(ref_type.module_info)

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

    def test_port_without_transport_options(self):
        instance = opendaq.Instance()
        config = instance.create_add_device_config(opendaq.DeviceInfoConfig('dev', 'daq.nd://127.0.0.1'))

        self.assertEqual(len(config.device_type.available_transport_options), 0)
        self.assertEqual(len(config.available_transport_options), 0)
        self.assertIsNone(config.selected_transport_option)
        self.assertEqual(config.port, -1)
        config.port = 7500
        self.assertEqual(config.port, 7500)

        streaming_config = config.streaming_options.available_streaming_configs['OpenDAQNativeStreaming']
        self.assertEqual(len(streaming_config.available_transport_options), 0)
        self.assertIsNone(streaming_config.selected_transport_option)
        self.assertEqual(streaming_config.port, -1)
        streaming_config.port = 7600
        self.assertEqual(streaming_config.port, 7600)

class TestTransportOption(opendaq_test.TestCase):

    def test_builder(self):
        builder = opendaq.TransportOptionBuilder('tls')
        builder.port = 7422
        builder.security_mode = opendaq.SecurityMode.Encrypted
        builder.certificate_required = True

        option = builder.build()
        self.assertEqual(option.id, 'tls')
        self.assertEqual(option.port, 7422)
        self.assertEqual(option.security_mode, opendaq.SecurityMode.Encrypted)
        self.assertTrue(option.certificate_required)

    def test_builder_from_existing(self):
        builder = opendaq.TransportOptionBuilder('tls')
        builder.port = 7422
        builder.security_mode = opendaq.SecurityMode.Encrypted
        option = builder.build()

        copy = opendaq.TransportOptionBuilderFromExisting(option)
        copy.port = 7500
        copied = copy.build()
        self.assertEqual(copied.id, 'tls')
        self.assertEqual(copied.port, 7500)
        self.assertEqual(copied.security_mode, opendaq.SecurityMode.Encrypted)
        self.assertEqual(option.port, 7422)

    def test_builder_defaults(self):
        builder = opendaq.TransportOptionBuilder('serial')
        self.assertEqual(builder.id, 'serial')

        option = builder.build()
        self.assertEqual(option.port, 0)
        # SecurityMode.None cannot be written in Python: None is a keyword
        self.assertEqual(option.security_mode, getattr(opendaq.SecurityMode, 'None'))
        self.assertFalse(option.certificate_required)

    def test_builder_with_empty_id_rejected(self):
        with self.assertRaises(RuntimeError):
            opendaq.TransportOptionBuilder('').build()

    def test_is_property_object(self):
        builder = opendaq.TransportOptionBuilder('tls')
        builder.port = 7422
        option = builder.build()

        self.assertEqual(option.get_property_value('Id'), 'tls')
        self.assertEqual(option.get_property_value('Port'), 7422)
        with self.assertRaises(RuntimeError):
            option.set_property_value('Port', 9000)
        self.assertEqual(option.port, 7422)

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
