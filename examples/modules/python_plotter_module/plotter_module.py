"""Example openDAQ Python plugin: a "Plotter" function block.

Demonstrates the Module/FunctionBlock plugin contract (see
bindings/python/package/opendaq/{module,function_block}.py) combined with DurationTailReader
(core/opendaq/reader/include/opendaq/duration_tail_reader.h): PlotterFb has a single input port,
and every time a packet arrives on it, reads whatever new samples are available and redraws a
plot of the signal's trailing history.

Not runnable as `python plotter_module.py` - like every openDAQ Python plugin, this file only
defines create_module(context); loading it is the caller's job. There is currently no way to do
that from pure Python: a real opendaq.Instance() has no directory-scan or API for .py plugin
files (InstanceBuilder.add_module_path() only discovers compiled .so/.dylib/.dll modules). A
plugin like this one has to be loaded by a C++ host that embeds Python and adds it explicitly,
the same way bindings/python/py_opendaq_module/tests/test_python_module.cpp does for tests:

    auto module = daq::createWithImplementation<daq::IModule, daq::PythonModule>(
        context, "plotter_module.py");
    context.getModuleManager().asPtr<daq::IModuleManager>().addModule(module);

Requires matplotlib and numpy (not openDAQ dependencies - install separately: `pip install
matplotlib numpy`).
"""

import numpy as np

# Agg is a pure off-screen renderer with no GUI event loop, so it's safe to draw with from any
# thread - unlike interactive backends (TkAgg, MacOSX, Qt...), which generally require GUI calls
# to happen on the process's main thread. on_packet_received() below runs on PythonRuntime's own
# dedicated dispatch thread (see python_runtime.h), which is never the main thread of whatever
# C++ host embeds this plugin, so an interactive backend would be unsafe here.
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

import opendaq


class PlotterFb(opendaq.FunctionBlock):
    """Plots the trailing history of whatever signal is connected to this function block's one
    input port, as a PNG file overwritten on every update.

    Attributes:
        reader (opendaq.IDurationTailReader | None): Built in on_connected() once a signal is
            attached, torn down in on_disconnected(). ``None`` in between.
    """

    # How far back the reader is asked to retain samples.
    HISTORY_DURATION_MS = 5000

    # DurationTailReader only *retains* unread samples within HISTORY_DURATION_MS - it doesn't
    # replay them. read()/read_with_domain() still drain it incrementally like any other reader,
    # so a given sample is only ever returned once. To keep showing a rolling window on the
    # plot, this function block accumulates what it reads into its own buffer, capped here
    # rather than trimmed by domain time (real timestamps would need converting the signal's
    # domain resolution/epoch, more than this example needs).
    MAX_PLOTTED_SAMPLES = 2000

    def __init__(self, context: opendaq.IContext, parent: opendaq.IComponent, local_id: str) -> None:
        super().__init__(context, parent, local_id)
        self.reader: opendaq.IDurationTailReader = None
        self.output_path = f"{local_id}.png"
        self.times = np.array([], dtype=np.int64)
        self.values = np.array([], dtype=np.float64)
        self.figure, self.axes = plt.subplots()

    @staticmethod
    def create_function_block_type() -> opendaq.IFunctionBlockType:
        return opendaq.FunctionBlockType(
            "PlotterFb",
            "Plotter",
            "Plots the trailing history of one input signal to a PNG file",
            None,
        )

    def on_init(self) -> None:
        self.add_input_port(opendaq.InputPort(self.context, self.input_ports_folder, "input", False))

    def on_connected(self, port: opendaq.IInputPort) -> None:
        # IInputPort exposes the signal now connected to it through `.signal` - DurationTailReader
        # is built from that signal, not from the port itself.
        self.reader = opendaq.DurationTailReader(port.signal, self.HISTORY_DURATION_MS)
        self.times = np.array([], dtype=np.int64)
        self.values = np.array([], dtype=np.float64)

    def on_disconnected(self, port: opendaq.IInputPort) -> None:
        self.reader = None

    def on_packet_received(self, port: opendaq.IInputPort) -> None:
        if self.reader is None:
            return

        count = self.reader.available_count
        if count == 0:
            return

        values, domain = self.reader.read_with_domain(count)
        if len(values) == 0:
            return

        self.times = np.concatenate([self.times, domain])[-self.MAX_PLOTTED_SAMPLES :]
        self.values = np.concatenate([self.values, values])[-self.MAX_PLOTTED_SAMPLES :]
        self._redraw()

    def _redraw(self) -> None:
        self.axes.clear()
        self.axes.plot(self.times, self.values)
        self.axes.set_xlabel("Domain (raw ticks)")
        self.axes.set_ylabel("Value")
        self.axes.set_title(self.local_id)
        self.figure.savefig(self.output_path)


class PlotterModule(opendaq.Module):
    def __init__(self, context: opendaq.IContext) -> None:
        super().__init__(context, name="PlotterModule", version=(1, 0, 0), id="PlotterModuleId")

    def on_get_available_function_block_types(self) -> "dict[str, opendaq.IFunctionBlockType]":
        return {"plotter_fb": PlotterFb.create_function_block_type()}

    def on_create_function_block(
        self,
        id: str,
        parent: opendaq.IComponent,
        local_id: str,
        config: opendaq.IPropertyObject = None,
    ) -> opendaq.FunctionBlock | None:
        if id == "plotter_fb":
            return PlotterFb(self.context, parent, local_id)
        return None


def create_module(context: "opendaq.IContext") -> PlotterModule:
    return PlotterModule(context)
