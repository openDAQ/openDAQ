"""Example openDAQ Python plugin: a "Plotter" function block.

Demonstrates the Module/FunctionBlock plugin contract (see
bindings/python/package/opendaq/{module,function_block}.py) combined with DurationTailReader
(core/opendaq/reader/include/opendaq/duration_tail_reader.h): PlotterFb builds a reader over
whatever signal is connected to its one input port, then - once show() is called - polls that
reader and redraws a live window of the signal's trailing history on a timer. There is no
packet-driven callback at all here, just polling.

Like every openDAQ Python plugin, this file only defines create_module(context); loading it is
the caller's job. Runnable directly as `python plotter_module.py` - see main(), below - which
loads this same file into its own opendaq.Instance() via
IModuleManager.load_python_module(context, path) (bindings/python/opendaq/src/py_module_manager.cpp).
A plugin can also be embedded from a C++ host instead, the same way run_live_example.cpp (in
this directory) and bindings/python/py_opendaq_module/tests/test_python_module.cpp do:

    auto module = daq::createPythonModule(context, "plotter_module.py");
    context.getModuleManager().asPtr<daq::IModuleManager>().addModule(module);

Requires matplotlib and numpy (not openDAQ dependencies - install separately: `pip install
matplotlib numpy`).
"""

import contextlib
import threading
from time import sleep

import numpy as np

# An interactive backend (TkAgg, MacOSX, Qt...) is required to show a live window - unlike Agg,
# a pure off-screen renderer with no GUI event loop. Interactive backends generally require every
# GUI call (creating a Figure, drawing, running the event loop) to happen on the process's main
# thread. show() below is the only method that may touch matplotlib, and it must only ever be
# called from whichever thread calls main(), i.e. the process's main thread - every other
# PlotterFb method (__init__, on_connected(), on_disconnected(), ...) runs on PythonRuntime's own
# dispatch thread instead (see python_runtime.h), never the main thread, and must not touch
# matplotlib at all. show() drives its own explicit loop (no matplotlib.animation.FuncAnimation)
# for exactly this reason: FuncAnimation's timer callbacks fire from wherever plt.show()'s own
# blocking event loop happens to run, which is only the process's main thread if plt.show() was
# itself called there - an explicit loop, calling plt.pause() to both redraw and pump GUI events,
# makes that a property of whichever thread calls show(), instead of something hidden inside it.
import matplotlib.pyplot as plt

import opendaq


def _disable_macos_ctrl_c_interrupt_if_not_pythons_main_thread() -> None:
    """Works around a third, separate main-thread assumption inside matplotlib's macOS backend.

    Every time its event loop starts (plt.show(), and again on every plt.pause() - see
    FigureManagerMac.start_event_loop/start_main_loop in backend_macosx.py), it wraps that in
    _allow_interrupt_macos(), which calls signal.set_wakeup_fd() so Ctrl+C can interrupt a
    blocked GUI loop. That call is restricted by CPython to "the main thread of the main
    interpreter" - a notion fixed once, permanently, to whichever thread first called
    Py_Initialize(). In a host that embeds Python (see the module docstring), that is
    PythonRuntime's own dispatch thread, never this method's actual caller - even though show()
    is, by then, correctly running on the process's real OS main thread (which is what the
    isMainThread check inside matplotlib's native _macosx extension actually cares about, and
    is satisfied). Ctrl+C support isn't needed for this example - closing the window is enough -
    so this disables just that one optional integration, and only when it would actually fail:
    threading.main_thread() is Python's own (wrong, in this scenario) notion of the main thread,
    so this leaves the real pure-Python path (where they're the same thread) untouched.
    """
    if threading.current_thread() is threading.main_thread():
        return

    try:
        import matplotlib.backends.backend_macosx as backend_macosx
    except ImportError:
        return

    backend_macosx._allow_interrupt_macos = contextlib.nullcontext


class PlotterFb(opendaq.FunctionBlock):
    """Builds a DurationTailReader over whatever signal is connected to this function block's
    one input port, and - once show() is called - polls it on a timer to draw a live window of
    the signal's trailing history.

    Attributes:
        reader (opendaq.IDurationTailReader | None): Built once in on_init(), directly from the
            input port (opendaq.DurationTailReaderFromPort) rather than per-connection, so it
            stays registered as the port's listener across disconnect/reconnect.
    """

    # How far back the reader retains unread samples, so show()'s polling doesn't need to keep
    # up with every single packet - it just needs to poll more often than this.
    HISTORY_DURATION_MS = 1000

    def __init__(self, context: opendaq.IContext, parent: opendaq.IComponent, local_id: str) -> None:
        super().__init__(context, parent, local_id)
        self.reader: opendaq.IDurationTailReader | None = None
        self.times = np.array([], dtype=np.int64)
        self.values = np.array([], dtype=np.float64)
        # Ticks per millisecond for whatever signal is currently connected - set in
        # on_connected() from that signal's domain resolution, since raw domain ticks aren't
        # necessarily milliseconds (e.g. daqref's are microseconds).
        self.ticks_per_ms: float | None = None
        # Figure/Axes/Line are created lazily in show(), not here - __init__ runs on
        # PythonRuntime's dispatch thread (see the module docstring), and matplotlib's
        # interactive backends require GUI objects to be created on the main thread.
        self.figure = None
        self.axes = None
        self.line = None

    @staticmethod
    def create_function_block_type() -> opendaq.IFunctionBlockType:
        return opendaq.FunctionBlockType(
            "PlotterFb",
            "Plotter",
            "Shows the trailing history of one input signal in a live window",
            None,
        )

    def on_init(self) -> None:
        port = opendaq.InputPort(self.context, self.input_ports_folder, "input", False)
        self.add_input_port(port)
        self.reader = opendaq.DurationTailReaderFromPort(port, self.HISTORY_DURATION_MS)
        self.reader.external_listener = opendaq.IInputPortNotifications.cast_from(self._cpp_fb.ref)
        self.context.scheduler.schedule_work_on_main_loop(opendaq.Work(self.show))

    def on_connected(self, port: opendaq.IInputPort) -> None:
        # DataDescriptor.tick_resolution is a ratio of seconds per tick (seconds = tick *
        # numerator / denominator - see TimeReader in opendaq/reader/time_reader.h, mirrored
        # here without pulling in a full TimeReader since DurationTailReader has no Time*
        # variant). Inverting and scaling by 1000 gives ticks per millisecond.
        resolution = port.signal.domain_signal.descriptor.tick_resolution
        self.ticks_per_ms = resolution.denominator / (resolution.numerator * 1000)

    def show(self, interval_s: float = 0.1) -> None:
        """Draws one frame for this function block's live window (opening it first, if this is
        the first call), then reschedules itself for the next frame.

        Deliberately does *not* loop internally for the window's whole lifetime. show() is
        reached by being scheduled onto context.scheduler's main loop (see on_init()), so
        it's already running *from inside* whichever run_main_loop_iteration() call picked it
        up - looping here would keep that call, and whatever thread made it, blocked for as
        long as the window stays open, with no way for that thread to ever interleave anything
        else between frames (not even its own exit/interrupt checks - e.g. Ctrl+C in
        run_live_example.cpp - since the scheduler explicitly rejects a reentrant
        run_main_loop_iteration() call, so this can't just pump the loop itself either, the
        first thing that was tried here). Rescheduling one frame at a time instead returns
        control to that thread after every single frame, which is what lets it actually revisit
        those checks promptly.

        Must be called from the process's main thread - see the module docstring.
        """
        self_weak = self._cpp_fb
        if self_weak.ref is None:
            return  # backing function block already destroyed - stop rescheduling

        if self.figure is None:
            _disable_macos_ctrl_c_interrupt_if_not_pythons_main_thread()
            self.figure, self.axes = plt.subplots()
            (self.line,) = self.axes.plot([], [])
            self.axes.set_xlabel("Time (ms)")
            self.axes.set_ylabel("Value")
            self.axes.set_title(self.local_id)
            # Static window: always exactly HISTORY_DURATION_MS wide, right edge pinned at "now"
            # (0). Only y autoscales per frame - see _poll_and_redraw()'s autoscale_view call.
            self.axes.set_xlim(-self.HISTORY_DURATION_MS, 0)
            self.axes.set_xticks([-self.HISTORY_DURATION_MS, -self.HISTORY_DURATION_MS / 2, 0])
            plt.show(block=False)

        if not plt.fignum_exists(self.figure.number):
            return  # window closed - nothing more to draw, and nothing to reschedule

        self._poll_and_redraw()
        # Pumps this backend's own GUI event loop and redraws - the same thing FuncAnimation
        # relied on a timer to trigger, done explicitly here instead.
        plt.pause(interval_s)

        self.context.scheduler.schedule_work_on_main_loop(opendaq.Work(self.show))

    def _poll_and_redraw(self) -> None:
        # DurationTailReader is a non-consuming tail reader, not a draining one: every read
        # returns (up to) the full current HISTORY_DURATION_MS window relative to the newest
        # received packet, not just samples new since the last poll - getAvailableCount()
        # doesn't track a read cursor either (see DurationTailReaderImpl::readData()/
        # getAvailableCount()). So each poll replaces the buffer outright rather than
        # accumulating into it - concatenating would pile up heavily overlapping reads.
        reader = self.reader
        if reader is not None:
            count = reader.available_count
            if count > 0:
                self.values, self.times = reader.read_with_domain(count)

        if len(self.times) == 0 or self.ticks_per_ms is None:
            self.line.set_data([], [])
            return

        # Relative to the newest sample, in milliseconds - so the x-axis's static
        # [-HISTORY_DURATION_MS, 0] window (see show()) lines up with the data regardless of
        # how long the function block has actually been running.
        relative_ms = (self.times - self.times[-1]) / self.ticks_per_ms
        self.line.set_data(relative_ms, self.values)

        self.axes.relim()
        self.axes.autoscale_view(scalex=False)  # x stays pinned to the static window


class PlotterModule(opendaq.Module):
    def __init__(self, context: opendaq.IContext) -> None:
        super().__init__(context, name="PlotterModule", version=(1, 0, 0), id="PlotterModuleId")

    def on_get_available_function_block_types(self) -> "dict[str, opendaq.IFunctionBlockType]":
        return {"PlotterFb": PlotterFb.create_function_block_type()}

    def on_create_function_block(
        self,
        id: str,
        parent: opendaq.IComponent,
        local_id: str,
        config: opendaq.IPropertyObject = None,
    ) -> opendaq.FunctionBlock | None:
        if id == "PlotterFb":
            return PlotterFb(self.context, parent, local_id)
        return None


def create_module(context: "opendaq.IContext") -> PlotterModule:
    return PlotterModule(context)

def main():
    instance_builder = opendaq.InstanceBuilder()
    instance_builder.set_root_device("daqref://device0")
    instance_builder.using_scheduler_main_loop = True
    instance = instance_builder.build()

    plotter_module = create_module(instance.context)
    instance.module_manager.load_python_module(plotter_module)

    signal = instance.channels[0].signals[0]
    fb_handle = instance.add_function_block("PlotterFb")
    fb_handle.input_ports[0].connect(signal)

    while True:
        sleep(0.01)
        instance.context.scheduler.run_main_loop_iteration()

if __name__ == "__main__":
    main()
