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
#include <opendaq/function_block_type.h>
#include <opcuatms_server/objects/tms_server_object.h>
#include <opendaq/function_block_type_ptr.h>
#include <opcuatms_server/objects/tms_server_property_object.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsServerFunctionBlockType;
using TmsServerFunctionBlockTypePtr = std::shared_ptr<TmsServerFunctionBlockType>;

class TmsServerFunctionBlockType : public TmsServerVariable<FunctionBlockTypePtr>
{
public:
    using Super = TmsServerVariable<FunctionBlockTypePtr>;

    TmsServerFunctionBlockType(const FunctionBlockTypePtr& object,
                               const OpcUaServerPtr& server,
                               const ContextPtr& context,
                               const TmsServerContextPtr& tmsContext);

    std::string getBrowseName() override;
    std::string getDisplayName() override;
    std::string getDescription() override;

protected:
    OpcUaNodeId getTmsTypeId() override;
    void addChildNodes() override;
    void configureVariableNodeAttributes(OpcUaObject<UA_VariableAttributes>& attr) override;

private:
    void addIdNode();
    void addNameNode();
    void addDescriptionNode();
    void addDefaultConfigNode();

    TmsServerPropertyObjectPtr tmsDefaultConfig;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
