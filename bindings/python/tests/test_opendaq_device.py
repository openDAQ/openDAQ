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

class TestAddDeviceConfig(opendaq_test.TestCase):

    @staticmethod
    def streaming_type():
        config = opendaq.PropertyObject()
        config.add_property(opendaq.IntProperty(opendaq.String('port'), opendaq.Integer(1234), opendaq.Boolean(True)))
        return opendaq.StreamingType('streamingType', '', '', 'stream', config, {}, None)

    @staticmethod
    def add_device_config():
        config = opendaq.PropertyObject()
        config.add_property(opendaq.StringProperty(opendaq.String('foo'), opendaq.String('bar'), opendaq.Boolean(True)))
        device_type = opendaq.DeviceType('deviceType', '', '', config, 'dev', {})
        return opendaq.AddDeviceConfig(device_type, {'streamingType': TestAddDeviceConfig.streaming_type()}, True)

    def test_create(self):
        config = self.add_device_config()

        self.assertEqual(config.device_type.id, 'deviceType')
        self.assertEqual(config.type_config.get_property_value('foo'), 'bar')
        config.primary_address_type = opendaq.AddressType.IPv6
        self.assertEqual(config.primary_address_type, opendaq.AddressType.IPv6)
        config.scan_on_add = False
        self.assertFalse(config.scan_on_add)

    def test_streaming_options(self):
        options = self.add_device_config().streaming_options

        self.assertEqual(list(options.streaming_configs.keys()), ['streamingType'])
        options.streaming_type = 'streamingType'
        self.assertEqual(options.streaming_config.streaming_type.id, 'streamingType')
        self.assertEqual(options.streaming_config.type_config.get_property_value('port'), 1234)
        options.streaming_mode = opendaq.StreamingMode.MinConnections
        self.assertEqual(options.streaming_mode, opendaq.StreamingMode.MinConnections)

    def test_add_streaming_config(self):
        config = opendaq.AddStreamingConfig(self.streaming_type())

        self.assertEqual(config.streaming_type.id, 'streamingType')
        self.assertEqual(config.type_config.get_property_value('port'), 1234)

if __name__ == '__main__':
    unittest.main()
