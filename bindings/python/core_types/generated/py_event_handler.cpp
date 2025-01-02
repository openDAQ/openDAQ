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

#include "py_core_types/py_event_handler_impl.h"
#include "py_core_types/py_core_types.h"
#include "py_core_types/py_converter.h"


daq::EventHandlerPtr<> createEventHandler(const EventHandler& eventHandler) {
    daq::EventHandlerPtr<> handler;
    daq::ErrCode err = daq::createObjectForwarding<daq::IEventHandler, daq::PyEventHandlerImpl>(&handler, eventHandler);
    daq::checkErrorInfo(err);
    return handler;
}

PyDaqIntf<daq::IEventHandler, daq::IBaseObject> declareIEventHandler(pybind11::module_ m)
{
    return wrapInterface<daq::IEventHandler, daq::IBaseObject>(m, "IEventHandler");
}

void defineIEventHandler(pybind11::module_ m, PyDaqIntf<daq::IEventHandler, daq::IBaseObject> cls)
{
    cls.doc() = "";

    m.def("EventHandler", [](const EventHandler& eventHandler){
        return createEventHandler(eventHandler).detach();
    }, py::arg("event_handler"));
}
