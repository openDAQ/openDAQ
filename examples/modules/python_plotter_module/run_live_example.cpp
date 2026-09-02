/*
 * C++ driver for plotter_module.py's live-plotting PlotterFb, run through a C++ host that embeds
 * Python via PythonRuntime/PythonModule (bindings/python/py_opendaq_module) - an alternative to
 * running `python3 plotter_module.py` directly (see that file's main(), which uses
 * IModuleManager.load_python_module() against the real opendaq package - both paths work; this
 * one is kept as a from-C++ usage example, via createPythonModule(context, path) instead.
 *
 * Mirrors plotter_module.py's own main(): PlotterFb.on_connected() schedules its own show() onto
 * context.scheduler's main loop once a signal is connected, so nothing here needs to reach back
 * into the loaded plugin's Python objects at all - this file just needs to pump that same main
 * loop from its own real thread, same as any other openDAQ host application would.
 *
 * Built and wired up from bindings/python/py_opendaq_module/examples/CMakeLists.txt, since that's
 * where daq::py_opendaq_module is already available - see the comment there.
 */

#include <atomic>
#include <csignal>
#include <thread>

#include <opendaq/opendaq.h>
#include <py_opendaq_module/python_module.h>

using namespace daq;

namespace
{
    std::atomic<bool> g_interrupted{false};

    void handleSigint(int)
    {
        g_interrupted.store(true);
    }
}

int main()
{
    std::signal(SIGINT, handleSigint);

    // setUsingSchedulerMainLoop is required for context.scheduler.run_main_loop_iteration() /
    // scheduleWorkOnMainLoop() (used by PlotterFb.show()/on_connected(), see plotter_module.py)
    // to work at all - a plain Instance() builds a scheduler with no main-loop support, so
    // runMainLoopIteration() would fail with "Main thread worker is not set" (scheduler_impl.cpp).
    const InstancePtr instance = InstanceBuilder().setRootDevice("daqref://device0")
                                                   .setUsingSchedulerMainLoop(true)
                                                   .build();

    const ModulePtr module = createPythonModule(instance.getContext(), PLOTTER_MODULE_PATH);
    instance.getModuleManager().addModule(module);

    const auto signal = instance.getChannels()[0].getSignals()[0];
    const FunctionBlockPtr fb = instance.addFunctionBlock("PlotterFb");
    // Connecting triggers PlotterFb.on_connected() (on PythonRuntime's dispatch thread), which
    // schedules self.show onto context.scheduler's main loop - it won't actually run until this
    // thread starts pumping that loop below.
    fb.getInputPorts()[0].connect(signal);

    // This is what makes it run on *this* thread - the real OS main thread, which is what
    // matplotlib's interactive backend requires for GUI creation on macOS. Whatever gets
    // scheduled onto the main loop (PlotterFb.show(), here) executes synchronously inside
    // whichever runMainLoopIteration() call picks it up.
    while (!g_interrupted.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        instance.getContext().getScheduler().runMainLoopIteration();
    }

    return 0;
}
