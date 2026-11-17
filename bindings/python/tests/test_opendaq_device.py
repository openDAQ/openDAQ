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

class TestAddComponentConfig(opendaq_test.TestCase):

    def test_transport_selection(self):
        config = opendaq.AddComponentConfig(['plain', 'tls'])

        self.assertEqual(list(config.available_transports), ['plain', 'tls'])
        self.assertEqual(config.transport, 'plain')

        config.transport = 'tls'
        self.assertEqual(config.transport, 'tls')
        # the typed accessors and the "Transport" selection property are the same value
        self.assertEqual(config.get_property_value('Transport'), 1)

        with self.assertRaises(RuntimeError):
            config.transport = 'wss'
        self.assertEqual(config.transport, 'tls')

    # the way a device or streaming type config carries the object
    def test_cast_from_type_config(self):
        type_config = opendaq.PropertyObject()
        type_config.add_property(opendaq.ObjectProperty('AddDeviceConfig', opendaq.AddComponentConfig(['plain', 'tls'])))

        add_config = opendaq.IAddComponentConfig.cast_from(type_config.get_property_value('AddDeviceConfig'))
        add_config.transport = 'tls'

        self.assertEqual(opendaq.IAddComponentConfig.cast_from(type_config.get_property_value('AddDeviceConfig')).transport, 'tls')


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
