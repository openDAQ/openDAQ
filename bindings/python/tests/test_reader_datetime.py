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

        # check skip
        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)
        skip, status = reader.skip_samples(10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertEqual(skip, 10)
        self.assertEqual(reader.available_count, 0)

        # check read
        mock.add_data(numpy.arange(10))
        values, status = reader.read(10, return_status=True)
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

        values, status = reader.read(10, return_status=True)
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

        values, domain, status = reader.read_with_domain(
            10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for t in domain:
            self.assertIsInstance(t, numpy.int64)

    def test_read_with_domain_undefined_type(self):
        mock = opendaq.MockSignal()

        reader = opendaq.StreamReader(mock.signal,
                                      value_type=opendaq.SampleType.Undefined,
                                      domain_type=opendaq.SampleType.Undefined,
                                      skip_events=False)
        values, domain, status = reader.read_with_domain(0, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Event)
        self.assertEqual(status.event_packet.event_id, 'DATA_DESCRIPTOR_CHANGED')
        self.assertTrue(len(values) == 0 and len(domain) == 0)

        mock.add_data(numpy.arange(10))

        values, domain, status = reader.read_with_domain(10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(len(values) == 10 and len(domain) == 10)

        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))
        self.assertTrue(domain.dtype == numpy.int64)

    def test_read_with_timestamps(self):
        mock = opendaq.MockSignal()
        stream = opendaq.StreamReader(mock.signal)
        stream.read(0)
        reader = opendaq.TimeStreamReader(stream)

        mock.add_data(numpy.arange(10))

        values, domain, status = reader.read_with_timestamps(
            10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for t in domain:
            self.assertIsInstance(t, numpy.datetime64)

    def test_read_value_type_struct(self):
        mock = opendaq.MockSignal(initialize_value_descriptor=False)
        # for descriptor to be created
        mock.add_objects({"first": 0, "second": {"first": -10},
                          "third": [{"first": 3.5}], "fourth": [0, 1]})

        reader = opendaq.StreamReader(
            mock.signal,
            value_type=opendaq.SampleType.Undefined,
            skip_events=False)

        data, status = reader.read(0, return_status=True)

        mock.add_objects([{"first": 0,
                           "second": {"first": -10},
                           "third": [{"first": 3.5}],
                           "fourth": [0,
                                      1]},
                          {"first": 1,
                           "second": {"first": -11},
                           "third": [{"first": 4.5}],
                           "fourth": [1,
                                      2]}],
                         False)  # do not update descriptor here

        data, status = reader.read(2, return_status=True)
        self.assertEqual(status.read_status, opendaq.ReadStatus.Ok)
        self.assertEqual(data['first'][0], 0)
        self.assertEqual(data['second']['first'][1], -11)
        self.assertEqual(data['third'][1]['first'], 4.5)
        self.assertEqual(data['fourth'][0][1], 1)

    def test_tail_read(self):
        mock = opendaq.MockSignal()
        reader = opendaq.TailReader(mock.signal, 10)
        reader.read(0)

        self.assertEqual(reader.history_size, 10)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 10)

        values, status = reader.read(10, return_status=True)
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

        values, status = reader.read(10, return_status=True)
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

        values, domain, status = reader.read_with_domain(
            10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for t in domain:
            self.assertIsInstance(t, numpy.int64)

    def test_tail_read_with_domain_undefined_type(self):
        mock = opendaq.MockSignal()

        reader = opendaq.TailReader(mock.signal, 10, value_type=opendaq.SampleType.Undefined,
                                    domain_type=opendaq.SampleType.Undefined, skip_events=False)
        values, domain, status = reader.read_with_domain(0, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Event)
        self.assertEqual(status.event_packet.event_id, 'DATA_DESCRIPTOR_CHANGED')
        self.assertTrue(len(values) == 0 and len(domain) == 0)

        mock.add_data(numpy.arange(10))

        values, domain, status = reader.read_with_domain(10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(len(values) == 10 and len(domain) == 10)

        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))
        self.assertTrue(domain.dtype == numpy.int64)

    def test_tail_read_with_timestamps(self):
        mock = opendaq.MockSignal()
        tail = opendaq.TailReader(mock.signal, 10)
        tail.read(0)
        reader = opendaq.TimeTailReader(tail)

        mock.add_data(numpy.arange(10))

        values, domain, status = reader.read_with_timestamps(
            10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values, numpy.arange(10)))

        for t in domain:
            self.assertIsInstance(t, numpy.datetime64)

    def test_tail_read_value_type_struct(self):
        mock = opendaq.MockSignal(initialize_value_descriptor=False)
        mock.add_objects({"first": 0, "second": {"first": -10},
                          "third": [{"first": 3.5}], "fourth": [0, 1]})

        reader = opendaq.TailReader(mock.signal, 10, value_type=opendaq.SampleType.Undefined, skip_events=False)

        data, status = reader.read(0, return_status=True)

        mock.add_objects([{"first": 0,
                           "second": {"first": -10},
                           "third": [{"first": 3.5}],
                           "fourth": [0,
                                      1]},
                          {"first": 1,
                           "second": {"first": -11},
                           "third": [{"first": 4.5}],
                           "fourth": [1,
                                      2]}],
                         False)

        data, status = reader.read(2, return_status=True)
        self.assertEqual(status.read_status, opendaq.ReadStatus.Ok)
        self.assertEqual(data['first'][0], 0)
        self.assertEqual(data['second']['first'][1], -11)
        self.assertEqual(data['third'][1]['first'], 4.5)
        self.assertEqual(data['fourth'][0][1], 1)

    def test_block_read(self):
        mock = opendaq.MockSignal()
        reader = opendaq.BlockReader(mock.signal, 2)
        reader.read(0)

        self.assertEqual(reader.block_size, 2)

        mock.add_data(numpy.arange(10))
        self.assertEqual(reader.available_count, 5)

        values, status = reader.read(5, return_status=True)
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

        values, status = reader.read(5, return_status=True)
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

        values, domain, status = reader.read_with_domain(5, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(
            values, numpy.arange(10).reshape(5, 2)))

        for t in domain:
            self.assertIsInstance(t, numpy.ndarray)
            for tt in t:
                self.assertIsInstance(tt, numpy.int64)

    def test_block_read_with_domain_undefined_type(self):
        mock = opendaq.MockSignal()

        reader = opendaq.BlockReader(mock.signal, 2, value_type=opendaq.SampleType.Undefined,
                                     domain_type=opendaq.SampleType.Undefined, skip_events=False)
        values, domain, status = reader.read_with_domain(0, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Event)
        self.assertEqual(status.event_packet.event_id, 'DATA_DESCRIPTOR_CHANGED')
        self.assertTrue(len(values) == 0 and len(domain) == 0)

        mock.add_data(numpy.arange(10))

        values, domain, status = reader.read_with_domain(10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(len(values) == 5 and len(domain) == 5)

        self.assertTrue(numpy.array_equal(values, numpy.arange(10).reshape(5, 2)))
        self.assertTrue(domain.dtype == numpy.int64)

    def test_block_read_with_timestamps(self):
        mock = opendaq.MockSignal()
        block = opendaq.BlockReader(mock.signal, 2)
        block.read(0)
        reader = opendaq.TimeBlockReader(block)

        mock.add_data(numpy.arange(10))
        self.assertEqual(block.available_count, 5)

        values, domain, status = reader.read_with_timestamps(
            5, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(
            values, numpy.arange(10).reshape(5, 2)))

        for t in domain:
            self.assertIsInstance(t, numpy.ndarray)
            for tt in t:
                self.assertIsInstance(tt, numpy.datetime64)

    def test_block_read_value_type_struct(self):
        mock = opendaq.MockSignal(initialize_value_descriptor=False)
        mock.add_objects({"first": 0, "second": {"first": -10},
                          "third": [{"first": 3.5}], "fourth": [0, 1]})

        reader = opendaq.BlockReader(
            mock.signal, 2,
            value_type=opendaq.SampleType.Undefined,
            skip_events=False
        )

        data, status = reader.read(0, return_status=True)

        mock.add_objects([{"first": 0,
                           "second": {"first": -10},
                           "third": [{"first": 3.5}],
                           "fourth": [0,
                                      1]},
                          {"first": 1,
                           "second": {"first": -11},
                           "third": [{"first": 4.5}],
                           "fourth": [1,
                                      2]}],
                         False)

        data, status = reader.read(2, return_status=True)
        self.assertEqual(status.read_status, opendaq.ReadStatus.Ok)
        self.assertEqual(data[0]['first'][0], 0)
        self.assertEqual(data[0]['second']['first'][1], -11)
        self.assertEqual(data[0]['third'][1]['first'], 4.5)
        self.assertEqual(data[0]['fourth'][0][1], 1)

    def test_multireader_read(self):
        epoch = opendaq.MockSignal.current_epoch()

        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        sigs = opendaq.List()
        sigs.append(sig1.signal)
        sigs.append(sig2.signal)

        builder = opendaq.MultiReaderBuilder()
        builder.input_port_notification_method = opendaq.PacketReadyNotification.SameThread
        builder.add_signals(sigs)
        reader = builder.build()
        reader.read(0)

        nparray = numpy.arange(10)

        # check skip
        sig1.add_data(nparray)
        sig2.add_data(nparray)
        self.assertEqual(reader.available_count, 10)
        skip, status = reader.skip_samples(10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertEqual(skip, 10)
        self.assertEqual(reader.available_count, 0)

        # check read
        sig1.add_data(nparray)
        sig2.add_data(nparray)
        values, status = reader.read(10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values[0], numpy.arange(10)))

        for v in values[0]:
            self.assertIsInstance(v, numpy.float64)

    def test_multireader_read_override_value_type(self):

        epoch = opendaq.MockSignal.current_epoch()

        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        builder = opendaq.MultiReaderBuilder()
        builder.input_port_notification_method = opendaq.PacketReadyNotification.SameThread
        builder.add_signal(sig1.signal)
        builder.add_signal(sig2.signal)
        builder.value_read_type = opendaq.SampleType.Int64
        reader = builder.build()

        reader.read(0)

        nparray = numpy.arange(10)
        sig1.add_data(nparray)
        sig2.add_data(nparray)

        self.assertEqual(reader.available_count, 10)

        values, status = reader.read(10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values[0], numpy.arange(10)))

        for v in values[0]:
            self.assertIsInstance(v, numpy.int64)

    def test_multireader_unsupported_value_type(self):
        epoch = opendaq.MockSignal.current_epoch()

        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        with self.assertRaises(RuntimeError):
            builder = opendaq.MultiReaderBuilder()
            builder.input_port_notification_method = opendaq.PacketReadyNotification.SameThread
            builder.add_signal(sig1.signal)
            builder.add_signal(sig2.signal)
            builder.value_read_type = opendaq.SampleType.RangeInt64
            reader = builder.build()

    def test_multireader_unsupported_domain_type(self):
        epoch = opendaq.MockSignal.current_epoch()

        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        with self.assertRaises(RuntimeError):
            builder = opendaq.MultiReaderBuilder()
            builder.input_port_notification_method = opendaq.PacketReadyNotification.SameThread 
            builder.add_signal(sig1.signal)
            builder.add_signal(sig2.signal)
            builder.domain_read_type = opendaq.SampleType.Undefined
            reader = builder.build()

    def test_multireader_read_with_domain(self):
        epoch = opendaq.MockSignal.current_epoch()

        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        builder = opendaq.MultiReaderBuilder()
        builder.input_port_notification_method = opendaq.PacketReadyNotification.SameThread
        builder.add_signal(sig1.signal)
        builder.add_signal(sig2.signal)
        reader = builder.build()

        reader.read(0)

        nparray = numpy.arange(10)
        sig1.add_data(nparray)
        sig2.add_data(nparray)

        self.assertEqual(reader.available_count, 10)

        values, domain, status = reader.read_with_domain(
            10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values[0], numpy.arange(10)))

        for t in domain[0]:
            self.assertIsInstance(t, numpy.int64)

    def test_multireader_read_with_timestamps(self):
        epoch = opendaq.MockSignal.current_epoch()

        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        builder = opendaq.MultiReaderBuilder()
        builder.input_port_notification_method = opendaq.PacketReadyNotification.SameThread
        builder.add_signal(sig1.signal)
        builder.add_signal(sig2.signal)
        reader = builder.build()

        reader.read(0)
        timed = opendaq.TimeMultiReader(reader)

        nparray = numpy.arange(10)
        sig1.add_data(nparray)
        sig2.add_data(nparray)

        self.assertEqual(reader.available_count, 10)

        values, domain, status = timed.read_with_timestamps(
            10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values[0], numpy.arange(10)))

        for t in domain[0]:
            self.assertIsInstance(t, numpy.datetime64)

    def test_multireader_read_domain_undefined_type(self):
        epoch = opendaq.MockSignal.current_epoch()

        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        builder = opendaq.MultiReaderBuilder()
        builder.input_port_notification_method = opendaq.PacketReadyNotification.SameThread
        builder.add_signal(sig1.signal)
        builder.add_signal(sig2.signal)
        builder.value_read_type = opendaq.SampleType.Undefined
        reader = builder.build()

        values, domain, status = reader.read_with_domain(0, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Event)
        self.assertEqual(
            status.main_descriptor.event_id,
            'DATA_DESCRIPTOR_CHANGED')
        self.assertTrue(len(values) == 0 and len(domain) == 0)

        nparray = numpy.arange(10)
        sig1.add_data(nparray)
        sig2.add_data(nparray)

        values, domain, status = reader.read_with_domain(
            10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(len(values[0]) == 10 and len(values[1]) == 10 and len(
            domain[0]) == 10 and len(domain[1]) == 10)

        self.assertTrue(numpy.array_equal(values[0], numpy.arange(10)))
        self.assertTrue(numpy.array_equal(values[1], numpy.arange(10)))
        self.assertTrue(domain.dtype == numpy.int64)

    def test_multireader_read_value_type_struct(self):
        epoch = opendaq.MockSignal.current_epoch()

        sig1 = opendaq.MockSignal('sig1', epoch, False)
        sig2 = opendaq.MockSignal('sig2', epoch, False)

        sig1.add_objects({"first": 0, "second": {"first": -10},
                          "third": [{"first": 3.5}], "fourth": [0, 1]})
        sig2.add_objects({"first": 0, "second": {"first": -10},
                          "third": [{"first": 3.5}], "fourth": [0, 1]})

        builder = opendaq.MultiReaderBuilder()
        builder.input_port_notification_method = opendaq.PacketReadyNotification.SameThread
        builder.add_signal(sig1.signal)
        builder.add_signal(sig2.signal)
        builder.value_read_type = opendaq.SampleType.Struct
        reader = builder.build()

        data, domain, status = reader.read_with_domain(0, return_status=True)

        sig1.add_objects([{"first": 0,
                           "second": {"first": -10},
                           "third": [{"first": 3.5}],
                           "fourth": [0,
                                      1]},
                          {"first": 1,
                           "second": {"first": -11},
                           "third": [{"first": 4.5}],
                           "fourth": [1,
                                      2]}],
                         False)

        sig2.add_objects([{"first": 1,
                           "second": {"first": -11},
                           "third": [{"first": 4.5}],
                           "fourth": [1,
                                      2]},
                          {"first": 2,
                           "second": {"first": -12},
                           "third": [{"first": 5.5}],
                           "fourth": [2,
                                      3]}],
                         False)

        data, domain, status = reader.read_with_domain(2, return_status=True)

        self.assertEqual(status.read_status, opendaq.ReadStatus.Ok)
        self.assertEqual(len(data), len(domain))
        self.assertListEqual(data['first'].tolist(), [[0, 1], [1, 2]])
        self.assertListEqual(data['second']['first'].tolist(), [
            [-10, -11], [-11, -12]])
        self.assertListEqual(
            data['third'][0]['first'].tolist(), [
                [3.5], [4.5]])
        self.assertListEqual(data['fourth'].tolist(), [
            [[0, 1], [1, 2]], [[1, 2], [2, 3]]])

    def test_multireader_builder(self):
        epoch = opendaq.MockSignal.current_epoch()
        sig1 = opendaq.MockSignal('sig1', epoch)
        sig2 = opendaq.MockSignal('sig2', epoch)

        builder = opendaq.MultiReaderBuilder()
        builder.input_port_notification_method = opendaq.PacketReadyNotification.SameThread
        builder.add_signal(sig1.signal)
        builder.add_signal(sig2.signal)
        builder.value_read_type = opendaq.SampleType.Int64
        reader = builder.build()
        reader.read(0)

        nparray = numpy.arange(10)
        sig1.add_data(nparray)
        sig2.add_data(nparray)

        self.assertEqual(reader.available_count, 10)

        values, status = reader.read(10, return_status=True)
        self.assertTrue(status.read_status == opendaq.ReadStatus.Ok)
        self.assertTrue(status.valid)
        self.assertTrue(numpy.array_equal(values[0], numpy.arange(10)))

        for v in values[0]:
            self.assertIsInstance(v, numpy.int64)


if __name__ == '__main__':
    unittest.main()
