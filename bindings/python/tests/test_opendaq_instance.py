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

if __name__ == '__main__':
    unittest.main()
