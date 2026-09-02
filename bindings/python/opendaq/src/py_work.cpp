#include <py_opendaq/py_opendaq.h>
#include <py_core_types/py_procedure.h>

namespace
{

/*!
 * @brief Adapts an IProcedure (built from a Python callable via Procedure()/PyProcedure_Create -
 * see py_procedure.cpp) to IWork, so it can be passed to IWork-taking APIs like
 * IScheduler::scheduleWorkOnMainLoop().
 *
 * `procedure` is a plain openDAQ ProcedurePtr (ordinary atomic refcounting), so *constructing*
 * or *copying* this object needs no GIL - only the underlying Python callable it eventually
 * wraps does, and PyProcedureImpl::dispatch() (py_procedure.h) already acquires the GIL for that
 * itself, regardless of which thread calls execute() here. *Destroying* this object is a
 * different story: PyProcedureImpl ultimately owns a py::object (the callable), and its base
 * class, PyObjectImpl (py_base_object.h), has no GIL guard of its own around destroying it. That
 * is fine for how IProcedure is normally used (a callback held by some long-lived C++ object,
 * released alongside ordinary Python-driven cleanup) but not here: openDAQ's scheduler drops its
 * own reference to a just-executed work item from its own internal main-loop bookkeeping - pure
 * C++, no GIL held at all - which is exactly when the *last* reference to this object (and so
 * to `procedure`) is typically dropped. Without an explicit guard here, that destroys the
 * wrapped py::object with no GIL held, tripping pybind11's "inc_ref()/dec_ref() called without
 * the GIL" assertion.
 */
class WorkFromProcedureImpl final : public daq::ImplementationOf<daq::IWork>
{
public:
    explicit WorkFromProcedureImpl(daq::ProcedurePtr procedure)
        : procedure(std::move(procedure))
    {
    }

    ~WorkFromProcedureImpl() override
    {
        pybind11::gil_scoped_acquire gil;
        procedure = nullptr;
    }

    daq::ErrCode INTERFACE_FUNC execute() override
    {
        return daq::daqTry([this] { procedure.dispatch(); });
    }

private:
    daq::ProcedurePtr procedure;
};

}  // namespace

PyDaqIntf<daq::IWork, daq::IBaseObject> declareIWork(pybind11::module_ m)
{
    return wrapInterface<daq::IWork, daq::IBaseObject>(m, "IWork", py::dynamic_attr());
}


void defineIWork(pybind11::module_ m, PyDaqIntf<daq::IWork, daq::IBaseObject> cls)
{
    cls.doc() = "A lightweight implementation of callback used in scheduler for worker tasks.";

    m.def(
        "Work",
        [](const py::object& callable) -> daq::IWork*
        {
            const daq::ProcedurePtr procedure = PyProcedure_Create<PyConverter>(callable);
            // .detach(): pybind11's registered caster for openDAQ interfaces (wrapInterface,
            // above) only knows how to wrap a raw, already-addRef'd Interface* - not the
            // GenericWorkPtr<IWork> smart pointer createWithImplementation returns - matching
            // every other interface-returning factory bound here (e.g. Instance(), NullContext()
            // in py_opendaq.cpp).
            return daq::createWithImplementation<daq::IWork, WorkFromProcedureImpl>(procedure).detach();
        },
        py::arg("callable"),
        "Wraps a Python callable as an IWork, for use with IScheduler methods that take one "
        "(e.g. schedule_work_on_main_loop).");

}