#!/usr/bin/env python

import opendaq_test
import opendaq
import unittest

class TestInstance(opendaq_test.TestCase):

    def test_create(self):
        instance = opendaq.Instance()
        devices = instance.available_devices

    def test_enumerate_available_devices(self):
        connection_string_to_find = 'daqref://device0'
        instance = opendaq.Instance()
        devices = instance.available_devices
        for device_info in devices:
            if device_info.connection_string == connection_string_to_find:
                stored_device_info = device_info
                added_device = instance.add_device(device_info.connection_string)

        self.assertIsNotNone(stored_device_info)
        self.assertIsNotNone(added_device)
        self.assertEqual(stored_device_info.connection_string, connection_string_to_find)
        #self.assertEqual(added_device.device_info.connection_string, connection_string_to_find)

    def test_server_kind_and_function_blocks(self):
        instance = opendaq.Instance()
        server_type = opendaq.IServerType.cast_from(instance.available_server_types['OpenDAQNativeStreaming'])
        config = server_type.create_default_config()
        config.set_property_value('NativeStreamingPort', 7449)
        server = instance.add_server('OpenDAQNativeStreaming', config)
        self.assertEqual(server_type.server_kind, opendaq.ServerKind.Mirror)
        self.assertEqual(server.server_type.server_kind, opendaq.ServerKind.Mirror)
        self.assertEqual(len(server.available_function_block_types), 0)
        self.assertEqual(len(server.function_blocks), 0)

if __name__ == '__main__':
    unittest.main()
