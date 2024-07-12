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

#include "websocket_streaming/websocket_streaming.h"
#include <opendaq/data_descriptor_ptr.h>

#include <optional>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

struct SignalProps
{
    std::optional<std::string> name;
    std::optional<std::string> description;
};

struct SubscribedSignalInfo
{
    DataDescriptorPtr dataDescriptor;
    SignalProps signalProps;
    std::string signalName;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
