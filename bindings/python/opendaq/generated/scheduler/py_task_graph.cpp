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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"
#include "py_core_objects/py_variant_extractor.h"

PyDaqIntf<daq::ITaskGraph, daq::ITask> declareITaskGraph(pybind11::module_ m)
{
    return wrapInterface<daq::ITaskGraph, daq::ITask>(m, "ITaskGraph");
}

void defineITaskGraph(pybind11::module_ m, PyDaqIntf<daq::ITaskGraph, daq::ITask> cls)
{
    cls.doc() = "A dependency graph (directed acyclic graph) of tasks that can be scheduled for execution on a Scheduler.";

    m.def("TaskGraph", [](daq::IProcedure* work, std::variant<daq::IString*, py::str, daq::IEvalValue*>& name){
        return daq::TaskGraph_Create(work, getVariantValue<daq::IString*>(name));
    }, py::arg("work"), py::arg("name"));

}
