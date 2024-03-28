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

#pragma once

#include "opendaq/opendaq.h"
#include "pybind11/numpy.h"

namespace py = pybind11;

BEGIN_NAMESPACE_OPENDAQ

class MockSignal
{
public:
    static std::string makeEpoch(const std::chrono::system_clock::time_point& timePoint);

    MockSignal();
    MockSignal(const MockSignal&) = delete;
    MockSignal& operator=(const MockSignal&) = delete;

    daq::SignalConfigPtr getSignal() const;

    daq::SignalConfigPtr getDomainSignal() const;

    void addData(const py::array_t<double>& data);

private:
    daq::LoggerPtr logger;
    daq::ContextPtr context;
    daq::SchedulerPtr scheduler;

    daq::SignalConfigPtr signal;
    daq::SignalConfigPtr domainSignal;
};

END_NAMESPACE_OPENDAQ