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
#pragma once
#include <opendaq/data_descriptor_ptr.h>

#include <cstdint>
#include <optional>
#include <string>

BEGIN_NAMESPACE_OPENDAQ

// Internal to MultiReader2: the domain constraints and the tick arithmetic onto the main input's grid
namespace multi_reader2
{

/*!
 * @brief A domain that passes the constraints an input is judged on alone: full-second origin, integer scalar ticks in
 * seconds, a linear rule with an integer delta above zero and an integer start, and an integer rate.
 */
struct TickDomain
{
    std::int64_t originSeconds = 0;  // since the Unix epoch
    std::int64_t resNum = 1;         // tick resolution, reduced
    std::int64_t resDen = 1;
    std::int64_t delta = 1;
    std::int64_t start = 0;
    std::int64_t referenceOffset = 0;  // the reference domain offset, in this domain's ticks
};

// nullopt for a domain that breaks a constraint; fault then names it
std::optional<TickDomain> parseDomain(const DataDescriptorPtr& domain, const char** fault = nullptr);

// Seconds since the Unix epoch of an ISO 8601 origin on a full second; nullopt for anything else
std::optional<std::int64_t> parseOrigin(const std::string& origin);

/*!
 * @brief An input's ticks in main ticks: main = floor((tick + shift) * num / den) + offset. The remainder of the division is
 * 0 exactly when the tick lies on the main input's tick resolution. The input's reference domain offset is the shift, and
 * the main input's is taken off the offset, so both are compared on their reference domain time.
 */
struct Relation
{
    std::int64_t num = 1;
    std::int64_t den = 1;
    std::int64_t offset = 0;
    std::int64_t shift = 0;

    std::int64_t toMain(std::int64_t tick, std::int64_t* remainder = nullptr) const;
};

// nullopt, with fault naming the cause, when the rates differ or the origin difference is not a whole Int64 count of main ticks
std::optional<Relation> relate(const TickDomain& input, const TickDomain& main, const char** fault = nullptr);

// The sync limit, 2 s of data and at least one tick, in main ticks
std::int64_t syncLimitTicks(const TickDomain& main);

// a + b, or nullopt when the sum leaves Int64
std::optional<std::int64_t> exactSum(std::int64_t a, std::int64_t b);

}

END_NAMESPACE_OPENDAQ
