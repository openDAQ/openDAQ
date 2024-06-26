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
 * Copyright 2022-2024 Blueberry d.o.o.
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

PyDaqIntf<daq::IBlockReaderBuilder, daq::IBaseObject> declareIBlockReaderBuilder(pybind11::module_ m)
{
    return wrapInterface<daq::IBlockReaderBuilder, daq::IBaseObject>(m, "IBlockReaderBuilder");
}

void defineIBlockReaderBuilder(pybind11::module_ m, PyDaqIntf<daq::IBlockReaderBuilder, daq::IBaseObject> cls)
{
    cls.doc() = "Builder component of Block reader objects. Contains setter methods to configure the Block reader parameters and a `build` method that builds the Unit object.";

    m.def("BlockReaderBuilder", &daq::BlockReaderBuilder_Create);

    cls.def("build",
        [](daq::IBlockReaderBuilder *object)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            return objectPtr.build().detach();
        },
        "Builds and returns a Block reader object using the currently set values of the Builder.");
    cls.def_property("old_block_reader",
        [](daq::IBlockReaderBuilder *object)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            return objectPtr.getOldBlockReader().detach();
        },
        [](daq::IBlockReaderBuilder *object, daq::IBlockReader* blockReader)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            objectPtr.setOldBlockReader(blockReader);
        },
        py::return_value_policy::take_ownership,
        "Gets the old Block reader instance to copy from / Sets old block reader instance to copy from");
    cls.def_property("signal",
        [](daq::IBlockReaderBuilder *object)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            return objectPtr.getSignal().detach();
        },
        [](daq::IBlockReaderBuilder *object, daq::ISignal* signal)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            objectPtr.setSignal(signal);
        },
        py::return_value_policy::take_ownership,
        "Gets the signal / Sets the signal to block reader");
    cls.def_property("input_port",
        [](daq::IBlockReaderBuilder *object)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            return objectPtr.getInputPort().detach();
        },
        [](daq::IBlockReaderBuilder *object, daq::IInputPort* port)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            objectPtr.setInputPort(port);
        },
        py::return_value_policy::take_ownership,
        "Gets the input port / Sets the input port to block reader");
    cls.def_property("value_read_type",
        [](daq::IBlockReaderBuilder *object)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            return objectPtr.getValueReadType();
        },
        [](daq::IBlockReaderBuilder *object, daq::SampleType type)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            objectPtr.setValueReadType(type);
        },
        "Gets the value signal read type / Sets the value signal read type");
    cls.def_property("domain_read_type",
        [](daq::IBlockReaderBuilder *object)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            return objectPtr.getDomainReadType();
        },
        [](daq::IBlockReaderBuilder *object, daq::SampleType type)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            objectPtr.setDomainReadType(type);
        },
        "Gets the domain signal read type / Sets the domain signal read type");
    cls.def_property("read_mode",
        [](daq::IBlockReaderBuilder *object)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            return objectPtr.getReadMode();
        },
        [](daq::IBlockReaderBuilder *object, daq::ReadMode mode)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            objectPtr.setReadMode(mode);
        },
        "Gets the read mode (Unscaled, Scaled, RawValue) / Sets the read mode (Unscaled, Scaled, RawValue)");
    cls.def_property("block_size",
        [](daq::IBlockReaderBuilder *object)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            return objectPtr.getBlockSize();
        },
        [](daq::IBlockReaderBuilder *object, const size_t size)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            objectPtr.setBlockSize(size);
        },
        "Gets the block size / Sets the block size");
    cls.def_property("overlap",
        [](daq::IBlockReaderBuilder *object)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            return objectPtr.getOverlap();
        },
        [](daq::IBlockReaderBuilder *object, const size_t overlap)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            objectPtr.setOverlap(overlap);
        },
        "Gets the overlap / Sets the overlap");
    cls.def_property("skip_events",
        [](daq::IBlockReaderBuilder *object)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            return objectPtr.getSkipEvents();
        },
        [](daq::IBlockReaderBuilder *object, const bool skipEvents)
        {
            const auto objectPtr = daq::BlockReaderBuilderPtr::Borrow(object);
            objectPtr.setSkipEvents(skipEvents);
        },
        "Gets the skip events / Sets the skip events");
}
