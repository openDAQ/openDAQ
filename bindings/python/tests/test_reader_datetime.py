#!/usr/bin/env python

import unittest
import opendaq_test
import opendaq

from datetime import datetime


class TestReaderDateTime(opendaq_test.TestCase):

    def test_start_simulator(self):
        instance = opendaq.Instance()
        instance.set_root_device('daqref://device0')
        instance.add_server('openDAQ WebsocketTcp Streaming', None)
        instance.add_standard_servers()

    def test_read_with_domain(self):
        import time

        instance = opendaq.Instance()
        device = instance.add_device('daqref://device0')
        signal = device.signals_recursive[0]
        reader = opendaq.StreamReader(signal)

        time.sleep(0.1)
        (values, domain) = reader.read_with_domain(100)
        for t in domain:
            self.assertNotIsInstance(t, datetime)

    def test_read_with_timestamps(self):
        import time

        instance = opendaq.Instance()
        device = instance.add_device('daqref://device0')
        signal = device.signals_recursive[0]
        reader = opendaq.StreamReader(signal)

        time.sleep(0.1)
        (values, domain) = reader.read_with_timestamps(100)
        for t in domain:
            self.assertIsInstance(t, datetime)


if __name__ == '__main__':
    unittest.main()
