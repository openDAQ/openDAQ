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

#include "coretypes/event_ptr.h"
#include "py_core_types/py_core_types.h"
#include "py_core_types/py_converter.h"


PyDaqIntf<daq::IEvent, daq::IBaseObject> declareIEvent(pybind11::module_ m)
{
    return wrapInterface<daq::IEvent, daq::IBaseObject>(m, "IEvent");
}

void defineIEvent(pybind11::module_ m, PyDaqIntf<daq::IEvent, daq::IBaseObject> cls)
{
    cls.doc() = "";

    m.def("Event", &daq::Event_Create);

    cls.def("add_handler",
        [](daq::IEvent *object, daq::IEventHandler* eventHandler)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPtr<>::Borrow(object);
            objectPtr.addHandler(eventHandler);
        },
        py::arg("event_handler"),
        "");
    cls.def("__add__",
        [](daq::IEvent *object, daq::IEventHandler* eventHandler)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPtr<>::Borrow(object);
            objectPtr.addHandler(eventHandler);
        }, py::is_operator(), py::arg("event_handler"));
    cls.def("remove_handler",
        [](daq::IEvent *object, daq::IEventHandler* eventHandler)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPtr<>::Borrow(object);
            objectPtr.removeHandler(eventHandler);
        },
        py::arg("event_handler"),
        "");
    cls.def("__sub__",
        [](daq::IEvent *object, daq::IEventHandler* eventHandler)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPtr<>::Borrow(object);
            objectPtr.removeHandler(eventHandler);
        }, py::is_operator(), py::arg("event_handler"));
    cls.def("clear",
        [](daq::IEvent *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPtr<>::Borrow(object);
            objectPtr.clear();
        },
        "");
    cls.def_property_readonly("subscriber_count",
        [](daq::IEvent *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPtr<>::Borrow(object);
            return objectPtr.getListenerCount();
        },
        "");
    cls.def_property_readonly("subscribers",
        [](daq::IEvent *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPtr<>::Borrow(object);
            return objectPtr.getListeners().detach();
        },
        py::return_value_policy::take_ownership,
        "");
    cls.def("mute",
        [](daq::IEvent *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPtr<>::Borrow(object);
            objectPtr.mute();
        },
        "");
    cls.def("unmute",
        [](daq::IEvent *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPtr<>::Borrow(object);
            objectPtr.unmute();
        },
        "");
    cls.def("mute_listener",
        [](daq::IEvent *object, daq::IEventHandler* eventHandler)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPtr<>::Borrow(object);
            objectPtr.muteListener(eventHandler);
        },
        py::arg("event_handler"),
        "");
    cls.def("__or__",
        [](daq::IEvent *object, daq::IEventHandler* eventHandler)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPtr<>::Borrow(object);
            objectPtr.muteListener(eventHandler);
        }, py::is_operator(), py::arg("event_handler"));
    cls.def("unmute_listener",
        [](daq::IEvent *object, daq::IEventHandler* eventHandler)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPtr<>::Borrow(object);
            objectPtr.unmuteListener(eventHandler);
        },
        py::arg("event_handler"),
        "");
    cls.def("__and__",
        [](daq::IEvent *object, daq::IEventHandler* eventHandler)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::EventPtr<>::Borrow(object);
            objectPtr.unmuteListener(eventHandler);
        }, py::is_operator(), py::arg("event_handler"));
}
