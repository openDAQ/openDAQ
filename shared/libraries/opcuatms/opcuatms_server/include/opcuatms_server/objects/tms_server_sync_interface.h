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
#include <opcuatms_server/objects/tms_server_property_object.h>
#include <open62541/daqdevice_nodeids.h>
#include <open62541/daqesp_nodeids.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsServerSyncInterfaceParameterPorts;
using TmsServerSyncInterfaceParameterPortsPtr = std::shared_ptr<TmsServerSyncInterfaceParameterPorts>;

class TmsServerSyncInterfaceParameterPorts : public TmsServerPropertyObject
{
public:
    using Super = TmsServerPropertyObject;
    using Super::Super;

    bool createOptionalNode(const opcua::OpcUaNodeId& nodeId) override
    {
        const auto name = server->readBrowseNameString(nodeId);

        if (name == "<Port>")
            return false;

        return Super::createOptionalNode(nodeId);
    }

protected:
    opcua::OpcUaNodeId getTmsTypeId() override
    {
        return OpcUaNodeId(NAMESPACE_DAQESP, UA_DAQESPID_PTPSYNCINTERFACETYPE_PARAMETERS_PORTS);
    }
};

class TmsServerSyncInterfaceParameter;
using TmsServerSyncInterfaceParameterPtr = std::shared_ptr<TmsServerSyncInterfaceParameter>;

class TmsServerSyncInterfaceParameter : public TmsServerPropertyObject
{
public:
    using Super = TmsServerPropertyObject;
    using Super::Super;

    void addChildNodes() override
    {
        uint32_t propNumber = 0;

        if (object.hasProperty("Ports"))
        {
            const auto prop = object.getProperty("Ports");
            const auto propName = prop.getName();
            const auto obj = object.getPropertyValue(propName);

            auto portsNodeId = getChildNodeId("Ports");
            auto ports = std::make_shared<TmsServerSyncInterfaceParameterPorts>(obj, server, daqContext, tmsContext, propName, prop);
            ports->setNumberInList(propNumber++);
            ports->registerToExistingOpcUaNode(portsNodeId);
            childObjects.insert({portsNodeId, ports});
        }

        if (ignoredProps.empty())
        {
            ignoredProps.emplace("Ports");
            ignoredProps.emplace("PtpConfigurationStructure");
        }
        Super::addChildNodes();
    }

    void bindCallbacks() override
    {
        this->addReadCallback("Configuration", [this] 
        { 
            return VariantConverter<IStruct>::ToVariant(this->object.getPropertyValue("PtpConfigurationStructure"), nullptr, daqContext); 
        });

        Super::bindCallbacks();
    }

protected:

    opcua::OpcUaNodeId getTmsTypeId() override
    {
        return OpcUaNodeId(NAMESPACE_DAQESP, UA_DAQESPID_PTPSYNCINTERFACETYPE_PARAMETERS);
    }
};

class TmsServerSyncInterface;
using TmsServerSyncInterfacePtr = std::shared_ptr<TmsServerSyncInterface>;

class TmsServerSyncInterface : public TmsServerPropertyObject
{
public:
    using Super = TmsServerPropertyObject;
    using Super::Super;

    void addChildNodes() override
    {
        ignoredProps.clear();
        uint32_t propNumber = object.getAllProperties().getCount();

        if (object.hasProperty("Status"))
        {
            ignoredProps.emplace("Status");
            propNumber -= 1;
        }
        if (name == "PtpSyncInterface" && object.hasProperty("Parameters"))
        {
            ignoredProps.emplace("Parameters");
            propNumber -= 1;
        }

        if (object.hasProperty("Status"))
        {
            const auto prop = object.getProperty("Status");
            const auto propName = prop.getName();
            const auto obj = object.getPropertyValue(propName);
            
            auto statusNodeId = getChildNodeId("Status");
            auto status = std::make_shared<TmsServerPropertyObject>(obj, server, daqContext, tmsContext, propName, prop);
            status->setNumberInList(propNumber++);
            status->registerToExistingOpcUaNode(statusNodeId);
            childObjects.insert({statusNodeId, status});
        }

        if (name == "PtpSyncInterface" && object.hasProperty("Parameters"))
        {
            const auto prop = object.getProperty("Parameters");
            const auto propName = prop.getName();
            const auto obj = object.getPropertyValue(propName);

            auto parametersNodeId = getChildNodeId("Parameters");
            auto parameters = std::make_shared<TmsServerSyncInterfaceParameter>(obj, server, daqContext, tmsContext, propName, prop);
            parameters->setNumberInList(propNumber++);
            parameters->registerToExistingOpcUaNode(parametersNodeId);
            childObjects.insert({parametersNodeId, parameters});
        }

        Super::addChildNodes();
    }

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
        ignoredProps.clear();
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
