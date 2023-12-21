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
#include <opendaq/device_ptr.h>
#include "opcuatms/opcuatms.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuatms_client/objects/tms_client_device_factory.h"
#include "opcuatms_client/objects/tms_client_context.h"
#include <opendaq/context_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

class TmsClient final
{
public:
    TmsClient(const ContextPtr& context,
              const ComponentPtr& parent,
              const std::string& opcUaUrl,
              const FunctionPtr& createStreamingCallback);

    daq::DevicePtr connect();

protected:
    void getRootDeviceNodeAttributes(OpcUaNodeId& nodeIdOut, std::string& browseNameOut);

    tms::TmsClientContextPtr tmsClientContext;
    ContextPtr context;
    daq::opcua::OpcUaClientPtr client;
    std::string opcUaUrl;
    FunctionPtr createStreamingCallback;
    ComponentPtr parent;

private:
    StringPtr getUniqueLocalId(const StringPtr& localId, int iteration = 0);
};

END_NAMESPACE_OPENDAQ_OPCUA
