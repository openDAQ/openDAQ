import opendaq as daq
import unittest
import gc

class TestCase(unittest.TestCase):
    def setUp(self):
        daq.clear_tracked_objects()
        self.expect_memory_leak = False

    def tearDown(self):
        daq.clear_error_info()
        daq.event_queue.clear()
        gc.collect()
        cnt = daq.get_tracked_object_count()
        if not self.expect_memory_leak:
            if cnt > 0:
                print(f'Warning: {cnt} OpenDAQ objects still alive after test teardown')
                daq.print_tracked_objects()
            self.assertEqual(cnt, 0, 'Some OpenDAQ objects still alive')
