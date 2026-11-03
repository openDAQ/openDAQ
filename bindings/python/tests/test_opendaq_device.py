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

    def test_add_device_config(self):
        config = opendaq.AddDeviceConfig(['plain', 'tls'])

        self.assertEqual(list(config.available_transports), ['plain', 'tls'])
        self.assertEqual(config.transport, 'plain')

        config.transport = 'tls'
        self.assertEqual(config.transport, 'tls')
        # the typed accessors and the "Transport" selection property are the same value
        self.assertEqual(config.get_property_value('Transport'), 1)

        with self.assertRaises(RuntimeError):
            config.transport = 'wss'
        self.assertEqual(config.transport, 'tls')

    def test_add_streaming_config(self):
        config = opendaq.AddStreamingConfig(['plain', 'tls'])

        self.assertEqual(list(config.available_transports), ['plain', 'tls'])
        self.assertEqual(config.transport, 'plain')

        config.transport = 'tls'
        self.assertEqual(config.transport, 'tls')

    # the way a device or streaming type config carries the object
    def test_cast_from_type_config(self):
        type_config = opendaq.PropertyObject()
        type_config.add_property(opendaq.ObjectProperty('AddDeviceConfig', opendaq.AddDeviceConfig(['plain', 'tls'])))

        add_config = opendaq.IAddDeviceConfig.cast_from(type_config.get_property_value('AddDeviceConfig'))
        add_config.transport = 'tls'

        self.assertEqual(opendaq.IAddComponentConfig.cast_from(type_config.get_property_value('AddDeviceConfig')).transport, 'tls')

if __name__ == '__main__':
    unittest.main()
