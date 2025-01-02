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
#include <opcuatms_server/objects/tms_server_property_object.h>
#include <open62541/daqdevice_nodeids.h>
#include <open62541/daqesp_nodeids.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsServerSyncInterface;
using TmsServerSyncInterfacePtr = std::shared_ptr<TmsServerSyncInterface>;

class TmsServerSyncInterface : public TmsServerPropertyObject
{
public:
    using Super = TmsServerPropertyObject;
    using Super::Super;

protected:
    opcua::OpcUaNodeId getTmsTypeId() override
    {
        if (name == "InterfaceClockSync")
            return OpcUaNodeId(NAMESPACE_DAQESP, UA_DAQESPID_INTERNALCLOCKSYNCINTERFACETYPE);

        if (name == "PtpSyncInterface")
            return OpcUaNodeId(NAMESPACE_DAQESP, UA_DAQESPID_PTPSYNCINTERFACETYPE);
        
        return Super::getTmsTypeId();
    }
};

class TmsServerSyncInterfaces;
using TmsServerSyncInterfacesPtr = std::shared_ptr<TmsServerSyncInterfaces>;

class TmsServerSyncInterfaces : public TmsServerPropertyObject
{
public:
    using Super = TmsServerPropertyObject;
    using Super::Super;
    
    bool createOptionalNode(const opcua::OpcUaNodeId& nodeId) override
    {
        const auto name = server->readBrowseNameString(nodeId);

        if (name == "<Interface>")
            return false;

        return Super::createOptionalNode(nodeId);
    }

    void addChildNodes() override
    {
        uint32_t propNumber = 0;

        for (const auto& prop : object.getAllProperties())
        {
            const auto propName = prop.getName();
            ignoredProps.emplace(propName);
            PropertyObjectPtr obj = object.getPropertyValue(propName);
            auto serverInfo = registerTmsObjectOrAddReference<TmsServerSyncInterface>(nodeId, obj, propNumber++, propName, prop);
            auto childNodeId = serverInfo->getNodeId();
            childObjects.insert({childNodeId, serverInfo});
        }

        Super::addChildNodes();
    }
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
