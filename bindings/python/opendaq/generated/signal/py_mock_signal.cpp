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

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "py_opendaq/py_mock_signal.h"
#include "py_opendaq/py_opendaq.h"

BEGIN_NAMESPACE_OPENDAQ

MockSignal::MockSignal()
{
    logger = daq::Logger();
    context = daq::Context(daq::Scheduler(logger, 1), logger, nullptr, nullptr, nullptr);
    scheduler = context.getScheduler();
    signal = daq::Signal(context, nullptr, "MockValueSignal");
    domainSignal = daq::Signal(context, nullptr, "MockDomainSignal");

    auto valueDescriptor = daq::DataDescriptorBuilder().setSampleType(daq::SampleType::Float64).build();
    auto domainDescriptor = daq::DataDescriptorBuilder()
                                .setSampleType(daq::SampleType::RangeInt64)
                                .setRule(daq::LinearDataRule(1, 0))
                                .setOrigin(MockSignal::makeEpoch(std::chrono::system_clock::now()))
                                .setTickResolution(daq::Ratio(1, 1000))
                                .setUnit(daq::Unit("s", -1, "seconds", "time"))
                                .build();

    signal->setDescriptor(valueDescriptor);
    domainSignal->setDescriptor(domainDescriptor);
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
    auto domainPacket = daq::DataPacket(domainSignal.getDescriptor(), size, 0);
    auto valuePacket = daq::DataPacketWithDomain(domainPacket, signal.getDescriptor(), size);

    auto dataPtr = static_cast<daq::SampleTypeToType<daq::SampleType::Float64>::Type*>(valuePacket.getData());
    for (auto i = 0; i < size; i++)
        dataPtr[i] = data.at(i);

    signal.sendPacket(valuePacket);
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

    cls.def(py::init([]() { return std::make_unique<daq::MockSignal>(); }), "Constructs a mock signal.");

    cls.def(
        "add_data",
        [](daq::MockSignal* object, py::array_t<double, py::array::c_style> data) { object->addData(data); },
        py::arg("data"),
        "Adds the given data to the signal.");

    cls.def_property_readonly(
        "signal", [](daq::MockSignal* object) { return object->getSignal().detach(); }, "The value signal.");

    cls.def_property_readonly(
        "domain_signal", [](daq::MockSignal* object) { return object->getDomainSignal().detach(); }, "The domain signal.");
}
