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
#include "opcuatms/opcuatms.h"
#include "opcuatms_client/objects/tms_client_context.h"
#include "opcuatms_client/objects/tms_client_property_object_impl.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(INLINE_FACTORY,
                                        TmsClientPropertyObject,
                                        IPropertyObject,
                                        const daq::ContextPtr&,
                                        daqContext,
                                        const daq::opcua::tms::TmsClientContextPtr&,
                                        clientContext,
                                        const opcua::OpcUaNodeId&,
                                        nodeId)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INLINE_FACTORY,
                                       TmsClientPropertyObject,
                                       IPropertyObject,
                                       const daq::ContextPtr&,
                                       daqContext,
                                       const daq::opcua::tms::TmsClientContextPtr&,
                                       ctx,
                                       const opcua::OpcUaNodeId&,
                                       nodeId)

inline PropertyObjectPtr TmsClientPropertyObject(const ContextPtr& daqContext, const TmsClientContextPtr& ctx, const OpcUaNodeId& nodeId)
{
    auto obj(TmsClientPropertyObject_Create(daqContext, ctx, nodeId));
    return obj;
}


END_NAMESPACE_OPENDAQ_OPCUA_TMS
