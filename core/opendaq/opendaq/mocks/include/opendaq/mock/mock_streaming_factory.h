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
#include <opendaq/streaming_ptr.h>
#include <opendaq/mock/mock_streaming.h>

BEGIN_NAMESPACE_OPENDAQ

inline StreamingPtr MockStreaming(const daq::StringPtr& connectionString, const ContextPtr& context)
{
    StreamingPtr obj(MockStreaming_Create(connectionString, context));
    return obj;
}

END_NAMESPACE_OPENDAQ
