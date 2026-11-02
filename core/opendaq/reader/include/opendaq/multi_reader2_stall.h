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
#include <opendaq/logger_component_ptr.h>
#include <opendaq/multi_reader2_slot.h>

#include <string>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Internal to MultiReader2: an input with nothing queued while the others sent more than queueLimitSeconds past its
 * newest sample is stalled, and a stalled or ahead input rejoins once its newest sample is within rejoinSeconds of the others'.
 * MultiReader2DataManager calls it at the queue limit on ingest, at the start of a read and on ingest of an input in
 * SyncFailed. With stallEnabled false no input stalls or rejoins, and a full queue drops its data whether or not another input
 * has nothing queued.
 */
namespace multi_reader2
{

constexpr bool stallEnabled = true;

// Narrower than the queue limit, so an input whose delay sits at the limit does not rejoin and stall again on every packet
constexpr std::int64_t rejoinSeconds = 1;

std::string stalledText();

// A contributing input has nothing queued: a full queue then keeps its data for the next read to stall that input
bool stallPending(const std::vector<Slot>& slots, bool valid);

// A contributing input has nothing queued while another sent more than queueLimitSeconds past its newest sample, or past
// the first sample of any input when it never sent one. Ticks counted rather than queued, so the byte limit hides no stall
bool stallDue(const std::vector<Slot>& slots, bool valid, const TickDomain& main);

// Marks every input stallDue finds as stalled; the caller judges the inputs again
void stall(std::vector<Slot>& slots, bool valid, const TickDomain& main, const LoggerComponentPtr& loggerComponent);

// On ingest of input i: queues its rejoin when it is stalled or ahead and its newest sample is within rejoinSeconds of the
// newest sample of the inputs without an error
void checkRejoin(std::vector<Slot>& slots, SizeT i, const TickDomain& main, const LoggerComponentPtr& loggerComponent);
}

END_NAMESPACE_OPENDAQ
