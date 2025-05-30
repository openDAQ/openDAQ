#include "py_context/py_context_impl.h"
#include "py_core_types/py_core_types.h"
#include "py_core_objects/py_variant_extractor.h"

BEGIN_NAMESPACE_OPENDAQ

ErrCode PythonContextImpl::setEventToQueue(IPythonQueuedEventHandler* eventHandler, IBaseObject* sender, IEventArgs* eventArgs)
{
    if (eventHandler == nullptr)
        return OPENDAQ_IGNORED;

    daq::ObjectPtr<daq::IPythonQueuedEventHandler> eventHandlerPtr = eventHandler;

    std::lock_guard<std::mutex> lock(callbackQueueMutex);
    callbackQueue.push([eventHandlerPtr = std::move(eventHandlerPtr)
                        , sender = std::move(sender)
                        , eventArgs = std::move(eventArgs)] 
    {
        return eventHandlerPtr->dispatch(sender, eventArgs);
    });

    return OPENDAQ_SUCCESS;
}

ErrCode PythonContextImpl::processEventsFromQueue()
{
    std::queue<callbackT> localQueue;
    {
        std::lock_guard<std::mutex> lock(callbackQueueMutex);
        std::swap(localQueue, callbackQueue);
    }

    while (!localQueue.empty())
    {
        auto cb = std::move(localQueue.front());
        localQueue.pop();
        ErrCode errCode = cb();
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    PythonContext,
    IScheduler*, Scheduler,
    ILogger*, Logger,
    ITypeManager*, typeManager,
    IModuleManager*, moduleManager,
    IAuthenticationProvider*, authenticationProvider,
    IDict*, options,
    IDict*, discoveryServices
)

END_NAMESPACE_OPENDAQ

// PyDaqIntf<daq::IPythonContext, daq::IContext> declareIPythonContext(pybind11::module_ m)
// {
//     return wrapInterface<daq::IPythonContext, daq::IContext>(m, "IPythonContext");
// }

// void defineIPythonContext(pybind11::module_ m, PyDaqIntf<daq::IPythonContext, daq::IContext> cls)
// {
//     cls.doc() = "The Context serves as a container for the Scheduler and Logger. It originates at the instance, and is passed to the root device, which forwards it to components such as function blocks and signals.";


//     m.def("Context", [](daq::IScheduler* Scheduler, daq::ILogger* Logger, daq::ITypeManager* typeManager, daq::IModuleManager* moduleManager, daq::IAuthenticationProvider* authenticationProvider, std::variant<daq::IDict*, py::dict>& options, std::variant<daq::IDict*, py::dict>& discoveryServers){
//         return daq::Context_Create(Scheduler, Logger, typeManager, moduleManager, authenticationProvider, getVariantValue<daq::IDict*>(options), getVariantValue<daq::IDict*>(discoveryServers));
//     }, py::arg("scheduler"), py::arg("logger"), py::arg("type_manager"), py::arg("module_manager"), py::arg("authentication_provider"), py::arg("options"), py::arg("discovery_servers"));

// }
