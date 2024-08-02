import opendaq as daq
import unittest
import gc

class TestCase(unittest.TestCase):
    def setUp(self):
        self.cnt = daq.get_tracked_object_count()
        self.expect_memory_leak = False

    def tearDown(self):
        daq.clear_error_info()
        gc.collect()
        cnt = daq.get_tracked_object_count()
        if not self.expect_memory_leak:
            self.assertEqual(cnt - self.cnt, 0, 'Some OpenDAQ objects still alive')
