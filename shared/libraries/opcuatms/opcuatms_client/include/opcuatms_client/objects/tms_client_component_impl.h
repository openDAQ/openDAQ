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
#include "opcuatms_client/objects/tms_client_property_object_impl.h"
#include "opendaq/channel_impl.h"
#include "opcuatms_client/objects/tms_client_component.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

template <typename Impl>
class TmsClientComponentBaseImpl;

using TmsClientComponentImpl = TmsClientComponentBaseImpl<ComponentImpl<IComponent, ITmsClientComponent>>;

template <class Impl>
class TmsClientComponentBaseImpl : public TmsClientPropertyObjectBaseImpl<Impl>
{
public:
    
    template<class T = Impl, template_utils::enable_if_none<T, FunctionBlockImpl<IFunctionBlock, ITmsClientComponent>, ChannelImpl<ITmsClientComponent>> = 0>
    TmsClientComponentBaseImpl(const ContextPtr& ctx,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               const TmsClientContextPtr& clientContext,
                               const opcua::OpcUaNodeId& nodeId)
        : TmsClientPropertyObjectBaseImpl<Impl>(ctx, parent, localId, clientContext, nodeId)
    {
        initComponent();
        clientContext->readObjectAttributes(nodeId);
    }
    
    template<class T = Impl, template_utils::enable_if_any<T, FunctionBlockImpl<IFunctionBlock, ITmsClientComponent>, ChannelImpl<ITmsClientComponent>> = 0>
    TmsClientComponentBaseImpl(const ContextPtr& ctx,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               const TmsClientContextPtr& clientContext,
                               const opcua::OpcUaNodeId& nodeId,
                               const FunctionBlockTypePtr& type)
        : TmsClientPropertyObjectBaseImpl<Impl>(ctx, parent, localId, clientContext, nodeId, type)
    {
        initComponent();
        clientContext->readObjectAttributes(nodeId);
    }

    // Component overrides
    ErrCode INTERFACE_FUNC getActive(Bool* active) override;
    ErrCode INTERFACE_FUNC setActive(Bool active) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC setDescription(IString* description) override;
    ErrCode INTERFACE_FUNC getVisible(Bool* visible) override;
    ErrCode INTERFACE_FUNC setVisible(Bool visible) override;

    // ITmsClientComponent
    ErrCode INTERFACE_FUNC getRemoteGlobalId(IString** globalId) override;

protected:
    bool isChildComponent(const ComponentPtr& component);

private:
    LoggerComponentPtr getLoggerComponent();
    void initComponent();
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
