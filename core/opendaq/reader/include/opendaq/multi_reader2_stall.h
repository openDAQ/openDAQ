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

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Internal to MultiReader2: finds a main input that stopped sending while another input went on, which the queue limits
 * would otherwise hide by dropping the other inputs' data with no error. MultiReader2DataManager::ingestData calls it.
 */
namespace multi_reader2
{

// The status text of a stalled main input, which fails as late and comes back as a late input does
std::string stalledText();

// On ingest of other, both contributing: fails main as late when other is more than two sync limits past main's newest
// sample, or while synchronizing with no main data queued, past its own first sample since the start
void checkStalled(Slot& main, const Slot& other, bool streaming, std::int64_t syncLimit, const LoggerComponentPtr& loggerComponent);
}

END_NAMESPACE_OPENDAQ
