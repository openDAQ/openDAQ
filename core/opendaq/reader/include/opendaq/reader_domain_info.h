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
#pragma once
#include <opendaq/reader_utils.h>
#include <opendaq/custom_log.h>
#include <opendaq/logger_component_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

struct ReaderDomainInfo
{
    explicit ReaderDomainInfo(LoggerComponentPtr loggerComponent)
        : loggerComponent(std::move(loggerComponent))
    {
    }

    void setMaxResolution(const RatioPtr& maxResolution)
    {
        multiplier = Ratio(
                resolution.getNumerator() * maxResolution.getDenominator(),
                resolution.getDenominator() * maxResolution.getNumerator())
            .simplify();

        LOG_T("Multiplier: {} / {}", multiplier.getNumerator(), multiplier.getDenominator())
    }

    void setEpochOffset(std::chrono::system_clock::time_point minEpoch, const RatioPtr& maxResolution)
    {
        using namespace reader;

#if !defined(NDEBUG)
        readResolution = maxResolution;
        readEpoch = minEpoch;
#endif

        // In system-clock ticks
        offset = epoch.time_since_epoch().count() - minEpoch.time_since_epoch().count(); 

        LOG_T("Epoch: {}", timePointString(epoch))
        LOG_T("Offset: {}", offset)

        if (offset != 0)
        {
            using SysPeriod = std::chrono::system_clock::period;

            // convert to maxResolution ticks
            offset = offset * (SysPeriod::num * maxResolution.getDenominator()) / (SysPeriod::den * maxResolution.getNumerator());
        }

        LOG_T("Adj. offset: {}", offset)
    }

    void setEpochOffsetRemainder(std::int64_t remainder)
    {
    }

    RatioPtr resolution{};
    RatioPtr multiplier{};
    std::int64_t offset{};
    std::int64_t roundedOffset{};
    std::chrono::system_clock::time_point epoch{};

    LoggerComponentPtr loggerComponent;

#if !defined(NDEBUG)
    RatioPtr readResolution{};
    std::chrono::system_clock::time_point readEpoch{};
#endif
};

END_NAMESPACE_OPENDAQ
