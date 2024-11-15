//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (PythonGenerator).
// </auto-generated>
//------------------------------------------------------------------------------

/*
 * Copyright 2022-2024 openDAQ d.o.o.
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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"
#include "py_core_objects/py_variant_extractor.h"

PyDaqIntf<daq::IConnectionStatusContainerPrivate, daq::IBaseObject> declareIConnectionStatusContainerPrivate(pybind11::module_ m)
{
    return wrapInterface<daq::IConnectionStatusContainerPrivate, daq::IBaseObject>(m, "IConnectionStatusContainerPrivate");
}

void defineIConnectionStatusContainerPrivate(pybind11::module_ m, PyDaqIntf<daq::IConnectionStatusContainerPrivate, daq::IBaseObject> cls)
{
    cls.doc() = "Provides access to private methods for managing the Device's connection statuses.";

    cls.def("add_configuration_connection_status",
        [](daq::IConnectionStatusContainerPrivate *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& connectionString, daq::IEnumeration* initialValue)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ConnectionStatusContainerPrivatePtr::Borrow(object);
            objectPtr.addConfigurationConnectionStatus(getVariantValue<daq::IString*>(connectionString), initialValue);
        },
        py::arg("connection_string"), py::arg("initial_value"),
        "Adds a new configuration connection status with the specified connection string and initial value.");
    cls.def("add_streaming_connection_status",
        [](daq::IConnectionStatusContainerPrivate *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& connectionString, daq::IEnumeration* initialValue, const py::object& streamingObject)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ConnectionStatusContainerPrivatePtr::Borrow(object);
            objectPtr.addStreamingConnectionStatus(getVariantValue<daq::IString*>(connectionString), initialValue, pyObjectToBaseObject(streamingObject));
        },
        py::arg("connection_string"), py::arg("initial_value"), py::arg("streaming_object"),
        "Adds a new streaming connection status with the specified connection string, initial value, and streaming object.");
    cls.def("remove_streaming_connection_status",
        [](daq::IConnectionStatusContainerPrivate *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& connectionString)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ConnectionStatusContainerPrivatePtr::Borrow(object);
            objectPtr.removeStreamingConnectionStatus(getVariantValue<daq::IString*>(connectionString));
        },
        py::arg("connection_string"),
        "Removes a streaming connection status associated with the specified connection string.");
    cls.def("update_connection_status",
        [](daq::IConnectionStatusContainerPrivate *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& connectionString, daq::IEnumeration* value, const py::object& streamingObject)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ConnectionStatusContainerPrivatePtr::Borrow(object);
            objectPtr.updateConnectionStatus(getVariantValue<daq::IString*>(connectionString), value, pyObjectToBaseObject(streamingObject));
        },
        py::arg("connection_string"), py::arg("value"), py::arg("streaming_object"),
        "Updates the value of an existing connection status.");
}