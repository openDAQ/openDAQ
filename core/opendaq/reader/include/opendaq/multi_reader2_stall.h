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

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Internal to MultiReader2: a main input that stopped sending while another input went on. Without it the queue limits
 * drop the other inputs' data with no error. The rule is this unit, its one call in MultiReader2DataManager::ingestData and
 * the text the judgement reports; deleting those removes it.
 */
namespace multi_reader2
{

// The status text of a stalled main input, which fails as late and comes back as a late input does
constexpr const char* stalledText = "not synchronized, more than 4 s behind the other inputs";

// On ingest of other, both contributing: fails main as late when other is more than two sync limits past main's newest
// sample, or while synchronizing with no main data queued, past its own first sample since the start. Returns whether main failed
bool checkStalled(Slot& main, const Slot& other, bool streaming, std::int64_t syncLimit, const LoggerComponentPtr& loggerComponent);

}

END_NAMESPACE_OPENDAQ
