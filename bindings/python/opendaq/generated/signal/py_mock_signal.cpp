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

#include <chrono>
#include <cstddef>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "py_opendaq/py_mock_signal.h"
#include "py_opendaq/py_opendaq.h"

BEGIN_NAMESPACE_OPENDAQ

MockSignal::MockSignal(const std::string& id, const std::string& epoch, bool initValueDescriptor)
{
    signal = daq::Signal(NullContext(), nullptr, id + "_valueSignal");
    domainSignal = daq::Signal(NullContext(), nullptr, id + "_domainSignal");

    auto valueDescriptor = daq::DataDescriptorBuilder()
                               .setSampleType(daq::SampleType::Float64)
                               .setUnit(Unit("V", -1, "volts", "voltage"))
                               .setName(id + "_values")
                               .build();
    auto domainDescriptor = daq::DataDescriptorBuilder()
                                .setSampleType(daq::SampleType::Int64)
                                .setUnit(daq::Unit("s", -1, "seconds", "time"))
                                .setTickResolution(daq::Ratio(1, 1000))
                                .setRule(daq::LinearDataRule(1, 0))
                                .setOrigin(epoch.empty() ? MockSignal::currentEpoch() : epoch)
                                .setName(id + "_time")
                                .build();

    if (initValueDescriptor)
        signal->setDescriptor(valueDescriptor);
    domainSignal->setDescriptor(domainDescriptor);
    signal->setDomainSignal(domainSignal);
}

daq::SignalConfigPtr MockSignal::getSignal() const
{
    return signal;
}

daq::SignalConfigPtr MockSignal::getDomainSignal() const
{
    return domainSignal;
}

void MockSignal::addData(const py::array_t<double>& data)
{
    const auto size = data.size();
    auto domainPacket = daq::DataPacket(domainSignal.getDescriptor(), size, offset);
    auto valuePacket = daq::DataPacketWithDomain(domainPacket, signal.getDescriptor(), size);

    auto dataPtr = static_cast<daq::SampleTypeToType<daq::SampleType::Float64>::Type*>(valuePacket.getRawData());
    for (auto i = 0; i < size; ++i)
        dataPtr[i] = data.at(i);

    signal.sendPacket(valuePacket);
    domainSignal.sendPacket(domainPacket);
    offset += size;
}

void MockSignal::addObjects(const py::object& objects, bool updateDescriptor)
{
    daq::DataDescriptorPtr descriptor;
    size_t size = 1;
    if (py::isinstance<py::dict>(objects) || py::isinstance<py::list>(objects))
    {
        if (py::isinstance<py::dict>(objects))
        {
            const auto& dict = py::cast<py::dict>(objects);
            if (dict.empty())
                throw daq::InvalidParameterException("Cannot add empty dictionary");
            descriptor = createDataDescriptor(dict, "obj");
        }
        else if (py::isinstance<py::list>(objects))
        {
            const auto& list = py::cast<py::list>(objects);
            if (list.empty())
                throw daq::InvalidParameterException("Cannot add empty list");
            descriptor = createDataDescriptor(list[0], "obj");
            size = list.size();
        }
    }
    else
    {
        throw daq::InvalidParameterException("Unsupported type: Should be a list or a dictionary");
    }

    if (updateDescriptor)
        signal.setDescriptor(descriptor);

    std::function<void*(const py::object&, void*)> fillBuffer = [&fillBuffer](const py::object& obj, void* data) -> void*
    {
        if (py::isinstance<py::dict>(obj))
        {
            for (const auto& field : py::cast<py::dict>(obj))
            {
                auto& value = field.second;
                data = fillBuffer(py::cast<py::object>(value), data);
            }
        }
        else if (py::isinstance<py::list>(obj))
        {
            for (const auto& currentObj : obj)
            {
                data = fillBuffer(py::cast<py::object>(currentObj), data);
            }
        }
        else if (py::isinstance<py::int_>(obj))
        {
            *static_cast<daq::SampleTypeToType<daq::SampleType::Int64>::Type*>(data) =
                py::cast<daq::SampleTypeToType<daq::SampleType::Int64>::Type>(obj);
            data = static_cast<daq::SampleTypeToType<daq::SampleType::Int64>::Type*>(data) + 1;
        }
        else if (py::isinstance<py::float_>(obj))
        {
            *static_cast<daq::SampleTypeToType<daq::SampleType::Float64>::Type*>(data) =
                py::cast<daq::SampleTypeToType<daq::SampleType::Float64>::Type>(obj);
            data = static_cast<daq::SampleTypeToType<daq::SampleType::Float64>::Type*>(data) + 1;
        }
        else
        {
            throw py::type_error("Unsupported type");
        }
        return data;
    };

    auto domainPacket = daq::DataPacket(domainSignal.getDescriptor(), size, offset);
    auto valuePacket = daq::DataPacketWithDomain(domainPacket, signal.getDescriptor(), size);
    if (py::isinstance<py::dict>(objects))
    {
        const auto& dict = py::cast<py::dict>(objects);
        fillBuffer(py::cast<py::object>(dict), valuePacket.getRawData());
    }
    else if (py::isinstance<py::list>(objects))
    {
        const auto& list = py::cast<py::list>(objects);
        fillBuffer(py::cast<py::object>(list), valuePacket.getRawData());
    }

    signal.sendPacket(valuePacket);
    domainSignal.sendPacket(domainPacket);
    offset += size;
}

daq::DataDescriptorPtr MockSignal::createDataDescriptor(const py::object& object, const std::string& name)
{
    if (py::isinstance<py::list>(object))
    {
        const auto& list = py::cast<py::list>(object);
        if (list.empty())
        {
            throw py::value_error("Cannot create DataDescriptor from empty list");
        }

        const auto& first = list[0];

        auto firstDescriptor = createDataDescriptor(first, name);
        auto listDescriptorBuilder = daq::DataDescriptorBuilderCopy(firstDescriptor);
        listDescriptorBuilder.setDimensions(
            daq::List<IDimension>(DimensionBuilder().setRule(LinearDimensionRule(0, 1, list.size())).build()));

        return listDescriptorBuilder.build();
    }
    else if (py::isinstance<py::dict>(object))
    {
        auto fields = daq::List<daq::DataDescriptorPtr>();
        for (const auto& field : py::cast<py::dict>(object))
        {
            const std::string& field_name = py::cast<std::string>(field.first);
            const py::object& value = py::cast<py::object>(field.second);

            fields.pushBack(createDataDescriptor(value, field_name));
        }

        auto structDescriptor = daq::DataDescriptorBuilder();
        return structDescriptor.setName(name)
            .setSampleType(daq::SampleType::Struct)
            .setStructFields(fields)
            .setRule(ExplicitDataRule())
            .build();
    }
    else if (py::isinstance<py::int_>(object))
    {
        auto intDescriptor = daq::DataDescriptorBuilder();
        return intDescriptor.setName(name).setSampleType(daq::SampleType::Int64).setRule(ExplicitDataRule()).build();
    }
    else if (py::isinstance<py::float_>(object))
    {
        auto floatDescriptor = daq::DataDescriptorBuilder();
        return floatDescriptor.setName(name).setSampleType(daq::SampleType::Float64).setRule(ExplicitDataRule()).build();
    }
    throw py::type_error("Unsupported argument type");
}

std::string MockSignal::currentEpoch()
{
    return makeEpoch(std::chrono::system_clock::now());
}

std::string MockSignal::makeEpoch(const std::chrono::system_clock::time_point& timePoint)
{
    std::stringstream ss;
    auto time = std::chrono::system_clock::to_time_t(timePoint);
    ss << std::put_time(std::localtime(&time), "%FT%T%z");
    return ss.str();
}

END_NAMESPACE_OPENDAQ

py::class_<daq::MockSignal> declareMockSignal(py::module_ m)
{
    return py::class_<daq::MockSignal>(m, "MockSignal");
}

void defineMockSignal(pybind11::module_ m, py::class_<daq::MockSignal> cls)
{
    cls.doc() = "A mock signal that can be used for testing purposes.";

    cls.def(py::init([](const std::string& id, const std::string& epoch, bool initializeValueDescriptor)
                     { return std::make_unique<daq::MockSignal>(id, epoch, initializeValueDescriptor); }),
            py::arg("id") = "mock",
            py::arg("epoch") = "",
            py::arg("initialize_value_descriptor") = true,
            "Constructs a mock signal.");

    cls.def(
        "add_data",
        [](daq::MockSignal* object, py::array_t<double, py::array::c_style> data) { object->addData(data); },
        py::arg("data"),
        "Adds the given data to the signal.");

    cls.def(
        "add_objects",
        [](daq::MockSignal* object, const py::object& objects, bool updateDescriptor) { object->addObjects(objects, updateDescriptor); },
        py::arg("objects"),
        py::arg("update_descriptor") = true,
        "Adds the given data objects to the signal. Should be either a list or a dictionary. Only int and float are supported.");

    cls.def_property_readonly("signal", [](daq::MockSignal* object) { return object->getSignal().detach(); }, "The value signal.");

    cls.def_property_readonly(
        "domain_signal", [](daq::MockSignal* object) { return object->getDomainSignal().detach(); }, "The domain signal.");

    cls.def_static("current_epoch", &daq::MockSignal::currentEpoch, "Returns the current epoch in the format 'YYYY-MM-DDTHH:MM:SS+ZZZZ'.");
}
