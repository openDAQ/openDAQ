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
#include <opcuatms_client/objects/tms_client_component_impl.h>
#include <opendaq/mirrored_device_impl.h>
#include <opendaq/device_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsClientDeviceImpl : public TmsClientComponentBaseImpl<MirroredDeviceBase<ITmsClientComponent>>
{
public:
    using Impl = MirroredDeviceBase<ITmsClientComponent>;
    using Super = TmsClientComponentBaseImpl<MirroredDeviceBase<ITmsClientComponent>>;
    explicit TmsClientDeviceImpl(const ContextPtr& ctx,
                                 const ComponentPtr& parent,
                                 const StringPtr& localId,
                                 const TmsClientContextPtr& clientContext,
                                 const opcua::OpcUaNodeId& nodeId,
                                 bool isRootDevice);
    
    ErrCode INTERFACE_FUNC getDomain(IDeviceDomain** deviceDomain) override;
    ErrCode INTERFACE_FUNC getAvailableOperationModes(IList** availableOpModes) override;
    ErrCode INTERFACE_FUNC setOperationMode(OperationModeType modeType) override;
    ErrCode INTERFACE_FUNC setOperationModeRecursive(OperationModeType modeType) override;
    ErrCode INTERFACE_FUNC getOperationMode(OperationModeType* modeType) override;

protected:
    void findAndCreateSubdevices();
    DevicePtr onAddDevice(const StringPtr& connectionString, const PropertyObjectPtr& config) override;
    DictPtr<IString, IDevice> onAddDevices(const DictPtr<IString, IPropertyObject>& connectionArgs,
                                           DictPtr<IString, IInteger> errCodes,
                                           DictPtr<IString, IErrorInfo> errorInfos) override;
    void onRemoveDevice(const DevicePtr& device) override;
    DeviceInfoPtr onGetInfo() override;
    uint64_t onGetTicksSinceOrigin() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    PropertyObjectPtr onCreateDefaultAddDeviceConfig() override;
    void findAndCreateFunctionBlocks();
    void findAndCreateSignals();
    void findAndCreateInputsOutputs();
    void findAndCreateCustomComponents();
    void findAndCreateSyncComponent();
    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override;
    void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock) override;
    ListPtr<ILogFileInfo> onGetLogFileInfos() override;
    StringPtr onGetLog(const StringPtr& id, Int size, Int offset) override;

    void findAndCreateServerCapabilities(const DeviceInfoPtr& deviceInfo);

    void removed() override;
    bool isAddedToLocalComponentTree() override;

private:
    void fetchTimeDomain();
    void fetchTicksSinceOrigin();

    SizeT ticksSinceOrigin{};
    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
    std::unordered_map<std::string, opcua::OpcUaNodeId> deviceInfoChangeableFields;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
