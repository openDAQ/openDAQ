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
#include <opendaq/signal_ptr.h>
#include <opcuatms_server/objects/tms_server_variable.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// TmsServerValue

class TmsServerValue;
using TmsServerValuePtr = std::shared_ptr<TmsServerValue>;

class TmsServerValue : public TmsServerVariable<BaseObjectPtr>
{
public:
    using Super = TmsServerVariable<BaseObjectPtr>;

    TmsServerValue(const SignalPtr& signal, const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext);
    std::string getBrowseName() override;

    static opcua::OpcUaNodeId SampleTypeToOpcUaDataType(SampleType sampleType);

protected:
    opcua::OpcUaNodeId getTmsTypeId() override;
    opcua::OpcUaNodeId getDataTypeId() override;
    void addChildNodes() override;
    void bindCallbacks() override;

private:
    SignalPtr signal;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS

