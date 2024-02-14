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
#include "opcuatms_client/objects/tms_client_component_impl.h"
#include "opendaq/device_impl.h"
#include "opendaq/device_ptr.h"
#include "opendaq/streaming_ptr.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsClientDeviceImpl : public TmsClientComponentBaseImpl<DeviceBase<ITmsClientComponent>>
{
public:
    explicit TmsClientDeviceImpl(const ContextPtr& ctx,
                                 const ComponentPtr& parent,
                                 const StringPtr& localId,
                                 const TmsClientContextPtr& clientContext,
                                 const opcua::OpcUaNodeId& nodeId,
                                 const FunctionPtr& createStreamingCallback,
                                 bool isRootDevice);

protected:
    void findAndCreateSubdevices();
    DevicePtr onAddDevice(const StringPtr& connectionString, const PropertyObjectPtr& config) override;
    void onRemoveDevice(const DevicePtr& device) override;
    DeviceInfoPtr onGetInfo() override;
    RatioPtr onGetResolution() override;
    uint64_t onGetTicksSinceOrigin() override;
    std::string onGetOrigin() override;
    UnitPtr onGetDomainUnit() override;
    void findAndCreateFunctionBlocks();
    void findAndCreateSignals();
    void findAndCreateInputsOutputs();
    void findAndCreateCustomComponents();
    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override;
    void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock) override;

    // Streaming related methods
    void findAndCreateStreamingOptions();
    void setUpStreamings();
    void connectToStreamings();

    DeviceInfoConfigPtr deviceInfo;

    // Streaming related members
    std::vector<StreamingPtr> streamings;
    FunctionPtr createStreamingCallback;
    bool isRootDevice;

private:
    void fetchTimeDomain();
    void fetchTicksSinceOrigin();

    bool timeDomainFetched = false;
    RatioPtr resolution;
    SizeT ticksSinceOrigin;
    StringPtr origin;
    UnitPtr domainUnit;
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
