/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <coretypes/constexpr_utils.h>
#include <opendaq/component_ptr.h>
#include <opendaq/signal_ptr.h>
#include "opcuaserver/node_event_manager.h"
#include "opcuaserver/opcuaserver.h"
#include "opcuatms/opcuatms.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

template <class CoreType, typename Enable = void>
struct RequestedNodeId
{
    opcua::OpcUaNodeId operator()(const CoreType& object)
    {
        return {};
    }
};

template <class CoreType>
struct RequestedNodeId<CoreType, std::enable_if_t<IsDerivedFromTemplate<CoreType, GenericComponentPtr>::Value>>
{
    opcua::OpcUaNodeId operator()(const CoreType& object)
    {
        return opcua::OpcUaNodeId(NAMESPACE_DAQBSP, object.getGlobalId().toStdString());
    }
};

class TmsServerObject;
using TmsServerObjectPtr = std::shared_ptr<TmsServerObject>;

class TmsServerContext;
using TmsServerContextPtr = std::shared_ptr<TmsServerContext>;

class TmsServerObject : public std::enable_shared_from_this<TmsServerObject>
{
public:
    using ReadVariantCallback = std::function<opcua::OpcUaVariant()>;
    using WriteVariantCallback = std::function<UA_StatusCode(const opcua::OpcUaVariant& variant)>;

    TmsServerObject(const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext);
    virtual ~TmsServerObject();

    virtual std::string getBrowseName();
    virtual std::string getDisplayName();
    virtual std::string getDescription();
    opcua::NodeEventManagerPtr addEvent(const StringPtr& nodeName);
    opcua::NodeEventManagerPtr addEvent(const opcua::OpcUaNodeId& nodeId);
    opcua::OpcUaNodeId registerOpcUaNode(
        const opcua::OpcUaNodeId& parentNodeId = opcua::OpcUaNodeId(UA_NS0ID_OBJECTSFOLDER));
    opcua::OpcUaNodeId registerToExistingOpcUaNode(const opcua::OpcUaNodeId& nodeId);
    opcua::OpcUaNodeId getNodeId();
    void setNumberInList(uint32_t numberInList);

    void addHierarchicalReference(const opcua::OpcUaNodeId& parent);
    virtual void createNonhierarchicalReferences();
    virtual void onCoreEvent(const CoreEventArgsPtr& eventArgs);

protected:
    virtual void validate();
    virtual opcua::OpcUaNodeId getRequestedNodeId();
    virtual opcua::OpcUaNodeId getReferenceType();
    virtual opcua::OpcUaNodeId getTmsTypeId() = 0;
    virtual BaseObjectPtr getObject() = 0;
    virtual opcua::OpcUaNodeId createNode(const opcua::OpcUaNodeId& parentNodeId);
    virtual void addChildNodes();
    virtual void bindCallbacks();
    virtual void registerToTmsServerContext();
    virtual int64_t getCurrentClock();
    std::string readTypeBrowseName();
    virtual bool createOptionalNode(const opcua::OpcUaNodeId& nodeId);
    virtual void configureNodeAttributes(opcua::OpcUaObject<UA_ObjectAttributes>& attr);
    void addReadCallback(const std::string& nodeName, ReadVariantCallback readFunc);
    void addWriteCallback(const std::string& nodeName, WriteVariantCallback writeFunc);
    void addReadCallback(const opcua::OpcUaNodeId& nodeId, ReadVariantCallback readFunc);
    void addWriteCallback(const opcua::OpcUaNodeId& nodeId, WriteVariantCallback writeFunc);
    void addReference(const opcua::OpcUaNodeId& targetNodeId, const opcua::OpcUaNodeId& referenceTypeId);
    void bindReadWriteCallbacks();
    void browseReferences();
    bool hasChildNode(const std::string& nodeName) const;
    opcua::OpcUaNodeId getChildNodeId(const std::string& nodeName);
    opcua::OpcUaNodeId findSignalNodeId(const SignalPtr& signal) const;

    template <class C>
    opcua::OpcUaNodeId findTmsObjectNodeId(const C& signal) const
    {
        auto nodeId = RequestedNodeId<C>{}(signal);
        return server->nodeExists(nodeId) ? nodeId : opcua::OpcUaNodeId();
    }

    template <class C>
    void createChildNonhierarchicalReferences(const C& container)
    {
        for (const auto& item : container)
            item->createNonhierarchicalReferences();
    }

    // TODO: NumberInList is configured only when object is registered. Order of nodes on objects of which child nodes
    // reference other ones will not be correct!
    template <class TMS_T, class DAQ_T, class... Params>
    std::shared_ptr<TMS_T> registerTmsObjectOrAddReference(const opcua::OpcUaNodeId& parentNodeId,
                                                           const DAQ_T& daqObject,
                                                           uint32_t numberInList,
                                                           Params... params)
    {
        auto tmsObjectNodeId = findTmsObjectNodeId(daqObject);
        if (!tmsObjectNodeId.isNull())
        {
            void* nodeContext = {};
            opcua::CheckStatusCodeException(UA_Server_getNodeContext(this->server->getUaServer(), *tmsObjectNodeId, &nodeContext));
            auto tmsObject = std::dynamic_pointer_cast<TMS_T>(static_cast<TMS_T*>(nodeContext)->shared_from_this());
            tmsObject->addHierarchicalReference(parentNodeId);
            return tmsObject;
        }
        else
        {
            auto tmsObject = std::make_shared<TMS_T>(daqObject, this->server, daqContext, tmsContext, std::forward<Params>(params)...);
            tmsObject->registerOpcUaNode(parentNodeId);
            if(numberInList != std::numeric_limits<uint32_t>::max())
                tmsObject->setNumberInList(numberInList);
            return tmsObject;
        }
    }

    opcua::OpcUaServerPtr server;
    std::string typeBrowseName;
    std::mutex valueMutex;
    opcua::OpcUaNodeId nodeId;
    ContextPtr daqContext;
    uint32_t numberInList;
    TmsServerContextPtr tmsContext;

private:
    void bindCallbacksInternal();

    std::unordered_map<opcua::OpcUaNodeId, ReadVariantCallback> readCallbacks;
    std::unordered_map<opcua::OpcUaNodeId, WriteVariantCallback> writeCallbacks;
    std::unordered_map<std::string, opcua::OpcUaObject<UA_ReferenceDescription>> references;
    std::unordered_map<opcua::OpcUaNodeId, opcua::NodeEventManagerPtr> eventManagers;
};

template <class CoreType>
class TmsServerObjectBaseImpl : public TmsServerObject
{
public:
    TmsServerObjectBaseImpl(const BaseObjectPtr& object, const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext)
        : TmsServerObject(server, context, tmsContext)
        , object(object)
    {
    }

protected:
    BaseObjectPtr getObject() override
    {
        return object;
    }

    opcua::OpcUaNodeId getRequestedNodeId() override
    {
        return RequestedNodeId<CoreType>{}(object);
    }

    CoreType object;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
