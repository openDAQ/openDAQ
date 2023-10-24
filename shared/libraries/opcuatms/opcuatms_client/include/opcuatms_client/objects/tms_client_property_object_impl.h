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
#include "coreobjects/property_object_impl.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuatms/opcuatms.h"
#include "opcuatms_client/objects/tms_client_object_impl.h"
#include "opcuaclient/reference_utils.h"
#include "opendaq/channel_impl.h"
#include "opendaq/streaming_info_impl.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

namespace detail
{
    template <class T, class... Ts>
    using enable_if_any = std::enable_if_t<(std::is_same_v<T, Ts> || ...), int>;

    template <class T, class... Ts>
    using enable_if_none = std::enable_if_t<!(std::is_same_v<T, Ts> || ...), int>;
}

template <typename Impl>
class TmsClientPropertyObjectBaseImpl;

using TmsClientPropertyObjectImpl = TmsClientPropertyObjectBaseImpl<PropertyObjectImpl>;

template <class Impl>
class TmsClientPropertyObjectBaseImpl : public TmsClientObjectImpl, public Impl
{
public:
    
    template<class T = Impl, detail::enable_if_any<T, PropertyObjectImpl> = 0>
    TmsClientPropertyObjectBaseImpl(const ContextPtr& daqContext, const TmsClientContextPtr& clientContext, const opcua::OpcUaNodeId& nodeId)
        : TmsClientObjectImpl(daqContext, clientContext, nodeId)
        , Impl()
        , referenceUtils(client)
    {
        browseRawProperties();
    }

    template<class T = Impl, detail::enable_if_any<T, StreamingInfoConfigImpl> = 0>
    TmsClientPropertyObjectBaseImpl(const ContextPtr& daqContext,
                                    const StringPtr& protocolId,
                                    const TmsClientContextPtr& clientContext,
                                    const opcua::OpcUaNodeId& nodeId)
        : TmsClientObjectImpl(daqContext, clientContext, nodeId)
        , Impl(protocolId)
        , referenceUtils(client)
    {
        browseRawProperties();
    }

    template<class T = Impl, detail::enable_if_none<T, FunctionBlock, Channel, PropertyObjectImpl, StreamingInfoConfigImpl> = 0>
    TmsClientPropertyObjectBaseImpl(const ContextPtr& ctx,
                                    const ComponentPtr& parent,
                                    const StringPtr& localId,
                                    const TmsClientContextPtr& clientContext,
                                    const opcua::OpcUaNodeId& nodeId)
        : TmsClientObjectImpl(ctx, clientContext, nodeId)
        , Impl(ctx, parent, localId)
        , referenceUtils(client)
    {
        browseRawProperties();
    }
    
    template<class T = Impl, detail::enable_if_any<T, FunctionBlock, Channel> = 0>
    TmsClientPropertyObjectBaseImpl(const ContextPtr& ctx,
                                    const ComponentPtr& parent,
                                    const StringPtr& localId,
                                    const TmsClientContextPtr& clientContext,
                                    const opcua::OpcUaNodeId& nodeId,
                                    const FunctionBlockTypePtr& type)
        : TmsClientObjectImpl(ctx, clientContext, nodeId)
        , Impl(type, ctx, parent, localId)
        , referenceUtils(client)
    {
        browseRawProperties();
    }

    ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setProtectedPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC getPropertyValue(IString* propertyName, IBaseObject** value) override;
    ErrCode INTERFACE_FUNC getPropertySelectionValue(IString* propertyName, IBaseObject** value) override;
    ErrCode INTERFACE_FUNC clearPropertyValue(IString* propertyName) override;
    ErrCode INTERFACE_FUNC getProperty(IString* propertyName, IProperty** value) override;
    ErrCode INTERFACE_FUNC addProperty(IProperty* property) override;
    ErrCode INTERFACE_FUNC removeProperty(IString* propertyName) override;
    ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IString* propertyName, IEvent** event) override;
    ErrCode INTERFACE_FUNC getOnPropertyValueRead(IString* propertyName, IEvent** event) override;
    ErrCode INTERFACE_FUNC getVisibleProperties(IList** properties) override;
    ErrCode INTERFACE_FUNC hasProperty(IString* propertyName, Bool* hasProperty) override;
    ErrCode INTERFACE_FUNC getAllProperties(IList** properties) override;
    ErrCode INTERFACE_FUNC setPropertyOrder(IList* orderedPropertyNames) override;

protected:
    opcua::ReferenceUtils referenceUtils;
    std::unordered_map<std::string, opcua::OpcUaNodeId> introspectionVariableIdMap;
    std::unordered_map<std::string, opcua::OpcUaNodeId> referenceVariableIdMap;
    std::unordered_map<std::string, opcua::OpcUaNodeId> objectTypeIdMap;
    opcua::OpcUaNodeId methodParentNodeId;

    void addProperties(const tsl::ordered_map<opcua::OpcUaNodeId, opcua::OpcUaObject<UA_ReferenceDescription>>& references,
                       std::map<uint32_t, PropertyPtr>& orderedProperties,
                       std::vector<PropertyPtr> unorderedProperties);
    void addMethodProperties(const tsl::ordered_map<opcua::OpcUaNodeId, opcua::OpcUaObject<UA_ReferenceDescription>>& references,
                             const opcua::OpcUaNodeId& parentNodeId);
    void browseRawProperties();
    ErrCode INTERFACE_FUNC setPropertyValueInternal(IString* propertyName, IBaseObject* value, bool protectedWrite);
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
