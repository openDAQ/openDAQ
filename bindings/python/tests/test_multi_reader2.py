import gc
import threading
import time
import unittest

import numpy as np
import opendaq as daq

import opendaq_test

ORIGIN = '2026-11-11T00:00:00+00:00'


def make_context():
    logger = daq.Logger([], daq.LogLevel.Warn)
    return daq.Context(daq.Scheduler(logger, 1), logger, daq.TypeManager(), None, None, {}, {})


def make_signal(ctx, name, unit='V', dims=None, rate=1000):
    domain_bldr = daq.DataDescriptorBuilder()
    domain_bldr.sample_type = daq.SampleType.Int64
    domain_bldr.tick_resolution = daq.Ratio(1, rate)
    domain_bldr.rule = daq.LinearDataRule(1, 0)
    domain_bldr.unit = daq.Unit(-1, 's', 'second', 'time')
    domain_bldr.origin = ORIGIN

    value_bldr = daq.DataDescriptorBuilder()
    value_bldr.sample_type = daq.SampleType.Float64
    value_bldr.unit = daq.Unit(-1, unit, '', '')
    if dims:
        value_bldr.dimensions = [daq.Dimension(daq.LinearDimensionRule(1, 0, dims), daq.Unit(-1, '', '', ''), 'bins')]

    domain = daq.Signal(ctx, None, name + '_time', None)
    domain.descriptor = domain_bldr.build()
    signal = daq.Signal(ctx, None, name, None)
    signal.descriptor = value_bldr.build()
    signal.domain_signal = domain
    return daq.ISignal.cast_from(signal)


def send(signal, first_tick, values):
    values = np.asarray(values, dtype=np.float64)
    count = values.shape[0]
    domain_packet = daq.DataPacket(signal.domain_signal.descriptor, count, first_tick)
    packet = daq.DataPacketWithDomain(domain_packet, signal.descriptor, count, 0)
    np.copyto(np.frombuffer(packet.raw_data, np.float64), values.reshape(-1))
    daq.ISignalConfig.cast_from(signal).send_packet(packet)


def wait_until(predicate, timeout=3.0):
    deadline = time.monotonic() + timeout
    while not predicate():
        if time.monotonic() > deadline:
            return False
        time.sleep(0.005)
    return True


class TestMultiReader2(opendaq_test.TestCase):

    def setUp(self):
        super().setUp()
        self.ctx = make_context()
        self.signals = [make_signal(self.ctx, f'sig{i}') for i in range(3)]

    def tearDown(self):
        self.signals = None
        self.ctx = None
        gc.collect()
        super().tearDown()

    def started(self, **kwargs):
        reader = daq.MultiReader2(self.signals, **kwargs)
        first = list(reader.drain())
        self.assertEqual(len(first), 1)
        self.assertTrue(first[0].status.has_changes)
        return reader

    def feed(self, reader, first_tick, count, signals=None):
        for i, signal in enumerate(signals or self.signals):
            send(signal, first_tick, np.arange(first_tick, first_tick + count) + 1000 * i)
        self.assertTrue(wait_until(lambda: reader.available_count >= count))

    def test_first_status(self):
        reader = daq.MultiReader2(self.signals)
        r = reader.read(10)
        self.assertEqual(r.count, 0)
        status = r.status
        self.assertTrue(status.has_changes)
        self.assertTrue(status.ok)
        self.assertEqual(status.health, daq.ComponentStatus.Ok)
        self.assertEqual(status.errors, {})
        self.assertTrue(status.domain_descriptor_changed)
        self.assertTrue(all(s.descriptor_changed for s in status.inputs))
        self.assertTrue(all(s.sample_size == 8 for s in status.inputs))
        self.assertIn('Ok', repr(status))
        self.assertEqual(status.main_input.global_id, self.signals[0].global_id)

    def test_read_result(self):
        reader = self.started()
        self.feed(reader, 0, 100)
        r = reader.read(60)
        self.assertEqual(r.count, 60)
        self.assertEqual(r.offset, 0)
        self.assertFalse(r.status.has_changes)
        np.testing.assert_array_equal(r[0], np.arange(60))
        np.testing.assert_array_equal(r[-1], np.arange(60) + 2000)
        np.testing.assert_array_equal(r[self.signals[1]], np.arange(60) + 1000)
        self.assertEqual(len(r.values), 3)
        self.assertEqual([s.global_id for s in r.inputs], [s.global_id for s in self.signals])
        for values in r.values:
            self.assertEqual(values.shape, (60,))
            self.assertEqual(values.dtype, np.float64)
            self.assertTrue(values.flags.c_contiguous)
        self.assertFalse(np.shares_memory(r[0], r[1]))
        np.testing.assert_array_equal(np.stack(r.values)[:, 0], [0, 1000, 2000])
        with self.assertRaises(IndexError):
            r[3]
        with self.assertRaises(KeyError):
            r[make_signal(self.ctx, 'other')]

    def test_read_is_capped_at_available(self):
        reader = self.started()
        self.feed(reader, 0, 40)
        r = reader.read(1000)
        self.assertEqual(r.count, 40)
        self.assertTrue(all(v.flags.c_contiguous and v.shape == (40,) for v in r.values))
        self.assertEqual(reader.read(10).values[0].shape, (0,))

    def test_ticks_and_times(self):
        reader = self.started()
        self.feed(reader, 0, 50)
        reader.read(20)
        r = reader.read(30)
        self.assertEqual(r.offset, 20)
        np.testing.assert_array_equal(r.ticks, np.arange(20, 50))
        expected = np.datetime64('2026-11-11T00:00:00', 'ns') + np.arange(20, 50) * np.timedelta64(1, 'ms')
        np.testing.assert_array_equal(r.times, expected)
        self.assertEqual(r.times.dtype, np.dtype('datetime64[ns]'))

    def test_read_into_2d_array(self):
        # A 2D array is a sequence of rows: one buffer per input, and the caller owns the matrix
        reader = self.started()
        self.feed(reader, 0, 50)
        matrix = np.zeros((3, 64))
        r = reader.read_into(matrix, 50)
        self.assertEqual(r.count, 50)
        np.testing.assert_array_equal(matrix[:, :50], np.stack([np.arange(50) + 1000 * i for i in range(3)]))
        self.assertTrue(np.shares_memory(r[0], matrix))
        self.assertEqual(r[0].shape, (50,))

    def test_read_into_list(self):
        reader = self.started()
        self.feed(reader, 0, 20)
        buffers = [np.zeros(20), np.zeros(20), np.zeros(20)]
        r = reader.read_into(buffers, 20)
        np.testing.assert_array_equal(buffers[1], np.arange(20) + 1000)
        self.assertTrue(all(np.shares_memory(r[i], buffers[i]) for i in range(3)))

    def test_read_into_rejects_bad_buffers(self):
        reader = self.started()
        self.feed(reader, 0, 20)
        with self.assertRaises(TypeError):
            reader.read_into(np.zeros((3, 20), dtype=np.float32), 20)
        with self.assertRaises(ValueError):
            reader.read_into(np.zeros((2, 20)), 20)
        with self.assertRaises(ValueError):
            reader.read_into(np.zeros((3, 10)), 20)
        with self.assertRaises(ValueError):
            reader.read_into([np.zeros(20), None, np.zeros(20)], 20)   # a healthy input without a buffer
        with self.assertRaises(ValueError):
            reader.read_into(np.zeros((3, 40))[:, ::2], 20)              # rows not contiguous
        with self.assertRaises(ValueError):
            reader.read_into(np.zeros(60), 20)                          # 60 items for 3 inputs
        with self.assertRaises(TypeError):
            reader.read_into(np.zeros(3), 20)                           # items are scalars, not arrays
        self.assertEqual(reader.available_count, 20)

    def test_skip_samples(self):
        reader = self.started()
        self.feed(reader, 0, 30)
        count, offset, status = reader.skip_samples(10)
        self.assertEqual((count, offset), (10, 0))
        self.assertFalse(status.has_changes)
        r = reader.read(20)
        self.assertEqual(r.offset, 10)
        np.testing.assert_array_equal(r[0], np.arange(10, 30))

    def test_drain(self):
        reader = daq.MultiReader2(self.signals)
        for i, signal in enumerate(self.signals):
            send(signal, 0, np.arange(25) + 1000 * i)
        results = []

        def pull():
            results.extend(reader.drain())
            return sum(r.count for r in results) >= 25

        self.assertTrue(wait_until(pull))
        self.assertTrue(results[0].status.has_changes)
        self.assertEqual(sum(r.count for r in results), 25)
        self.assertEqual(list(reader.drain()), [])

    def test_configure_keeps_unnamed_fields(self):
        reader = daq.MultiReader2(self.signals, min_read_count=5, error_policy=daq.MultiReader2ErrorPolicy.Exclude)
        status = reader.configure(value_read_type=daq.SampleType.Int32)
        self.assertTrue(status.has_changes)
        params = reader.params
        self.assertEqual(params.value_read_type, daq.SampleType.Int32)
        self.assertEqual(params.min_read_count, 5)
        self.assertEqual(params.error_policy, daq.MultiReader2ErrorPolicy.Exclude)
        self.assertEqual(len(params.inputs), 3)

        self.feed(reader, 0, 10)
        r = reader.read(10)
        self.assertEqual(r[0].dtype, np.int32)
        np.testing.assert_array_equal(r[1], np.arange(10) + 1000)

    def test_configure_with_params_object(self):
        reader = self.started()
        params = daq.MultiReader2Params(inputs=self.signals[:2], value_read_type=daq.SampleType.Int64)
        status = reader.configure(params, min_read_count=2)
        self.assertEqual(len(status.inputs), 2)
        self.assertEqual(reader.params.min_read_count, 2)
        params.min_read_count = 7                                     # the reader keeps its own copy
        self.assertEqual(reader.params.min_read_count, 2)

    def test_rejected_arguments(self):
        with self.assertRaises(ValueError):
            daq.MultiReader2(self.signals, value_read_type=daq.SampleType.Undefined)
        with self.assertRaises(TypeError):
            daq.MultiReader2(self.signals, timeout_ms=10)
        reader = self.started()
        with self.assertRaises(ValueError):
            reader.configure(value_read_type=daq.SampleType.Undefined)
        with self.assertRaises(TypeError):                            # rejected before anything is applied
            reader.configure(min_read_count=3, on_data_available=5)
        self.assertEqual(reader.params.min_read_count, 1)
        with self.assertRaises(Exception):
            reader.configure(main_input=make_signal(self.ctx, 'stranger'))
        self.assertEqual(reader.params.value_read_type, daq.SampleType.Float64)

    def test_exclude_rejected_input(self):
        amps = make_signal(self.ctx, 'amps', unit='A')
        inputs = self.signals[:2] + [amps]
        reader = daq.MultiReader2(inputs,
                                  error_policy=daq.MultiReader2ErrorPolicy.Exclude,
                                  accepts_descriptor=lambda inp, desc, dom: desc.unit.symbol == 'V')
        status = reader.read(0).status
        self.assertEqual(status.health, daq.ComponentStatus.Warning)
        self.assertFalse(status.ok)
        self.assertEqual(list(status.errors.values()), [daq.MultiReader2InputError.ValueDescriptorInvalid])
        self.assertEqual(list(status.errors.keys())[0].global_id, amps.global_id)
        self.assertIn('amps', repr(status))

        self.feed(reader, 0, 10, signals=inputs)
        r = reader.read(10)
        self.assertEqual(r.count, 10)
        self.assertIsNone(r[2])
        self.assertIsNone(r[amps])
        self.assertEqual([v is None for v in r.values], [False, False, True])
        buffers = [np.zeros(10), np.zeros(10), None]                   # no buffer needed for the excluded input
        self.feed(reader, 10, 10, signals=inputs)
        r = reader.read_into(buffers, 10)
        self.assertEqual(r.count, 10)
        self.assertIsNone(r[2])

    def test_raising_accepts_is_a_rejection(self):
        def accepts(inp, desc, dom):
            raise RuntimeError('nope')
        reader = daq.MultiReader2(self.signals, accepts_descriptor=accepts)
        status = reader.read(0).status
        self.assertEqual(status.health, daq.ComponentStatus.Error)
        self.assertEqual(len(status.errors), 3)

    def test_accepts_cannot_call_its_own_reader(self):
        holder = [daq.MultiReader2(self.signals)]
        raised = []

        def accepts(inp, desc, dom):
            try:
                holder[0].read(0)
            except RuntimeError as error:
                raised.append(str(error))
                raise
            return True

        status = holder[0].configure(accepts_descriptor=accepts)
        self.assertEqual(status.health, daq.ComponentStatus.Error)
        self.assertEqual(len(status.errors), 3)
        self.assertIn('cannot call its own reader', raised[0])
        holder.clear()  # breaks the reader -> accepts -> reader cycle

    def test_vector_input(self):
        vectors = [make_signal(self.ctx, f'vec{i}', dims=4) for i in range(2)]
        reader = daq.MultiReader2(vectors)
        reader.read(0)
        for i, signal in enumerate(vectors):
            send(signal, 0, np.arange(40).reshape(10, 4) + 100 * i)
        self.assertTrue(wait_until(lambda: reader.available_count >= 10))
        r = reader.read(10)
        self.assertEqual(r[0].shape, (10, 4))
        buffer = np.zeros((2, 10, 4))
        for i, signal in enumerate(vectors):
            send(signal, 10, np.arange(40).reshape(10, 4) + 100 * i)
        self.assertTrue(wait_until(lambda: reader.available_count >= 10))
        reader.read_into(buffer, 10)
        np.testing.assert_array_equal(buffer[1, 0], [100, 101, 102, 103])
        np.testing.assert_array_equal(r[1][2], [108, 109, 110, 111])

    def test_mixed_dimensions(self):
        inputs = [self.signals[0], make_signal(self.ctx, 'vec', dims=2)]
        reader = daq.MultiReader2(inputs)
        reader.read(0)
        send(inputs[0], 0, np.arange(5))
        send(inputs[1], 0, np.arange(10).reshape(5, 2))
        self.assertTrue(wait_until(lambda: reader.available_count >= 5))
        r = reader.read(5)
        self.assertEqual(r[0].shape, (5,))
        self.assertEqual(r[1].shape, (5, 2))
        buffers = [np.zeros(5), np.zeros((5, 2))]
        self.feed_mixed(reader, inputs)
        r = reader.read_into(buffers, 5)
        np.testing.assert_array_equal(buffers[1][0], [10, 11])

    def feed_mixed(self, reader, inputs):
        send(inputs[0], 5, np.arange(5, 10))
        send(inputs[1], 5, np.arange(10, 20).reshape(5, 2))
        self.assertTrue(wait_until(lambda: reader.available_count >= 5))

    def test_on_data_available(self):
        calls = threading.Event()
        reader = daq.MultiReader2(self.signals, on_data_available=calls.set)
        reader.read(0)                                                # construction schedules no call
        self.feed(reader, 0, 10)
        self.assertTrue(calls.wait(3))

        reader.configure(on_data_available=None)
        reader.read(0)
        calls.clear()
        self.feed(reader, 10, 10)
        time.sleep(0.1)
        self.assertFalse(calls.is_set())

    def test_handler_drains(self):
        lock = threading.Lock()
        received = []

        def on_data():
            with lock:
                for r in reader.drain():
                    if r.count:
                        received.append(np.stack(r.values))

        reader = daq.MultiReader2(self.signals, on_data_available=on_data)
        for i, signal in enumerate(self.signals):
            send(signal, 0, np.arange(30) + 1000 * i)
        self.assertTrue(wait_until(lambda: sum(a.shape[1] for a in received) >= 30))
        with lock:
            data = np.concatenate(received, axis=1)
        np.testing.assert_array_equal(data[2], np.arange(30) + 2000)
        # The handler's closure refers to the reader: the cycle is collected at teardown (no objects leak)

    def test_min_read_count(self):
        reader = self.started(min_read_count=10)
        self.feed(reader, 0, 10)
        with self.assertRaises(Exception):
            reader.read(5)
        self.assertEqual(reader.read(10).count, 10)


if __name__ == '__main__':
    unittest.main()
