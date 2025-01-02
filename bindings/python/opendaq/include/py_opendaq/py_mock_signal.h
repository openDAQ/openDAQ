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

#pragma once

#include "opendaq/opendaq.h"
#include "pybind11/numpy.h"

namespace py = pybind11;

BEGIN_NAMESPACE_OPENDAQ

class MockSignal
{
public:
    static std::string currentEpoch();
    static std::string makeEpoch(const std::chrono::system_clock::time_point& timePoint = std::chrono::system_clock::now());

    MockSignal(const std::string& id = "mock", const std::string& epoch = {}, bool initValueDescriptor = true);

    daq::SignalConfigPtr getSignal() const;

    daq::SignalConfigPtr getDomainSignal() const;

    void addData(const py::array_t<double>& data);
    void addObjects(const py::object& objects, bool updateDescriptor);

private:
    static daq::DataDescriptorPtr createDataDescriptor(const py::object& object, const std::string& name = {});

    daq::LoggerPtr logger;
    daq::ContextPtr context;
    daq::SchedulerPtr scheduler;

    daq::SignalConfigPtr signal;
    daq::SignalConfigPtr domainSignal;

    int64_t offset{0};
};

END_NAMESPACE_OPENDAQ