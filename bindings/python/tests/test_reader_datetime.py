#!/usr/bin/env python

import unittest
import opendaq_test
import opendaq
import numpy


class TestReaderDateTime(opendaq_test.TestCase):

    def test_read(self):
        mock = opendaq.MockSignal()
        reader = opendaq.StreamReader(mock.signal)
        reader.read(0)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)

        values, status = reader.read(10)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for v in values:
            self.assertIsInstance(v, numpy.float64)

    def test_read_override_value_type(self):
        mock = opendaq.MockSignal()
        reader = opendaq.StreamReader(
            mock.signal, value_type=opendaq.SampleType.Int64)
        reader.read(0)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)

        values, status = reader.read(10)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for v in values:
            self.assertIsInstance(v, numpy.int64)

    def test_unsupported_value_type(self):
        mock = opendaq.MockSignal()
        with self.assertRaises(RuntimeError):
            opendaq.StreamReader(
                mock.signal, value_type=opendaq.SampleType.RangeInt64)

    def test_read_with_domain(self):
        mock = opendaq.MockSignal()
        reader = opendaq.StreamReader(mock.signal)
        reader.read(0)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)

        values, domain, status = reader.read_with_domain(10)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for t in domain:
            self.assertIsInstance(t, numpy.int64)

    def test_read_with_timestamps(self):
        mock = opendaq.MockSignal()
        stream = opendaq.StreamReader(mock.signal)
        stream.read(0)
        reader = opendaq.TimeStreamReader(stream)

        mock.add_data(numpy.arange(10))

        values, domain, status = reader.read_with_timestamps(10)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for t in domain:
            self.assertIsInstance(t, numpy.datetime64)

    def test_tail_read(self):
        mock = opendaq.MockSignal()
        reader = opendaq.TailReader(mock.signal, 10)
        reader.read(0)

        self.assertEqual(reader.history_size, 10)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)

        values, status = reader.read(10)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for v in values:
            self.assertIsInstance(v, numpy.float64)

    def test_tail_read_override_value_type(self):
        mock = opendaq.MockSignal()
        reader = opendaq.TailReader(
            mock.signal, 10, value_type=opendaq.SampleType.Int64)
        reader.read(0)

        self.assertEqual(reader.history_size, 10)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)

        values, status = reader.read(10)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for v in values:
            self.assertIsInstance(v, numpy.int64)

    def test_tail_unsupported_value_type(self):
        mock = opendaq.MockSignal()
        with self.assertRaises(RuntimeError):
            opendaq.TailReader(
                mock.signal, 100, value_type=opendaq.SampleType.RangeInt64)

    def test_tail_read_with_domain(self):
        mock = opendaq.MockSignal()
        reader = opendaq.TailReader(mock.signal, 10)
        reader.read(0)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)

        values, domain, status = reader.read_with_domain(10)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for t in domain:
            self.assertIsInstance(t, numpy.int64)

    def test_tail_read_with_timestamps(self):
        mock = opendaq.MockSignal()
        tail = opendaq.TailReader(mock.signal, 10)
        tail.read(0)
        reader = opendaq.TimeTailReader(tail)

        mock.add_data(numpy.arange(10))

        values, domain, status = reader.read_with_timestamps(10)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for t in domain:
            self.assertIsInstance(t, numpy.datetime64)

    def test_block_read(self):
        mock = opendaq.MockSignal()
        reader = opendaq.BlockReader(mock.signal, 2)
        reader.read(0)

        self.assertEqual(reader.block_size, 2)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 5)

        values, status = reader.read(5)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(
            values, numpy.arange(10).reshape(5, 2)))

        for v in values:
            self.assertIsInstance(v, numpy.ndarray)
            for vv in v:
                self.assertIsInstance(vv, numpy.float64)

    def test_block_read_override_value_type(self):
        mock = opendaq.MockSignal()
        reader = opendaq.BlockReader(
            mock.signal, 2, value_type=opendaq.SampleType.Int64)
        reader.read(0)

        self.assertEqual(reader.block_size, 2)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 5)

        values, status = reader.read(5)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(
            values, numpy.arange(10).reshape(5, 2)))

        for v in values:
            self.assertIsInstance(v, numpy.ndarray)
            for vv in v:
                self.assertIsInstance(vv, numpy.int64)

    def test_block_unsupported_value_type(self):
        mock = opendaq.MockSignal()
        with self.assertRaises(RuntimeError):
            opendaq.BlockReader(
                mock.signal, 2, value_type=opendaq.SampleType.RangeInt64)

    def test_block_read_with_domain(self):
        mock = opendaq.MockSignal()
        reader = opendaq.BlockReader(mock.signal, 2)
        reader.read(0)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 5)

        values, domain, status = reader.read_with_domain(5)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(
            values, numpy.arange(10).reshape(5, 2)))

        for t in domain:
            self.assertIsInstance(t, numpy.ndarray)
            for tt in t:
                self.assertIsInstance(tt, numpy.int64)

    def test_block_read_with_timestamps(self):
        mock = opendaq.MockSignal()
        block = opendaq.BlockReader(mock.signal, 2)
        block.read(0)
        reader = opendaq.TimeBlockReader(block)

        mock.add_data(numpy.arange(10))
        self.assertEqual(block.available_count, 5)

        values, domain, status = reader.read_with_timestamps(5)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(
            values, numpy.arange(10).reshape(5, 2)))

        for t in domain:
            self.assertIsInstance(t, numpy.ndarray)
            for tt in t:
                self.assertIsInstance(tt, numpy.datetime64)

    def test_multireader_read(self):
        epoch = opendaq.MockSignal.current_epoch()

        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        sigs = opendaq.List()
        sigs.append(sig1.signal)
        sigs.append(sig2.signal)

        reader = opendaq.MultiReader(sigs)
        reader.read(0)

        nparray = numpy.arange(10)
        sig1.add_data(nparray)
        sig2.add_data(nparray)

        self.assertEqual(reader.available_count, 10)

        values, status = reader.read(10)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values[0], numpy.arange(10)))

        for v in values[0]:
            self.assertIsInstance(v, numpy.float64)

    def test_multireader_read_override_value_type(self):

        epoch = opendaq.MockSignal.current_epoch()

        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        sigs = opendaq.List()
        sigs.append(sig1.signal)
        sigs.append(sig2.signal)

        reader = opendaq.MultiReader(sigs, value_type=opendaq.SampleType.Int64)
        reader.read(0)

        nparray = numpy.arange(10)
        sig1.add_data(nparray)
        sig2.add_data(nparray)

        self.assertEqual(reader.available_count, 10)

        values, status = reader.read(10)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values[0], numpy.arange(10)))

        for v in values[0]:
            self.assertIsInstance(v, numpy.int64)

    def test_multireader_unsupported_value_type(self):
        epoch = opendaq.MockSignal.current_epoch()

        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        sigs = opendaq.List()
        sigs.append(sig1.signal)
        sigs.append(sig2.signal)

        with self.assertRaises(RuntimeError):
            opendaq.MultiReader(sigs, value_type=opendaq.SampleType.RangeInt64)

    def test_multireader_read_with_domain(self):
        epoch = opendaq.MockSignal.current_epoch()

        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        sigs = opendaq.List()
        sigs.append(sig1.signal)
        sigs.append(sig2.signal)

        reader = opendaq.MultiReader(sigs)
        reader.read(0)

        nparray = numpy.arange(10)
        sig1.add_data(nparray)
        sig2.add_data(nparray)

        self.assertEqual(reader.available_count, 10)

        values, domain, status = reader.read_with_domain(10)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values[0], numpy.arange(10)))

        for t in domain[0]:
            self.assertIsInstance(t, numpy.int64)

    def test_multireader_read_with_timestamps(self):
        epoch = opendaq.MockSignal.current_epoch()

        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        sigs = opendaq.List()
        sigs.append(sig1.signal)
        sigs.append(sig2.signal)

        reader = opendaq.MultiReader(sigs)
        reader.read(0)
        timed = opendaq.TimeMultiReader(reader)

        nparray = numpy.arange(10)
        sig1.add_data(nparray)
        sig2.add_data(nparray)

        self.assertEqual(reader.available_count, 10)

        values, domain, status = timed.read_with_timestamps(10)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values[0], numpy.arange(10)))

        for t in domain[0]:
            self.assertIsInstance(t, numpy.datetime64)

    def test_multireader_builder(self):
        epoch = opendaq.MockSignal.current_epoch()
        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        builder = opendaq.MultiReaderBuilder()
        builder.add_signal(sig1.signal)
        builder.add_signal(sig2.signal)
        builder.value_read_type = opendaq.SampleType.Int64
        reader = builder.build()
        reader.read(0)

        nparray = numpy.arange(10)
        sig1.add_data(nparray)
        sig2.add_data(nparray)

        self.assertEqual(reader.available_count, 10)

        values, status = reader.read(10)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values[0], numpy.arange(10)))

        for v in values[0]:
            self.assertIsInstance(v, numpy.int64)


if __name__ == '__main__':
    unittest.main()
