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
#include <opcuatms_client/objects/tms_client_object_impl.h>
#include <opendaq/function_block_type_impl.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsClientFunctionBlockTypeImpl final : public TmsClientObjectImpl, public FunctionBlockTypeImpl
{
public:
    explicit TmsClientFunctionBlockTypeImpl(const ContextPtr& context,
                                            const TmsClientContextPtr& clientContext,
                                            const opcua::OpcUaNodeId& nodeId);
    // IFunctionBlockType
    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC createDefaultConfig(IPropertyObject** defaultConfig) override;

private:
    void readAttributes();

    FunctionBlockTypePtr type;
    PropertyObjectPtr defaultConfig;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
