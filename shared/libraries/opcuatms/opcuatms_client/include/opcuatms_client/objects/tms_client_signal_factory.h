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
#include "opcuatms_client/objects/tms_client_context.h"
#include "opcuatms_client/objects/tms_client_signal_impl.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

inline SignalPtr TmsClientSignal(
    const ContextPtr& ctx,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const daq::opcua::tms::TmsClientContextPtr& clientContext,
    const opcua::OpcUaNodeId& nodeId)
{
    SignalPtr obj(createWithImplementation<ISignal, TmsClientSignalImpl>(ctx, parent, localId, clientContext, nodeId));
    return obj;
}

namespace details
{
    inline bool endsWith(std::string const& str, std::string const& suffix)
    {
        if (str.length() < suffix.length())
        {
            return false;
        }
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }
}

inline SignalPtr FindOrCreateTmsClientSignal(const ContextPtr& ctx,
                                             const ComponentPtr& parent,
                                             const daq::opcua::tms::TmsClientContextPtr& clientContext,
                                             const opcua::OpcUaNodeId& nodeId)
{
    SignalPtr clientSignal = clientContext->getObject(nodeId);
    if (!clientSignal.assigned())
    {
        auto localId = clientContext->getAttributeReader()->getValue(nodeId, UA_ATTRIBUTEID_BROWSENAME).toString();
        clientSignal = TmsClientSignal(ctx, parent, localId, clientContext, nodeId);

        // TODO current client implementation limitation: The order of populating signals is important.
        // The linked signal must be populated after the main signal; otherwise, the signal will have the wrong global ID.
        if (!details::endsWith(clientSignal.getGlobalId(), nodeId.getIdentifier()))
            ctx.getLogger()
                .getOrAddComponent("OpcUaTmsClient")
                .logMessage(SourceLocation(), "Wrong global ID of the signal on the client side (TODO)", LogLevel::Warn);
    }

    return clientSignal;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
