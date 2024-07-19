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

#include "coretypes/coretype.h"
#include "coretypes/procedure.h"
#include "opcuashared/opcuanodeid.h"
#include "tms_client_context.h"
#include "opendaq/context_ptr.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsClientProcedureImpl : public ImplementationOf<ICoreType, IProcedure>
{
public:
    TmsClientProcedureImpl(const TmsClientContextPtr& ctx,
                          const ContextPtr& daqContexts,
                          const opcua::OpcUaNodeId& parentId,
                          const opcua::OpcUaNodeId& methodId);

    ErrCode INTERFACE_FUNC dispatch(IBaseObject* args) override;
    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override;

private:
    TmsClientContextPtr ctx;
    ContextPtr daqContext;
    opcua::OpcUaNodeId parentId;
    opcua::OpcUaNodeId methodId;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
