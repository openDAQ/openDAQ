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

#include <pybind11/gil.h>
#include <pybind11/numpy.h>

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"

namespace
{

// The reader's Python object keeps the read type and the last status: a read sizes its buffers by the status before it
constexpr const char* READ_TYPE_ATTRIBUTE = "__mr2_read_type";
constexpr const char* STATUS_ATTRIBUTE = "__mr2_status";

py::object pyReader(const daq::MultiReader2Ptr& reader)
{
    return py::cast(InterfaceWrapper<daq::IMultiReader2>(reader.addRefAndReturn()));
}

py::object pyStatus(const daq::MultiReader2StatusPtr& status)
{
    return py::cast(InterfaceWrapper<daq::IMultiReader2Status>(status.addRefAndReturn()));
}

void keep(const daq::MultiReader2Ptr& reader, const daq::MultiReader2ParamsPtr& params, const daq::MultiReader2StatusPtr& status)
{
    const auto object = pyReader(reader);
    if (params.assigned())
        object.attr(READ_TYPE_ATTRIBUTE) = py::cast(params.getValueReadType());
    object.attr(STATUS_ATTRIBUTE) = pyStatus(status);
}

// Float64, the default, for a reader created outside Python
daq::SampleType keptReadType(const py::object& object)
{
    return py::hasattr(object, READ_TYPE_ATTRIBUTE) ? object.attr(READ_TYPE_ATTRIBUTE).cast<daq::SampleType>() : daq::SampleType::Float64;
}

daq::MultiReader2StatusPtr keptStatus(const py::object& object)
{
    if (!py::hasattr(object, STATUS_ATTRIBUTE))
        return nullptr;
    daq::IMultiReader2Status* status = object.attr(STATUS_ATTRIBUTE).cast<daq::IMultiReader2Status*>();
    return daq::MultiReader2StatusPtr(status);
}

py::dtype dtypeOf(daq::SampleType type)
{
    switch (type)
    {
        case daq::SampleType::Float32:
            return py::dtype("float32");
        case daq::SampleType::Float64:
            return py::dtype("float64");
        case daq::SampleType::UInt8:
            return py::dtype("uint8");
        case daq::SampleType::Int8:
            return py::dtype("int8");
        case daq::SampleType::UInt16:
            return py::dtype("uint16");
        case daq::SampleType::Int16:
            return py::dtype("int16");
        case daq::SampleType::UInt32:
            return py::dtype("uint32");
        case daq::SampleType::Int32:
            return py::dtype("int32");
        case daq::SampleType::UInt64:
            return py::dtype("uint64");
        case daq::SampleType::Int64:
            return py::dtype("int64");
        default:
            return py::dtype("uint8");  // a struct: its raw bytes
    }
}

// One input's samples in Python: the element type and the elements per sample, none for an input that does not contribute
struct Layout
{
    py::dtype dtype;
    size_t elements = 0;
};

std::vector<Layout> layoutsOf(const daq::MultiReader2StatusPtr& status, daq::SampleType readType)
{
    std::vector<Layout> layouts;
    const bool invalid = status.getStatus() == daq::ComponentStatus::Error;
    for (const daq::MultiReader2InputStatusPtr in : status.getInputs())
    {
        const auto descriptor = in.getDescriptor();
        const auto type = readType != daq::SampleType::Undefined ? readType
                          : descriptor.assigned()                ? descriptor.getSampleType()
                                                                 : daq::SampleType::Float64;
        Layout layout{dtypeOf(type)};
        if (!invalid && in.getError() == daq::MultiReader2InputError::None)
            layout.elements = in.getSampleSize() / layout.dtype.itemsize();
        layouts.push_back(std::move(layout));
    }
    return layouts;
}

// One array per input, cut to the count read, and None for an input in error, whose buffer is not written
class Values
{
public:
    Values(const std::vector<Layout>& layouts, size_t count, std::vector<void*>& buffers, size_t first)
    {
        for (size_t i = 0; i < layouts.size(); ++i)
        {
            elements.push_back(layouts[i].elements);
            arrays.emplace_back(layouts[i].dtype, std::vector<py::ssize_t>{py::ssize_t(count * layouts[i].elements)});
            if (layouts[i].elements > 0)
                buffers[first + i] = arrays.back().mutable_data();
        }
    }

    py::list result(size_t got) const
    {
        py::list list;
        for (size_t i = 0; i < arrays.size(); ++i)
            if (elements[i] > 0)
                list.append(arrays[i][py::slice(0, py::ssize_t(got * elements[i]), 1)]);
            else
                list.append(py::none());
        return list;
    }

private:
    std::vector<py::array> arrays;
    std::vector<size_t> elements;
};

// A read sized by the last status. Before the first status there is none: the read takes the status owed since
// construction, with no samples. With the domain, the main ticks come first, as the buffers do
py::tuple read(daq::IMultiReader2* object, size_t count, bool withDomain)
{
    const auto reader = daq::MultiReader2Ptr::Borrow(object);
    const auto pyObject = pyReader(reader);
    const auto previous = keptStatus(pyObject);
    if (!previous.assigned())
        count = 0;
    const auto layouts = previous.assigned() ? layoutsOf(previous, keptReadType(pyObject)) : std::vector<Layout>();

    const size_t first = withDomain ? 1 : 0;
    std::vector<void*> buffers(first + layouts.size(), nullptr);
    const Values values(layouts, count, buffers, first);
    py::array_t<int64_t> ticks(withDomain ? count : 0);
    if (withDomain)
        buffers[0] = ticks.mutable_data();

    daq::SizeT got = count;
    daq::SizeT offset = 0;
    daq::IMultiReader2Status* raw = nullptr;
    daq::ErrCode err;
    {
        py::gil_scoped_release release;
        void* data = count > 0 ? buffers.data() : nullptr;
        err = withDomain ? object->readWithDomain(data, &got, 0, &raw) : object->read(data, &got, 0, &offset, &raw);
    }
    daq::checkErrorInfo(err);
    const daq::MultiReader2StatusPtr status(std::move(raw));
    keep(reader, nullptr, status);

    if (withDomain)
        return py::make_tuple(ticks[py::slice(0, py::ssize_t(got), 1)], values.result(got), pyStatus(status));
    return py::make_tuple(values.result(got), offset, pyStatus(status));
}

}

PyDaqIntf<daq::IMultiReader2, daq::IBaseObject> declareIMultiReader2(pybind11::module_ m)
{
    return wrapInterface<daq::IMultiReader2, daq::IBaseObject>(m, "IMultiReader2", py::dynamic_attr());
}

void defineIMultiReader2(pybind11::module_ m, PyDaqIntf<daq::IMultiReader2, daq::IBaseObject> cls)
{
    cls.doc() = "Reads several signals at once, aligned onto the main input's tick grid.";

    m.def("MultiReader2",
        [](daq::IMultiReader2Params* params)
        {
            const auto paramsPtr = daq::MultiReader2ParamsPtr::Borrow(params);
            daq::MultiReader2Ptr reader;
            {
                py::gil_scoped_release release;
                reader = daq::MultiReader2(paramsPtr);
            }
            const auto object = pyReader(reader);
            object.attr(READ_TYPE_ATTRIBUTE) = py::cast(paramsPtr.getValueReadType());
            return object;
        },
        py::arg("params"),
        "Creates a reader of the params' inputs, aligned onto the main input's tick grid.");

    cls.def("configure",
        [](daq::IMultiReader2 *object, daq::IMultiReader2Params* params)
        {
            const auto objectPtr = daq::MultiReader2Ptr::Borrow(object);
            const auto paramsPtr = daq::MultiReader2ParamsPtr::Borrow(params);
            daq::MultiReader2StatusPtr status;
            {
                py::gil_scoped_release release;
                status = objectPtr.configure(paramsPtr);
            }
            keep(objectPtr, paramsPtr, status);
            return pyStatus(status);
        },
        py::arg("params"),
        "Validates the params, then applies them as if the reader were newly constructed.");
    cls.def_property_readonly("available_count",
        [](daq::IMultiReader2 *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2Ptr::Borrow(object);
            return objectPtr.getAvailableCount();
        },
        "Gets the count the next read returns in full.");
    cls.def("read",
        [](daq::IMultiReader2 *object, size_t count)
        {
            return read(object, count, false);
        },
        py::arg("count"),
        "Reads up to `count` aligned samples: (values, offset, status), with an array per input in slot order, cut to the "
        "count read, and None for an input in error. The first read after construction only takes the status.");
    cls.def("read_with_domain",
        [](daq::IMultiReader2 *object, size_t count)
        {
            return read(object, count, true);
        },
        py::arg("count"),
        "As `read`, with the samples' Int64 main ticks counted from the main origin first: (ticks, values, status).");
    cls.def("skip_samples",
        [](daq::IMultiReader2 *object, size_t count)
        {
            const auto objectPtr = daq::MultiReader2Ptr::Borrow(object);
            daq::SizeT offset = 0;
            daq::MultiReader2StatusPtr status;
            {
                py::gil_scoped_release release;
                status = objectPtr.skipSamples(&count, 0, offset);
            }
            keep(objectPtr, nullptr, status);
            return py::make_tuple(count, offset, pyStatus(status));
        },
        py::arg("count"),
        "Behaves exactly like `read`, without copying: (count, offset, status).");
    cls.def_property_readonly("on_data_available",
        [](daq::IMultiReader2 *object)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::MultiReader2Ptr::Borrow(object);
            return objectPtr.getOnDataAvailable().getEventPtr().detach();
        },
        py::return_value_policy::take_ownership,
        "Gets the event called when data becomes readable or a status with changes awaits a read.");
}
