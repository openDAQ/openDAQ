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
#include <coreobjects/eval_value_factory.h>
#include <opcuatms_server/objects/tms_server_variable.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// TmsServerEvalValue

class TmsServerEvalValue;
using TmsServerEvalValuePtr = std::shared_ptr<TmsServerEvalValue>;

class TmsServerEvalValue : public TmsServerVariable<EvalValuePtr>
{
public:
    using Super = TmsServerVariable<EvalValuePtr>;
    using ReadCallback = std::function<BaseObjectPtr()>;
    using WriteCallback = std::function<UA_StatusCode(const BaseObjectPtr& object)>;
    
    TmsServerEvalValue(const EvalValuePtr& object, const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext);
    TmsServerEvalValue(const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext);
    std::string getBrowseName() override;
    void setReadCallback(ReadCallback readCallback);
    void setWriteCallback(WriteCallback writeCallback);
    void setIsSelectionType(bool isSelection);

protected:
    opcua::OpcUaNodeId getTmsTypeId() override;
    void bindCallbacks() override;
    void configureVariableNodeAttributes(opcua::OpcUaObject<UA_VariableAttributes>& attr) override;

private:
    opcua::OpcUaVariant readEvaluationExpression();
    opcua::OpcUaVariant readRoot();
    UA_StatusCode writeRoot(const opcua::OpcUaVariant& variant);

    ReadCallback readCallback;
    WriteCallback writeCallback;

    bool isSelection = false;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
