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
#include <opendaq/function_block_ptr.h>
#include <opendaq/channel_ptr.h>
#include "opcuatms_server/objects/tms_server_function_block.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsServerChannel;
using TmsServerChannelPtr = std::shared_ptr<TmsServerChannel>;

class TmsServerChannel : public TmsServerFunctionBlock<ChannelPtr>
{
public:
    using Super = TmsServerFunctionBlock<ChannelPtr>;

    TmsServerChannel(const ChannelPtr& object, const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext);

protected:
    opcua::OpcUaNodeId getTmsTypeId() override;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
