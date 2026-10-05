/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

// Hand-written bindings for MultiReader2: the binding generator cannot express the jagged read buffers, so read
// allocates and returns one array per input instead.

#include <pybind11/gil.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"
#include "py_core_objects/py_variant_extractor.h"

#include <opendaq/input_port_ptr.h>
#include <opendaq/multi_reader2_factory.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/signal_ptr.h>

namespace
{
    constexpr const char* PARAMS_ATTRIBUTE = "__params";

    daq::MultiReader2ParamsPtr paramsOf(py::handle self)
    {
        if (!py::hasattr(self, PARAMS_ATTRIBUTE))
            throw std::runtime_error("The reader has no parameters attached");
        daq::IMultiReader2Params* params = py::cast<daq::IMultiReader2Params*>(self.attr(PARAMS_ATTRIBUTE));
        return daq::MultiReader2ParamsPtr(params);
    }

    py::dtype dtypeOf(daq::SampleType type)
    {
        switch (type)
        {
            case daq::SampleType::Float32: return py::dtype::of<float>();
            case daq::SampleType::Float64: return py::dtype::of<double>();
            case daq::SampleType::UInt8: return py::dtype::of<uint8_t>();
            case daq::SampleType::Int8: return py::dtype::of<int8_t>();
            case daq::SampleType::UInt16: return py::dtype::of<uint16_t>();
            case daq::SampleType::Int16: return py::dtype::of<int16_t>();
            case daq::SampleType::UInt32: return py::dtype::of<uint32_t>();
            case daq::SampleType::Int32: return py::dtype::of<int32_t>();
            case daq::SampleType::UInt64: return py::dtype::of<uint64_t>();
            case daq::SampleType::Int64: return py::dtype::of<int64_t>();
            default: throw std::runtime_error("The Python binding reads numeric value read types only");
        }
    }

    // Elements per sample from the input's current descriptor; 1 for scalars and for inputs without a signal
    size_t elementsPerSample(const daq::ComponentPtr& input)
    {
        daq::SignalPtr signal = input.asPtrOrNull<daq::ISignal>(true);
        if (!signal.assigned())
        {
            const auto port = input.asPtrOrNull<daq::IInputPort>(true);
            if (port.assigned())
                signal = port.getSignal();
        }
        if (!signal.assigned() || !signal.getDescriptor().assigned())
            return 1;
        const auto dimensions = signal.getDescriptor().getDimensions();
        size_t elements = 1;
        if (dimensions.assigned())
            for (const auto& dimension : dimensions)
                elements *= dimension.getSize();
        return std::max<size_t>(elements, 1);
    }

    struct Buffers
    {
        std::vector<py::array> arrays;
        std::vector<void*> pointers;
        std::vector<size_t> elements;
    };

    Buffers allocate(const daq::MultiReader2ParamsPtr& params, size_t count, bool withDomain)
    {
        Buffers buffers;
        const auto dtype = dtypeOf(params.getValueReadType());
        if (withDomain)
        {
            buffers.arrays.push_back(py::array(py::dtype::of<int64_t>(), py::array::ShapeContainer{static_cast<py::ssize_t>(count)}));
            buffers.pointers.push_back(buffers.arrays.back().mutable_data());
            buffers.elements.push_back(1);
        }
        for (const auto& input : params.getInputs())
        {
            const size_t elements = elementsPerSample(input);
            py::array array = elements > 1 ? py::array(dtype, py::array::ShapeContainer{static_cast<py::ssize_t>(count), static_cast<py::ssize_t>(elements)})
                                           : py::array(dtype, py::array::ShapeContainer{static_cast<py::ssize_t>(count)});
            buffers.pointers.push_back(count > 0 ? array.mutable_data() : nullptr);
            buffers.arrays.push_back(std::move(array));
            buffers.elements.push_back(elements);
        }
        return buffers;
    }

    py::list truncate(Buffers& buffers, size_t from, size_t count)
    {
        py::list result;
        for (size_t i = from; i < buffers.arrays.size(); i++)
            result.append(buffers.arrays[i][py::slice(0, count, 1)]);
        return result;
    }
}

PyDaqIntf<daq::IMultiReader2Params, daq::IBaseObject> declareIMultiReader2Params(pybind11::module_ m)
{
    py::enum_<daq::MultiReader2ErrorPolicy>(m, "MultiReader2ErrorPolicy")
        .value("Invalidate", daq::MultiReader2ErrorPolicy::Invalidate)
        .value("Exclude", daq::MultiReader2ErrorPolicy::Exclude);

    return wrapInterface<daq::IMultiReader2Params, daq::IBaseObject>(m, "IMultiReader2Params");
}

void defineIMultiReader2Params(pybind11::module_ m, PyDaqIntf<daq::IMultiReader2Params, daq::IBaseObject> cls)
{
    cls.doc() = "Parameters applied to a multi reader via configure; read at configure time and not retained.";

    m.def("MultiReader2Params", []() { return daq::MultiReader2Params_Create(); },
          "Creates a params object with the defaults: every input used, automatic main input, Float64 read type, MinReadCount 1, Invalidate error policy.");

    cls.def_property("inputs",
        [](daq::IMultiReader2Params* object)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            return objectPtr.getInputs().detach();
        },
        [](daq::IMultiReader2Params* object, std::variant<daq::IList*, py::list>& inputs)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            objectPtr.setInputs(getVariantValue<daq::IList*>(inputs));
        },
        py::return_value_policy::take_ownership,
        "The inputs in slot order; non-empty, no duplicates, all signals or all input ports. Used flags of inputs that stay in the list are kept.");
    cls.def_property("main_input",
        [](daq::IMultiReader2Params* object)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            return objectPtr.getMainInput().detach();
        },
        [](daq::IMultiReader2Params* object, daq::IComponent* input)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            objectPtr.setMainInput(input);
        },
        py::return_value_policy::take_ownership,
        "What everything else aligns against. None: the reader chooses the first used healthy input. Set: pinned, must be in the list and used, its error invalidates the reader.");
    cls.def("get_input_used",
        [](daq::IMultiReader2Params* object, daq::IComponent* input)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            return static_cast<bool>(objectPtr.getInputUsed(input));
        },
        py::arg("input"),
        "Whether an input takes part in reading; true by default.");
    cls.def("set_input_used",
        [](daq::IMultiReader2Params* object, daq::IComponent* input, bool used)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            objectPtr.setInputUsed(input, used);
        },
        py::arg("input"), py::arg("used"),
        "Sets whether an input takes part in reading. An unused input keeps its slot, delivers no data, cannot invalidate the reader, and its state is reported. A pinned main input cannot be unused.");
    cls.def_property("used",
        [](daq::IMultiReader2Params* object)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            return static_cast<bool>(objectPtr.getUsed());
        },
        [](daq::IMultiReader2Params* object, bool used)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            objectPtr.setUsed(used);
        },
        "Whether the reader reads at all; false behaves as if every input were unused.");
    cls.def_property("value_read_type",
        [](daq::IMultiReader2Params* object)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            return objectPtr.getValueReadType();
        },
        [](daq::IMultiReader2Params* object, daq::SampleType type)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            objectPtr.setValueReadType(type);
        },
        "The type every input converts to; Float64 by default. The Python binding reads numeric types only.");
    cls.def_property("min_read_count",
        [](daq::IMultiReader2Params* object)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            return objectPtr.getMinReadCount();
        },
        [](daq::IMultiReader2Params* object, size_t count)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            objectPtr.setMinReadCount(count);
        },
        "The smallest number of samples a read returns; at least 1.");
    cls.def_property("error_policy",
        [](daq::IMultiReader2Params* object)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            return objectPtr.getErrorPolicy();
        },
        [](daq::IMultiReader2Params* object, daq::MultiReader2ErrorPolicy policy)
        {
            const auto objectPtr = daq::MultiReader2ParamsPtr::Borrow(object);
            objectPtr.setErrorPolicy(policy);
        },
        "What an erroring used input does to the reader: Invalidate (default) or Exclude.");
}

PyDaqIntf<daq::IMultiReader2InputStatus, daq::IBaseObject> declareIMultiReader2InputStatus(pybind11::module_ m)
{
    py::enum_<daq::MultiReader2InputError>(m, "MultiReader2InputError")
        .value("None_", daq::MultiReader2InputError::None)
        .value("Disconnected", daq::MultiReader2InputError::Disconnected)
        .value("ValueDescriptorInvalid", daq::MultiReader2InputError::ValueDescriptorInvalid)
        .value("DomainDescriptorInvalid", daq::MultiReader2InputError::DomainDescriptorInvalid)
        .value("SyncFailed", daq::MultiReader2InputError::SyncFailed);

    return wrapInterface<daq::IMultiReader2InputStatus, daq::IBaseObject>(m, "IMultiReader2InputStatus");
}

void defineIMultiReader2InputStatus(pybind11::module_ m, PyDaqIntf<daq::IMultiReader2InputStatus, daq::IBaseObject> cls)
{
    cls.doc() = "One input's state on a status.";

    cls.def_property_readonly("input",
        [](daq::IMultiReader2InputStatus* object)
        {
            const auto objectPtr = daq::MultiReader2InputStatusPtr::Borrow(object);
            return objectPtr.getInput().detach();
        },
        py::return_value_policy::take_ownership,
        "The signal or input port this entry describes.");
    cls.def_property_readonly("used",
        [](daq::IMultiReader2InputStatus* object)
        {
            const auto objectPtr = daq::MultiReader2InputStatusPtr::Borrow(object);
            return static_cast<bool>(objectPtr.getUsed());
        },
        "Whether the input takes part in reading, as configured.");
    cls.def_property_readonly("error",
        [](daq::IMultiReader2InputStatus* object)
        {
            const auto objectPtr = daq::MultiReader2InputStatusPtr::Borrow(object);
            return objectPtr.getError();
        },
        "The input's error; None_ while healthy, persists while the cause holds.");
    cls.def_property_readonly("descriptor_changed",
        [](daq::IMultiReader2InputStatus* object)
        {
            const auto objectPtr = daq::MultiReader2InputStatusPtr::Borrow(object);
            return static_cast<bool>(objectPtr.getDescriptorChanged());
        },
        "Whether the value descriptor is new since the previous status; only while the reader is valid and the input has no error.");
    cls.def_property_readonly("descriptor",
        [](daq::IMultiReader2InputStatus* object)
        {
            const auto objectPtr = daq::MultiReader2InputStatusPtr::Borrow(object);
            return objectPtr.getDescriptor().detach();
        },
        py::return_value_policy::take_ownership,
        "The input's value descriptor; None while the reader is invalid or the input is in error.");
}

PyDaqIntf<daq::IMultiReader2Status, daq::IBaseObject> declareIMultiReader2Status(pybind11::module_ m)
{
    return wrapInterface<daq::IMultiReader2Status, daq::IBaseObject>(m, "IMultiReader2Status");
}

void defineIMultiReader2Status(pybind11::module_ m, PyDaqIntf<daq::IMultiReader2Status, daq::IBaseObject> cls)
{
    cls.doc() = "Immutable result of one read: value descriptor changes per input, the main domain change, a resynchronization, and each input's error.";

    cls.def_property_readonly("has_changes",
        [](daq::IMultiReader2Status* object)
        {
            const auto objectPtr = daq::MultiReader2StatusPtr::Borrow(object);
            return static_cast<bool>(objectPtr.getHasChanges());
        },
        "False: nothing differs from the previous status, count says it all.");
    cls.def_property_readonly("valid",
        [](daq::IMultiReader2Status* object)
        {
            const auto objectPtr = daq::MultiReader2StatusPtr::Borrow(object);
            return static_cast<bool>(objectPtr.getValid());
        },
        "False: the reader delivers nothing and reports no descriptors.");
    cls.def_property_readonly("domain_descriptor_changed",
        [](daq::IMultiReader2Status* object)
        {
            const auto objectPtr = daq::MultiReader2StatusPtr::Borrow(object);
            return static_cast<bool>(objectPtr.getDomainDescriptorChanged());
        },
        "Whether the main domain descriptor is new since the previous status; only while valid.");
    cls.def_property_readonly("domain_descriptor",
        [](daq::IMultiReader2Status* object)
        {
            const auto objectPtr = daq::MultiReader2StatusPtr::Borrow(object);
            return objectPtr.getDomainDescriptor().detach();
        },
        py::return_value_policy::take_ownership,
        "The main input's domain descriptor; None while invalid.");
    cls.def_property_readonly("resynchronized",
        [](daq::IMultiReader2Status* object)
        {
            const auto objectPtr = daq::MultiReader2StatusPtr::Borrow(object);
            return static_cast<bool>(objectPtr.getResynchronized());
        },
        "Whether synchronization restarted, so the next packet offset is discontinuous; only while valid.");
    cls.def_property_readonly("inputs",
        [](daq::IMultiReader2Status* object)
        {
            const auto objectPtr = daq::MultiReader2StatusPtr::Borrow(object);
            return objectPtr.getInputs().detach();
        },
        py::return_value_policy::take_ownership,
        "Every input in slot order, unused ones included.");
    cls.def("get_input_status",
        [](daq::IMultiReader2Status* object, daq::IComponent* input)
        {
            const auto objectPtr = daq::MultiReader2StatusPtr::Borrow(object);
            return objectPtr.getInputStatus(input).detach();
        },
        py::arg("input"),
        py::return_value_policy::take_ownership,
        "One input's entry.");
}

PyDaqIntf<daq::IMultiReader2, daq::IBaseObject> declareIMultiReader2(pybind11::module_ m)
{
    return wrapInterface<daq::IMultiReader2, daq::IBaseObject>(m, "IMultiReader2", py::dynamic_attr());
}

void defineIMultiReader2(pybind11::module_ m, PyDaqIntf<daq::IMultiReader2, daq::IBaseObject> cls)
{
    cls.doc() = "Reads several equal-rate signals at once, aligned onto the main input's tick grid. Everything goes in through configure and comes back on read, as one status per read.";

    m.def("MultiReader2",
        [](daq::IMultiReader2Params* params)
        {
            auto reader = py::cast(daq::MultiReader2_Create(params), py::return_value_policy::take_ownership);
            reader.attr(PARAMS_ATTRIBUTE) = py::cast(params);
            return reader;
        },
        py::arg("params"),
        "Creates a multi reader configured by the params.");

    cls.def("configure",
        [](py::object self, daq::IMultiReader2Params* params)
        {
            daq::IMultiReader2* object = py::cast<daq::IMultiReader2*>(self);
            const auto objectPtr = daq::MultiReader2Ptr::Borrow(object);
            {
                py::gil_scoped_release release;
                objectPtr.configure(params);
            }
            self.attr(PARAMS_ATTRIBUTE) = py::cast(params);
        },
        py::arg("params"),
        "Applies the params wholesale; a rejected params object raises and leaves the reader untouched. Params equal to the current configuration are a no-op.");
    cls.def_property_readonly("main_input",
        [](daq::IMultiReader2* object)
        {
            const auto objectPtr = daq::MultiReader2Ptr::Borrow(object);
            return objectPtr.getMainInput().toStdString();
        },
        "The global id of the main input in effect; empty when no used input is healthy.");
    cls.def_property_readonly("available_count",
        [](daq::IMultiReader2* object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2Ptr::Borrow(object);
            return objectPtr.getAvailableCount();
        },
        "Aligned samples readable right now; 0 unless the reader is streaming and the run reaches MinReadCount.");
    cls.def("read",
        [](py::object self, size_t count)
        {
            daq::IMultiReader2* object = py::cast<daq::IMultiReader2*>(self);
            const auto objectPtr = daq::MultiReader2Ptr::Borrow(object);
            auto buffers = allocate(paramsOf(self), count, false);
            daq::SizeT read = count;
            daq::SizeT offset = 0;
            daq::MultiReader2StatusPtr status;
            {
                py::gil_scoped_release release;
                status = objectPtr.read(count > 0 ? buffers.pointers.data() : nullptr, &read, offset);
            }
            return py::make_tuple(truncate(buffers, 0, read), offset, status.detach());
        },
        py::arg("count"),
        "Reads at most count aligned samples. Returns (values, packet_offset, status): one array per input in slot order, cut to the count read; arrays of unused or erroring inputs hold nothing meaningful. count 0 is the status probe.");
    cls.def("read_with_domain",
        [](py::object self, size_t count)
        {
            daq::IMultiReader2* object = py::cast<daq::IMultiReader2*>(self);
            const auto objectPtr = daq::MultiReader2Ptr::Borrow(object);
            auto buffers = allocate(paramsOf(self), count, true);
            daq::SizeT read = count;
            daq::MultiReader2StatusPtr status;
            {
                py::gil_scoped_release release;
                status = objectPtr.readWithDomain(count > 0 ? buffers.pointers.data() : nullptr, &read);
            }
            return py::make_tuple(buffers.arrays[0][py::slice(0, read, 1)], truncate(buffers, 1, read), status.detach());
        },
        py::arg("count"),
        "Reads at most count aligned samples with their main-tick timestamps. Returns (ticks, values, status).");
    cls.def("skip_samples",
        [](daq::IMultiReader2* object, size_t count)
        {
            const auto objectPtr = daq::MultiReader2Ptr::Borrow(object);
            daq::SizeT skipped = count;
            daq::MultiReader2StatusPtr status;
            {
                py::gil_scoped_release release;
                status = objectPtr.skipSamples(&skipped);
            }
            return py::make_tuple(skipped, status.detach());
        },
        py::arg("count"),
        "Discards at most count aligned samples without copying. Returns (count, status).");
    cls.def_property_readonly("on_data_available",
        [](daq::IMultiReader2* object)
        {
            const auto objectPtr = daq::MultiReader2Ptr::Borrow(object);
            return objectPtr.getOnDataAvailable().getEventPtr().detach();
        },
        py::return_value_policy::take_ownership,
        "The event scheduled when data becomes deliverable or a status with changes awaits a read.");
}
