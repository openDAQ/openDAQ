#!/usr/bin/env python

import time
import unittest

import numpy as np

import opendaq
import opendaq_test


class TestMultiReader2(opendaq_test.TestCase):

    def setUp(self):
        super().setUp()
        self.logger = opendaq.Logger([], opendaq.LogLevel.Off)
        self.scheduler = opendaq.Scheduler(self.logger, 1)
        self.context = opendaq.Context(self.scheduler, self.logger, opendaq.TypeManager(), None, None, {}, {})

        builder = opendaq.DataDescriptorBuilder()
        builder.sample_type = opendaq.SampleType.Int64
        builder.unit = opendaq.Unit(-1, 's', 'second', 'time')
        builder.tick_resolution = opendaq.Ratio(1, 1000)
        builder.rule = opendaq.LinearDataRule(1, 0)
        builder.origin = '1970-01-01T00:00:00Z'
        self.time_desc = builder.build()

        builder = opendaq.DataDescriptorBuilder()
        builder.sample_type = opendaq.SampleType.Float64
        builder.unit = opendaq.Unit(-1, 'V', 'volt', 'voltage')
        self.value_desc = builder.build()

        self.signals = []
        self.time_signals = []
        for i in range(2):
            time_signal = opendaq.SignalWithDescriptor(self.context, self.time_desc, None, f'time{i}', None)
            signal = opendaq.SignalWithDescriptor(self.context, self.value_desc, None, f'value{i}', None)
            signal.domain_signal = time_signal
            self.time_signals.append(time_signal)
            self.signals.append(signal)

    def tearDown(self):
        self.scheduler.stop()
        self.signals = self.time_signals = None
        self.value_desc = self.time_desc = None
        self.context = self.scheduler = self.logger = None
        super().tearDown()

    def params(self, **values):
        params = opendaq.MultiReader2Params()
        params.inputs = self.signals
        for name, value in values.items():
            setattr(params, name, value)
        return params

    def send(self, index, first, count):
        time_packet = opendaq.DataPacket(self.time_desc, count, first)
        value_packet = opendaq.DataPacketWithDomain(time_packet, self.value_desc, count, 0)
        np.copyto(np.frombuffer(value_packet.raw_data, dtype=np.float64), np.arange(first, first + count, dtype=np.float64))
        self.signals[index].send_packet(value_packet)

    def send_all(self, first, count):
        for i in range(len(self.signals)):
            self.send(i, first, count)

    # The reader takes packets on a scheduler worker
    def wait_for(self, reader, count):
        deadline = time.monotonic() + 2
        while reader.available_count < count and time.monotonic() < deadline:
            time.sleep(0.001)
        self.assertEqual(reader.available_count, count)

    def test_first_read_takes_the_status(self):
        reader = opendaq.MultiReader2(self.params())
        values, offset, status = reader.read(10)
        self.assertEqual(values, [])
        self.assertTrue(status.has_changes)
        self.assertEqual(status.status, opendaq.ComponentStatus.Ok)
        self.assertEqual(len(status.inputs), 2)
        for i in range(2):
            input_status = status.get_input_status(self.signals[i])
            self.assertEqual(input_status.error, getattr(opendaq.MultiReader2InputError, 'None'))
            self.assertEqual(input_status.sample_size, 8)

    def test_read_returns_an_array_per_input_the_offset_and_the_status(self):
        reader = opendaq.MultiReader2(self.params())
        reader.read(0)
        self.send_all(5, 10)
        self.wait_for(reader, 10)

        values, offset, status = reader.read(10)
        self.assertFalse(status.has_changes)
        self.assertEqual(offset, 5)
        self.assertEqual(len(values), 2)
        for array in values:
            self.assertTrue(np.array_equal(array, np.arange(5, 15)))
            self.assertEqual(array.dtype, np.float64)

    def test_read_with_domain_returns_the_ticks_first(self):
        reader = opendaq.MultiReader2(self.params())
        reader.read(0)
        self.send_all(5, 10)
        self.wait_for(reader, 10)

        ticks, values, status = reader.read_with_domain(10)
        self.assertTrue(np.array_equal(ticks, np.arange(5, 15)))
        self.assertTrue(np.array_equal(values[1], np.arange(5, 15)))
        self.assertFalse(status.has_changes)

    def test_skip_samples(self):
        reader = opendaq.MultiReader2(self.params())
        reader.read(0)
        self.send_all(0, 10)
        self.wait_for(reader, 10)

        skipped, offset, status = reader.skip_samples(4)
        self.assertEqual((skipped, offset), (4, 0))
        self.assertFalse(status.has_changes)
        values, offset, status = reader.read(6)
        self.assertEqual(offset, 4)
        self.assertTrue(np.array_equal(values[0], np.arange(4, 10)))

    def test_read_type_sets_the_dtype(self):
        reader = opendaq.MultiReader2(self.params(value_read_type=opendaq.SampleType.Int32))
        reader.read(0)
        self.send_all(0, 10)
        self.wait_for(reader, 10)

        values, offset, status = reader.read(10)
        self.assertEqual(values[1].dtype, np.int32)
        self.assertTrue(np.array_equal(values[1], np.arange(10)))

    def test_accepts_descriptor_sees_every_input_and_a_rejected_input_is_none(self):
        asked = []

        def accepts(component, value, domain, values):
            asked.append(len(values))
            return component.local_id != 'value1'

        params = self.params(error_policy=opendaq.MultiReader2ErrorPolicy.Exclude)
        params.accepts_descriptor = accepts
        reader = opendaq.MultiReader2(params)
        values, offset, status = reader.read(0)
        self.assertEqual(asked, [2, 2])
        self.assertEqual(status.status, opendaq.ComponentStatus.Warning)
        self.assertEqual(status.get_input_status(self.signals[1]).error, opendaq.MultiReader2InputError.ValueDescriptorInvalid)

        self.send_all(0, 10)
        self.wait_for(reader, 10)
        values, offset, status = reader.read(10)
        self.assertTrue(np.array_equal(values[0], np.arange(10)))
        self.assertIsNone(values[1])

    def test_on_data_available_calls_a_handler(self):
        reader = opendaq.MultiReader2(self.params())
        reader.read(0)
        calls = []
        handler = opendaq.EventHandler(lambda sender, args: calls.append(sender))
        reader.on_data_available + handler

        self.send_all(0, 10)
        deadline = time.monotonic() + 2
        while not calls and time.monotonic() < deadline:
            time.sleep(0.001)
        self.assertTrue(calls)
        self.assertIsNone(calls[0])
        reader.on_data_available - handler


if __name__ == '__main__':
    unittest.main()
