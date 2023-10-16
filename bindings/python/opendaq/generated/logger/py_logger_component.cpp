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
 * Copyright 2022-2023 Blueberry d.o.o.
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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"

PyDaqIntf<daq::ILoggerComponent, daq::IBaseObject> declareILoggerComponent(pybind11::module_ m)
{
    return wrapInterface<daq::ILoggerComponent, daq::IBaseObject>(m, "ILoggerComponent");
}

void defineILoggerComponent(pybind11::module_ m, PyDaqIntf<daq::ILoggerComponent, daq::IBaseObject> cls)
{
    cls.doc() = "Logs messages produced by a specific part of openDAC SDK. The messages are written into the @ref ILoggerSink \"Logger Sinks\" associated with the Logger Component object.";

    m.def("LoggerComponent", &daq::LoggerComponent_Create);

    cls.def_property_readonly("name",
        [](daq::ILoggerComponent *object)
        {
            const auto objectPtr = daq::LoggerComponentPtr::Borrow(object);
            return objectPtr.getName().toStdString();
        },
        "Gets the name of the component.");
    cls.def_property("level",
        [](daq::ILoggerComponent *object)
        {
            const auto objectPtr = daq::LoggerComponentPtr::Borrow(object);
            return objectPtr.getLevel();
        },
        [](daq::ILoggerComponent *object, daq::LogLevel level)
        {
            const auto objectPtr = daq::LoggerComponentPtr::Borrow(object);
            objectPtr.setLevel(level);
        },
        "Gets the minimal severity level of messages to be logged by the component. / Sets the minimal severity level of messages to be logged by the component.");
    cls.def("log_message",
        [](daq::ILoggerComponent *object, daq::SourceLocation location, daq::ConstCharPtr msg, daq::LogLevel level)
        {
            const auto objectPtr = daq::LoggerComponentPtr::Borrow(object);
            objectPtr.logMessage(location, msg, level);
        },
        py::arg("location"), py::arg("msg"), py::arg("level"),
        "Logs a message with the provided source location and severity level.");
    cls.def_property("pattern",
        nullptr,
        [](daq::ILoggerComponent *object, const std::string& pattern)
        {
            const auto objectPtr = daq::LoggerComponentPtr::Borrow(object);
            objectPtr.setPattern(pattern);
        },
        "Sets the custom formatter pattern for the component.");
    cls.def("should_log",
        [](daq::ILoggerComponent *object, daq::LogLevel level)
        {
            const auto objectPtr = daq::LoggerComponentPtr::Borrow(object);
            return objectPtr.shouldLog(level);
        },
        py::arg("level"),
        "Checks whether the messages with given log severity level will be logged or not.");
    cls.def("flush",
        [](daq::ILoggerComponent *object)
        {
            const auto objectPtr = daq::LoggerComponentPtr::Borrow(object);
            objectPtr.flush();
        },
        "Triggers writing out the messages stored in temporary buffers.");
    cls.def("flush_on_level",
        [](daq::ILoggerComponent *object, daq::LogLevel level)
        {
            const auto objectPtr = daq::LoggerComponentPtr::Borrow(object);
            objectPtr.flushOnLevel(level);
        },
        py::arg("level"),
        "Sets the minimum severity level of messages to be automatically written to the associated sinks bypassing the temporary buffers.");
}
