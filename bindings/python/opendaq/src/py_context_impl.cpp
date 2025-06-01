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

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY,
    PythonContext,
    IContext,
    createContext,
    IScheduler*, Scheduler,
    ILogger*, Logger,
    ITypeManager*, typeManager,
    IModuleManager*, moduleManager,
    IAuthenticationProvider*, authenticationProvider,
    IDict*, options,
    IDict*, discoveryServices
)

END_NAMESPACE_OPENDAQ

PyDaqIntf<daq::IPythonContext, daq::IContext> declareIPythonContext(pybind11::module_ m)
{
    return wrapInterface<daq::IPythonContext, daq::IContext>(m, "IPythonContext");
}

void defineIPythonContext(pybind11::module_ m, PyDaqIntf<daq::IPythonContext, daq::IContext> cls)
{
    cls.def("setEventToQueue", [](daq::IPythonContext* context, daq::IPythonQueuedEventHandler* eventHandler, daq::IBaseObject* sender, daq::IEventArgs* eventArgs)
    {
        py::gil_scoped_release release;
        daq::checkErrorInfo(context->setEventToQueue(eventHandler, sender, eventArgs));
    });

    cls.def("processEventsFromQueue", [](daq::IPythonContext* context)
    {
        py::gil_scoped_release release;
        daq::checkErrorInfo(context->processEventsFromQueue());
    });
}
