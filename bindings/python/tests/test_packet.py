#!/usr/bin/env python

import opendaq_test
import opendaq
import unittest
import numpy as np

class TestPacket(opendaq_test.TestCase):

    def test_packet(self):
        test_raw_data = np.arange(0, 10, 1, dtype=np.int32)
        test_scaled_data = np.arange(0.0, 1.0, 0.1, dtype=np.float64)
    
        ctx = opendaq.NullContext()
    
        time_signal = opendaq.Signal(ctx, None, "time", None)
        value_signal = opendaq.Signal(ctx, None, "value", None)
        value_signal.domain_signal = time_signal
        
        time_desc_builder = opendaq.DataDescriptorBuilder()
        time_desc_builder.tick_resolution = opendaq.Ratio(1, 1000)
        time_desc_builder.unit = opendaq.Unit(-1, "s", "second", "time")
        time_desc_builder.sample_type = opendaq.SampleType.Int64
        time_desc_builder.rule = opendaq.LinearDataRule(1, 0)
        time_desc = time_desc_builder.build()    
        time_signal.descriptor = time_desc

        value_desc_builder = opendaq.DataDescriptorBuilder()
        value_desc_builder.sample_type = opendaq.SampleType.Float64
        value_desc_builder.post_scaling = opendaq.LinearScaling(0.1, 0.0, opendaq.SampleType.Int32, opendaq.ScaledSampleType.Float64)
        value_desc = value_desc_builder.build()    
        value_signal.descriptor = value_desc
        
        reader = opendaq.StreamReader(value_signal)
        
        time_packet = opendaq.DataPacket(time_desc, 10, 0)
        time_signal.send_packet(time_packet)
        
        value_packet = opendaq.DataPacketWithDomain(time_packet, value_desc, 10, 0)

        raw_value_data = np.frombuffer(value_packet.raw_data, dtype=np.int32)
        np.copyto(raw_value_data, test_raw_data)        

        value_signal.send_packet(value_packet)
        
        reader.read(0)
        values, time = reader.read_with_domain(10)
        
        self.assertTrue(np.array_equal(values, test_scaled_data))
        self.assertTrue(np.array_equal(time, np.arange(10, dtype=np.int64)))

if __name__ == '__main__':
    unittest.main()
