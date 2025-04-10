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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"
#include "py_core_objects/py_variant_extractor.h"

PyDaqIntf<daq::IConfigProvider, daq::IBaseObject> declareIConfigProvider(pybind11::module_ m)
{
    return wrapInterface<daq::IConfigProvider, daq::IBaseObject>(m, "IConfigProvider");
}

void defineIConfigProvider(pybind11::module_ m, PyDaqIntf<daq::IConfigProvider, daq::IBaseObject> cls)
{
    cls.doc() = "Config provider is an interface that was made for populating an options dictionary of an instance builder from external sources like a config file, environment variables, or command line arguments. The process of population of the dictionary have to be alligned with rules: - all keys are set in lowercase. Values are set without case changes. - if a provider is trying to override an existing value, it has to have the same type. For example provider can not replace integer value with string or object with list - if a provider is overriding a list, it replaces old list items with a new one.";

    m.def("JsonConfigProvider", [](std::variant<daq::IString*, py::str, daq::IEvalValue*>& filename){
        return daq::JsonConfigProvider_Create(getVariantValue<daq::IString*>(filename));
    }, py::arg("filename"));

    m.def("EnvConfigProvider", &daq::EnvConfigProvider_Create);
    m.def("CmdLineArgsConfigProvider", [](std::variant<daq::IList*, py::list, daq::IEvalValue*>& cmdLineArgs){
        return daq::CmdLineArgsConfigProvider_Create(getVariantValue<daq::IList*>(cmdLineArgs));
    }, py::arg("cmd_line_args"));


    cls.def("populate_options",
        [](daq::IConfigProvider *object, std::variant<daq::IDict*, py::dict>& options)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::ConfigProviderPtr::Borrow(object);
            objectPtr.populateOptions(getVariantValue<daq::IDict*>(options));
        },
        py::arg("options"),
        "Populate the existing options dictionary with variables from config provider");
}
