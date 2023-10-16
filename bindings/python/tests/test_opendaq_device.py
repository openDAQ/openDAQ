#!/usr/bin/env python

import opendaq_test
import opendaq
import unittest

class TestDeviceInfo(opendaq_test.TestCase):

    def test_create(self):
        # TODO
        return

        device_info = opendaq.DeviceInfoConfig('name', 'daq.opcua://127.0.0.1')

if __name__ == '__main__':
    unittest.main()
