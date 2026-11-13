#include "py_core_types/py_gil.h"

#include <pybind11/pybind11.h>

thread_local int PyFinalization::aliveGuards = 0;

// Never destroyed, so worker threads can still use it during static destruction
PyFinalization& pyFinalization()
{
    static auto* finalization = new PyFinalization();
    return *finalization;
}

void PyFinalization::registerAtExit()
{
    // Called once finalization completes. If it can't be registered, objects released in module teardown leak.
    if (Py_AtExit([] { pyFinalization().finalized = true; }) != 0)
        finalized = true;

    // Atexit callbacks run before Python starts finalizing
    pybind11::module_::import("atexit").attr("register")(pybind11::cpp_function([this]
    {
        finalizingThread = std::this_thread::get_id();
        exiting = true;
        pybind11::gil_scoped_release release;
        while (guardCount > 0)
            std::this_thread::yield();
    }));
}

bool PyFinalization::enter()
{
    // Count before checking, so either the atexit hook waits for this guard or this sees exiting
    ++guardCount;
    // An enclosing guard holds finalization back, e.g. a callback that releases Python objects. The finalizing
    // thread still runs Python code until finalization completes, e.g. module teardown.
    return aliveGuards > 0 || !exiting || (std::this_thread::get_id() == finalizingThread && !finalized);
}

void PyFinalization::leave()
{
    --guardCount;
}

PyFinalization::Guard::Guard()
    : pyAlive(pyFinalization().enter())
{
    aliveGuards += pyAlive;
}

PyFinalization::Guard::~Guard()
{
    aliveGuards -= pyAlive;
    pyFinalization().leave();
}

PyFinalization::Guard::operator bool() const
{
    return pyAlive;
}

PyGilAcquireUnlessExiting::PyGilAcquireUnlessExiting()
{
    if (guard)
        gil.emplace();
}

PyGilAcquireUnlessExiting::operator bool() const
{
    return gil.has_value();
}
