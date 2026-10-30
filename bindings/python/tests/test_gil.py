#!/usr/bin/env python

import opendaq_test
import os
import re
import subprocess
import sys
import tempfile
import textwrap
import unittest


# Python objects without an openDAQ equivalent are wrapped into a C++ PyObjectImpl.
# Releasing the last reference to such a wrapper from C++ while the GIL is released
# must not touch the Python reference count without re-acquiring the GIL.
# Without the GIL, debug builds of pybind11 abort the process (std::terminate), so each
# scenario runs in a separate interpreter and the test checks its exit code.
PREAMBLE = textwrap.dedent('''
    import opendaq

    class Foo:
        pass

    property_object = opendaq.PropertyObject()
    property_object.add_property(opendaq.ListProperty(
        opendaq.String('list'), opendaq.List(), opendaq.Boolean(True)))
''')


class TestGil(opendaq_test.TestCase):

    def run_scenario(self, code):
        env = dict(os.environ)
        env['PYTHONPATH'] = os.pathsep.join(filter(None, [os.getcwd()] + sys.path))
        result = subprocess.run([sys.executable, '-c', PREAMBLE + textwrap.dedent(code)],
                                capture_output=True, text=True, cwd=os.getcwd(), env=env, timeout=60)
        self.assertEqual(result.returncode, 0,
                         f'Scenario exited with {result.returncode}\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}')
        return result.stdout

    def test_wrapped_py_object_released_during_unwind(self):
        out = self.run_scenario('''
            try:
                property_object.set_property_value('missing', Foo())
            except RuntimeError:
                print('raised')
        ''')
        self.assertIn('raised', out)

    def test_wrapped_py_object_released_on_overwrite(self):
        self.run_scenario('''
            property_object.set_property_value('list', [Foo()])
            property_object.set_property_value('list', [])
        ''')

    def test_wrapped_py_function_released_on_worker_thread(self):
        out = self.run_scenario('''
            import threading

            scheduler = opendaq.Scheduler(opendaq.Logger(opendaq.List(), opendaq.LogLevel.Info), 1)
            references_dropped = threading.Event()
            freed = threading.Event()
            freed_on_main = None

            class Callback:
                def __call__(self):
                    # Return only after Python dropped its references, so the worker holds the last one
                    references_dropped.wait()

                def __del__(self):
                    global freed_on_main
                    freed_on_main = threading.current_thread() is threading.main_thread()
                    freed.set()

            awaitable = scheduler.schedule_function(opendaq.Function(Callback()))
            references_dropped.set()
            # The worker can drop the task after wait_all returns, so wait for the release itself
            scheduler.wait_all()
            if freed.wait(10):
                print('freed on main thread' if freed_on_main else 'freed on worker thread')
        ''')
        self.assertIn('freed on worker thread', out)

    def test_scheduler_released_while_worker_releases_py_function(self):
        out = self.run_scenario('''
            import threading

            for _ in range(100):
                scheduler = opendaq.Scheduler(opendaq.Logger(opendaq.List(), opendaq.LogLevel.Info), 1)
                references_dropped = threading.Event()

                class Callback:
                    def __call__(self):
                        references_dropped.wait()

                awaitable = scheduler.schedule_function(opendaq.Function(Callback()))
                references_dropped.set()
                scheduler.wait_all()
                del awaitable
                # Joins the worker, which may still be releasing the last reference to the function
                del scheduler
            print('completed')
        ''')
        self.assertIn('completed', out)

    def test_scheduler_released_while_py_function_runs(self):
        out = self.run_scenario('''
            import time

            for _ in range(20):
                scheduler = opendaq.Scheduler(opendaq.Logger(opendaq.List(), opendaq.LogLevel.Info), 1)

                class Callback:
                    def __call__(self):
                        # Drops the GIL and needs it back to finish
                        time.sleep(0.01)

                awaitable = scheduler.schedule_function(opendaq.Function(Callback()))
                del awaitable
                # Joins the worker while the callback is still running
                del scheduler
            print('completed')
        ''')
        self.assertIn('completed', out)

    def test_worker_releases_py_object_at_exit(self):
        count = 200_000
        out = self.run_scenario(f'''
            import atexit
            import threading
            import time

            scheduler = opendaq.Scheduler(opendaq.Logger(opendaq.List(), opendaq.LogLevel.Info), 1)
            big = (1 << 6_000_000) - 1
            main_thread = threading.get_ident()
            released_on_worker = 0

            class Item:
                def __del__(self):
                    global released_on_worker
                    if threading.get_ident() != main_thread:
                        released_on_worker += 1

            class Callback:
                def __call__(self):
                    # The worker releases the wrapped items one by one
                    return [Item() for _ in range({count})]

            def hold_gil():
                deadline = time.monotonic() + 10
                while released_on_worker == 0 and time.monotonic() < deadline:
                    time.sleep(0.001)
                # Holds the GIL until Python starts finalizing
                big * big
                print('released on worker before exit:', released_on_worker, flush=True)

            awaitable = scheduler.schedule_function(opendaq.Function(Callback()))
            del awaitable
            atexit.register(hold_gil)
        ''')
        match = re.search(r'released on worker before exit: (\d+)', out)
        self.assertIsNotNone(match, out)
        # Otherwise the worker wasn't releasing objects when Python started finalizing
        self.assertTrue(0 < int(match.group(1)) < count, out)

    def test_worker_calls_py_function_at_exit(self):
        out = self.run_scenario('''
            import threading
            import time

            scheduler = opendaq.Scheduler(opendaq.Logger(opendaq.List(), opendaq.LogLevel.Info), 1)
            started = threading.Event()

            class Running:
                def __call__(self):
                    started.set()
                    # The main thread exits meanwhile and waits for this callback
                    time.sleep(1)
                    print('running completed', flush=True)

            class Pending:
                def __call__(self):
                    print('pending called', flush=True)

            running = scheduler.schedule_function(opendaq.Function(Running()))
            pending = scheduler.schedule_function(opendaq.Function(Pending()))
            started.wait()
        ''')
        self.assertIn('running completed', out)
        self.assertNotIn('pending called', out)

    def test_py_function_releases_py_object_at_exit(self):
        out = self.run_scenario('''
            import threading
            import time

            scheduler = opendaq.Scheduler(opendaq.Logger(opendaq.List(), opendaq.LogLevel.Info), 1)
            started = threading.Event()

            class Item:
                def __del__(self):
                    print('item released', flush=True)

            class Running:
                def __call__(self):
                    started.set()
                    # The main thread exits meanwhile and waits for this callback
                    time.sleep(1)
                    property_object.set_property_value('list', [Item()])
                    # Releases the wrapped item while Python waits for this callback
                    property_object.set_property_value('list', [])

            running = scheduler.schedule_function(opendaq.Function(Running()))
            started.wait()
        ''')
        self.assertIn('item released', out)

    def test_wrapped_py_object_released_in_module_teardown(self):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, 'out.txt')
            # The file object must not reference the main module, or module teardown can't release it
            self.run_scenario(f'''
                f = open({path!r}, 'w')
                # Flushed only when the file object is released
                f.write('released')
                property_object.set_property_value('list', [f])
                del f
            ''')
            with open(path) as f:
                self.assertEqual(f.read(), 'released')

    def test_worker_releases_event_handler_at_exit(self):
        out = self.run_scenario('''
            import atexit
            import threading

            scheduler = opendaq.Scheduler(opendaq.Logger(opendaq.List(), opendaq.LogLevel.Info), 1)
            big = (1 << 6_000_000) - 1
            started = threading.Event()
            returned = False

            class Callback:
                def __call__(self):
                    global returned
                    started.set()
                    handlers = [opendaq.EventHandler(lambda sender, args: None) for _ in range(200_000)]
                    returned = True
                    # The worker releases the handlers one by one
                    return handlers

            def hold_gil():
                started.wait()
                # Holds the GIL until Python starts finalizing
                big * big
                print('callback returned before exit:', returned, flush=True)

            awaitable = scheduler.schedule_function(opendaq.Function(Callback()))
            del awaitable
            atexit.register(hold_gil)
        ''')
        # Otherwise the worker released the handlers before Python started finalizing
        self.assertIn('callback returned before exit: False', out)


if __name__ == '__main__':
    unittest.main()
