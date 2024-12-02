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

PyDaqIntf<daq::IFunctionBlock, daq::IFolder> declareIFunctionBlock(pybind11::module_ m)
{
    return wrapInterface<daq::IFunctionBlock, daq::IFolder>(m, "IFunctionBlock");
}

void defineIFunctionBlock(pybind11::module_ m, PyDaqIntf<daq::IFunctionBlock, daq::IFolder> cls)
{
    cls.doc() = "Function blocks perform calculations on inputs/generate data, outputting new data in its output signals as packets.";

    cls.def_property_readonly("function_block_type",
        [](daq::IFunctionBlock *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::FunctionBlockPtr::Borrow(object);
            return objectPtr.getFunctionBlockType().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets an information structure contain metadata of the function block type.");
    cls.def_property_readonly("input_ports",
        [](daq::IFunctionBlock *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::FunctionBlockPtr::Borrow(object);
            return objectPtr.getInputPorts().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a list of the function block's input ports.");
    cls.def("get_input_ports",
        [](daq::IFunctionBlock *object, daq::ISearchFilter* searchFilter)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::FunctionBlockPtr::Borrow(object);
            return objectPtr.getInputPorts(searchFilter).detach();
        },
        py::arg("search_filter") = nullptr,
        "Gets a list of the function block's input ports.");
    cls.def_property_readonly("signals",
        [](daq::IFunctionBlock *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::FunctionBlockPtr::Borrow(object);
            return objectPtr.getSignals().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the list of the function block's output signals.");
    cls.def("get_signals",
        [](daq::IFunctionBlock *object, daq::ISearchFilter* searchFilter)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::FunctionBlockPtr::Borrow(object);
            return objectPtr.getSignals(searchFilter).detach();
        },
        py::arg("search_filter") = nullptr,
        "Gets the list of the function block's output signals.");
    cls.def_property_readonly("signals_recursive",
        [](daq::IFunctionBlock *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::FunctionBlockPtr::Borrow(object);
            return objectPtr.getSignalsRecursive().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the list of the function block's visible output signals including signals from visible child function blocks.");
    cls.def("get_signals_recursive",
        [](daq::IFunctionBlock *object, daq::ISearchFilter* searchFilter)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::FunctionBlockPtr::Borrow(object);
            return objectPtr.getSignalsRecursive(searchFilter).detach();
        },
        py::arg("search_filter") = nullptr,
        "Gets the list of the function block's visible output signals including signals from visible child function blocks.");
    cls.def_property_readonly("status_signal",
        [](daq::IFunctionBlock *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::FunctionBlockPtr::Borrow(object);
            return objectPtr.getStatusSignal().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the function block's status signal.");
    cls.def_property_readonly("function_blocks",
        [](daq::IFunctionBlock *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::FunctionBlockPtr::Borrow(object);
            return objectPtr.getFunctionBlocks().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets a list of sub-function blocks.");
    cls.def("get_function_blocks",
        [](daq::IFunctionBlock *object, daq::ISearchFilter* searchFilter)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::FunctionBlockPtr::Borrow(object);
            return objectPtr.getFunctionBlocks(searchFilter).detach();
        },
        py::arg("search_filter") = nullptr,
        "Gets a list of sub-function blocks.");
    cls.def_property_readonly("available_function_block_types",
        [](daq::IFunctionBlock *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::FunctionBlockPtr::Borrow(object);
            return objectPtr.getAvailableFunctionBlockTypes().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets all neasted function block types that are supported, containing their description.");
    cls.def("add_function_block",
        [](daq::IFunctionBlock *object, std::variant<daq::IString*, py::str, daq::IEvalValue*>& typeId, daq::IPropertyObject* config)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::FunctionBlockPtr::Borrow(object);
            return objectPtr.addFunctionBlock(getVariantValue<daq::IString*>(typeId), config).detach();
        },
        py::arg("type_id"), py::arg("config") = nullptr,
        "Creates and adds a function block as the neasted of current function block with the provided unique ID and returns it.");
    cls.def("remove_function_block",
        [](daq::IFunctionBlock *object, daq::IFunctionBlock* functionBlock)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::FunctionBlockPtr::Borrow(object);
            objectPtr.removeFunctionBlock(functionBlock);
        },
        py::arg("function_block"),
        "Removes the function block provided as argument, disconnecting its signals and input ports.");
}
