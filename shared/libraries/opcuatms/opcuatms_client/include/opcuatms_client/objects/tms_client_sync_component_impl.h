/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <opcuatms_client/objects/tms_client_component_impl.h>
#include <opendaq/sync_component_impl.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsClientSyncComponentImpl : public TmsClientComponentBaseImpl<GenericSyncComponentImpl<ISyncComponent, ITmsClientComponent>>
{
public:

    using Impl = GenericSyncComponentImpl<ISyncComponent, ITmsClientComponent>;
    using Super = TmsClientComponentBaseImpl<Impl>;

    TmsClientSyncComponentImpl(const ContextPtr& ctx,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               const TmsClientContextPtr& clientContext,
                               const opcua::OpcUaNodeId& nodeId)
        : Super(ctx, parent, localId, clientContext, nodeId)
    {
        StringPtr propertyName = "Interfaces";
        BaseObjectPtr interfacesValue;
        checkErrorInfo(getPropertyValue(propertyName, &interfacesValue));
        checkErrorInfo(Impl::setProtectedPropertyValue(propertyName, interfacesValue));
    }

    ErrCode INTERFACE_FUNC getSyncLocked(Bool* synchronizationLocked) override
    {
        try
        {
            const auto syncLockNodeId = clientContext->getReferenceBrowser()->getChildNodeId(nodeId, "SynchronizationLocked");
            OpcUaVariant opcUaVariant = client->readValue(*syncLockNodeId);
            if (!opcUaVariant.isNull())
            {
                BooleanPtr syncLockPtr = VariantConverter<IBoolean>::ToDaqObject(opcUaVariant);
                return syncLockPtr->getValue(synchronizationLocked);
            }
        }
        catch (...)
        {
            LOG_W("Failed to get sync locked on OpcUA client sync component \"{}\"", this->globalId);
        }
        return OPENDAQ_SUCCESS;
    }
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
