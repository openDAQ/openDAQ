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
#include <opendaq/sync_component_ptr.h>
#include "opcuatms_server/objects/tms_server_sync_interface.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsServerSyncComponent;
using TmsServerSyncComponentPtr = std::shared_ptr<TmsServerSyncComponent>;

class TmsServerSyncComponent : public TmsServerComponent<SyncComponentPtr>
{
public:
    using Super = TmsServerComponent<SyncComponentPtr>;

    TmsServerSyncComponent(const SyncComponentPtr& object, const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext);

    void addChildNodes() override;
    void bindCallbacks() override;

protected:
    opcua::OpcUaNodeId getTmsTypeId() override;
    void bindPropertyCallbacks(const std::string& name);
    void triggerEvent(PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args);

    TmsServerSyncInterfacesPtr interfaces;
    TmsServerPropertyPtr source;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
