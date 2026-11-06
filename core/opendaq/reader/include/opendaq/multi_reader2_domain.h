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

/*!
 * @brief Internal to MultiReader2: the domain constraints and the tick arithmetic onto the main input's grid (spec 0007,
 * Domain constraints). Pure functions with checked Int64 arithmetic, so every overflow is a failed constraint and never a
 * wrong tick. Tested on their own in test_domain_and_queue.cpp.
 *
 * Future design hooks: fractional origins change TickMap::offset into a rational and parseDomain's origin rule; dividers relax relate's equal-rate rule to an integer ratio, which makes TickMap::step a multiple of the main
 * delta; reference domains add their offset to TickMap::offset.
 */
namespace multi_reader2
{

// a + b, a - b and a * b, or nullopt when the result leaves Int64
std::optional<std::int64_t> checkedAdd(std::int64_t a, std::int64_t b);
std::optional<std::int64_t> checkedSub(std::int64_t a, std::int64_t b);
std::optional<std::int64_t> checkedMul(std::int64_t a, std::int64_t b);

// a - b > limit; a difference beyond Int64 exceeds every limit
bool exceeds(std::int64_t a, std::int64_t b, std::int64_t limit);

// A domain descriptor that meets the constraints an input meets on its own: integer scalar ticks in seconds, a linear rule
// with an integer delta above 0 and an integer start, an integer rate, and an ISO 8601 origin on a full second or none
struct TickDomain
{
    std::int64_t resolutionNum = 1;  // seconds per tick
    std::int64_t resolutionDen = 1;
    std::int64_t delta = 1;          // ticks per sample
    std::int64_t start = 0;          // a packet's first tick is its packetOffset plus start
    std::int64_t rate = 1;           // samples per second
    std::int64_t originSeconds = 0;  // since the Unix epoch
    bool relative = false;           // no origin: read only with other domains without one, on a shared time base

    // That many seconds of data in samples and in ticks; saturated, so a limit never wraps
    std::int64_t samplesIn(std::int64_t seconds) const;
    std::int64_t ticksIn(std::int64_t seconds) const;
};

// nullopt for a domain that breaks one of the constraints
std::optional<TickDomain> parseDomain(const DataDescriptorPtr& domain);

// Seconds since the Unix epoch of an ISO 8601 origin on a full second: a date and time with an offset (Z, +hh:mm, +hhmm or
// +hh; none is UTC), a date alone, or a year or a year and month alone, which start on the first day at midnight UTC.
// nullopt for anything else, a trailing remainder included
std::optional<std::int64_t> parseOrigin(const std::string& origin);

// One input's ticks on the main input's grid. The packet's first tick, packetOffset + start, maps to the main tick
// tick / q * p + offset when tick is a multiple of q; any other tick falls between two main ticks (off grid)
struct TickMap
{
    std::int64_t p = 1;       // input tick resolution / main tick resolution, reduced
    std::int64_t q = 1;
    std::int64_t offset = 0;  // the origins' difference in main ticks
    std::int64_t start = 0;   // the input rule's start
    std::int64_t step = 1;    // main ticks from one input sample to the next: the main delta at equal rates

    // The main tick of a packet's first sample; nullopt when it is off the main resolution or out of Int64
    std::optional<std::int64_t> firstTick(std::int64_t packetOffset) const;
};

// The map of the input's ticks onto the main input's; nullopt when the two cannot be read together: the rates differ, only
// one of the two has an origin, or the origins are not a whole number of main ticks apart within Int64 (DomainDescriptorInvalid)
std::optional<TickMap> relate(const TickDomain& input, const TickDomain& main);

}

END_NAMESPACE_OPENDAQ
