#!/usr/bin/env python

import opendaq_test
import os
import subprocess
import sys
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


if __name__ == '__main__':
    unittest.main()
