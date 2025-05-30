/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <pybind11/gil.h>
#include <pybind11/functional.h>

#include <coretypes/objectptr.h>

#include "py_core_types/py_queued_event_handler_impl.h"
#include "coretypes/errors.h"
#include "py_core_types/py_core_types.h"
#include "py_core_types/py_event_queue.h"

daq::ObjectPtr<daq::IPythonQueuedEventHandler> createQueuedEventHandler(pybind11::object eventHandler) 
{
    daq::ObjectPtr<daq::IPythonQueuedEventHandler> eventHandlerPtr;
    const daq::ErrCode err = daq::createObjectForwarding<daq::IPythonQueuedEventHandler, daq::PyQueuedEventHandler>(&eventHandlerPtr, eventHandler);
    daq::checkErrorInfo(err);
    return eventHandlerPtr;
}

PyDaqIntf<daq::IPythonQueuedEventHandler, daq::IEventHandler> declareIPythonQueuedEventHandler(pybind11::module_ m)
{
    return wrapInterface<daq::IPythonQueuedEventHandler, daq::IEventHandler>(m, "IPythonQueuedEventHandler");
}

void defineIPythonQueuedEventHandler(pybind11::module_ m, PyDaqIntf<daq::IPythonQueuedEventHandler, daq::IEventHandler> cls)
{
    cls.doc() = "";

    m.def("QueuedEventHandler", [](pybind11::object eventHandler){
        return createQueuedEventHandler(eventHandler).detach();
    }, py::arg("event_handler"));
}

daq::PyQueuedEventHandler::PyQueuedEventHandler(pybind11::object sub)
    : subscription(std::move(sub))
{}

daq::ErrCode daq::PyQueuedEventHandler::handleEvent(daq::IBaseObject* sender, daq::IEventArgs* eventArgs)
{
    enqueuePythonEvent(this->thisInterface(), sender, eventArgs);
    return OPENDAQ_SUCCESS;
}

daq::ErrCode daq::PyQueuedEventHandler::dispatch(daq::IBaseObject* sender, daq::IEventArgs* eventArgs)
{
    pybind11::gil_scoped_acquire gil;
    try
    {
        auto senderObj = baseObjectToPyObject(sender);
        auto eventArgsObj = baseObjectToPyObject(eventArgs);
        auto args = pybind11::make_tuple(senderObj, eventArgsObj);

        subscription(*args);
    }
    catch (const pybind11::error_already_set& e)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_OPERATION, "Python callback error: %s", e.what());
    }

    return OPENDAQ_SUCCESS;
}