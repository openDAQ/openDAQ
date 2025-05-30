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

#include "py_core_types/py_queued_event_handler.h"
#include "py_core_types/py_core_types.h"
#include "py_core_types/py_event_queue.h"

daq::EventHandlerPtr<> createQueuedEventHandler(pybind11::object eventHandler) 
{
    daq::ObjectPtr<daq::IQueuedEventHandler> eventHandlerPtr;
    const daq::ErrCode err = daq::createObjectForwarding<daq::IQueuedEventHandler, daq::PyQueuedEventHandler>(&eventHandlerPtr, eventHandler);
    daq::checkErrorInfo(err);
    return eventHandlerPtr;
}

PyDaqIntf<daq::IQueuedEventHandler, daq::IEventHandler> declareIQueuedEventHandler(pybind11::module_ m)
{
    return wrapInterface<daq::IQueuedEventHandler, daq::IEventHandler>(m, "IQueuedEventHandler");
}

void defineIQueuedEventHandler(pybind11::module_ m, PyDaqIntf<daq::IQueuedEventHandler, daq::IEventHandler> cls)
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
    enqueuePythonEvent(this, sender, eventArgs);
    return OPENDAQ_SUCCESS;
}

void daq::PyQueuedEventHandler::dispatch(const daq::ObjectPtr<daq::IBaseObject>& sender, const daq::ObjectPtr<daq::IEventArgs>& eventArgs)
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
        pybind11::print("Python callback error:", e.what());
    }
}