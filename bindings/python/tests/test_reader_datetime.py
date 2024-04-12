#!/usr/bin/env python

import unittest
import opendaq_test
import opendaq
import numpy


class TestReaderDateTime(opendaq_test.TestCase):

    def try_read(self, reader, blocks):
        values = None
        i = 0

        while reader.available_count != 0:
            temp_values = reader.read(blocks)
            if values is None and len(temp_values) != 0:
                values = numpy.empty((blocks, reader.block_size), dtype = temp_values.dtype)
            for value in temp_values:
                values[i] = value
                i += 1
                if i == blocks:
                    break
        
        return values
    
    def try_read_with_domain(self, reader, blocks):
        values = None
        domain = None
        i = 0

        while reader.available_count != 0:
            (temp_values, temp_domain) = reader.read_with_domain(blocks)
            if values is None and len(temp_values) != 0:
                values = numpy.empty((blocks, reader.block_size), dtype = temp_values.dtype)
                domain = numpy.empty((blocks, reader.block_size), dtype = temp_domain.dtype)
            for idx in range(len(temp_values)):
                values[i] = temp_values[idx]
                domain[i] = temp_domain[idx]
                i += 1
                if i == blocks:
                    break
        
        return values, domain
    
    def try_read_with_timestamps(self, time_reader, block_reader, blocks):
        values = None
        domain = None
        i = 0

        while block_reader.available_count != 0:
            (temp_values, temp_domain) = time_reader.read_with_timestamps(blocks)
            if values is None and len(temp_values) != 0:
                values = numpy.empty((blocks, block_reader.block_size), dtype = temp_values.dtype)
                domain = numpy.empty((blocks, block_reader.block_size), dtype = temp_domain.dtype)
            for idx in range(len(temp_values)):
                values[i] = temp_values[idx]
                domain[i] = temp_domain[idx]
                i += 1
                if i == blocks:
                    break
        
        return values, domain

    def test_read(self):
        mock = opendaq.MockSignal()
        reader = opendaq.StreamReader(mock.signal)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)

        values = reader.read(10)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for v in values:
            self.assertIsInstance(v, numpy.float64)

    def test_read_override_value_type(self):
        mock = opendaq.MockSignal()
        reader = opendaq.StreamReader(
            mock.signal, value_type=opendaq.SampleType.Int64)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)

        values = reader.read(10)
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

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)

        (values, domain) = reader.read_with_domain(10)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for t in domain:
            self.assertIsInstance(t, numpy.int64)

    def test_read_with_timestamps(self):
        mock = opendaq.MockSignal()
        stream = opendaq.StreamReader(mock.signal)
        reader = opendaq.TimeStreamReader(stream)

        mock.add_data(numpy.arange(10))

        (values, domain) = reader.read_with_timestamps(10)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for t in domain:
            self.assertIsInstance(t, numpy.datetime64)

    def test_tail_read(self):
        mock = opendaq.MockSignal()
        reader = opendaq.TailReader(mock.signal, 10)

        self.assertEqual(reader.history_size, 10)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)

        values = reader.read(10)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for v in values:
            self.assertIsInstance(v, numpy.float64)

    def test_tail_read_override_value_type(self):
        mock = opendaq.MockSignal()
        reader = opendaq.TailReader(
            mock.signal, 10, value_type=opendaq.SampleType.Int64)

        self.assertEqual(reader.history_size, 10)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)

        values = reader.read(10)
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

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)

        (values, domain) = reader.read_with_domain(10)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for t in domain:
            self.assertIsInstance(t, numpy.int64)

    def test_tail_read_with_timestamps(self):
        mock = opendaq.MockSignal()
        tail = opendaq.TailReader(mock.signal, 10)
        reader = opendaq.TimeTailReader(tail)

        mock.add_data(numpy.arange(10))

        (values, domain) = reader.read_with_timestamps(10)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for t in domain:
            self.assertIsInstance(t, numpy.datetime64)

    def test_block_read(self):
        mock = opendaq.MockSignal()
        reader = opendaq.BlockReader(mock.signal, 2)

        self.assertEqual(reader.block_size, 2)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 5)
        
        values = self.try_read(reader, 5)
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

        self.assertEqual(reader.block_size, 2)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 5)

        values = self.try_read(reader, 5)
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

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 5)

        (values, domain) = self.try_read_with_domain(reader, 5)
        self.assertTrue(numpy.array_equal(
            values, numpy.arange(10).reshape(5, 2)))

        for t in domain:
            self.assertIsInstance(t, numpy.ndarray)
            for tt in t:
                self.assertIsInstance(tt, numpy.int64)

    def test_block_read_with_timestamps(self):
        mock = opendaq.MockSignal()
        block = opendaq.BlockReader(mock.signal, 2)
        reader = opendaq.TimeBlockReader(block)

        mock.add_data(numpy.arange(10))
        self.assertEqual(block.available_count, 5)

        (values, domain) = self.try_read_with_timestamps(reader, block, 5)
        self.assertTrue(numpy.array_equal(
            values, numpy.arange(10).reshape(5, 2)))

        for t in domain:
            self.assertIsInstance(t, numpy.ndarray)
            for tt in t:
                self.assertIsInstance(tt, numpy.datetime64)


if __name__ == '__main__':
    unittest.main()
