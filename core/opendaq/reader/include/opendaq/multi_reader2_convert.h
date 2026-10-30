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
#include <opendaq/data_packet_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

// Internal to MultiReader2: how an input's samples land in the read buffers
namespace multi_reader2
{

// A read type a reader can be configured with: Undefined or a real or integer type
bool isReadType(SampleType readType);

// Bytes a sample takes in a buffer of the read type, 0 when it does not convert; Undefined takes the descriptor's own scaled type
SizeT readSampleSize(const DataDescriptorPtr& value, SampleType readType);

// Copies count samples from first into dst, scaled then converted: integers round half away from zero and saturate, NaN is 0
void copySamples(const DataPacketPtr& packet, SizeT first, SizeT count, void* dst, SampleType readType);

}

END_NAMESPACE_OPENDAQ
