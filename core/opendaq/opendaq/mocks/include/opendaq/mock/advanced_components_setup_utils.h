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
#include <opendaq/device_ptr.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/device_impl.h>
#include <opendaq/channel_impl.h>
#include <opendaq/server_impl.h>

namespace daq::test_utils
{
    DevicePtr createTestDevice(const std::string& localId = "root_dev");
    ComponentPtr createAdvancedPropertyComponent(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    PropertyObjectPtr createMockNestedPropertyObject();

    class MockFb1Impl final : public FunctionBlock
    {
    public:
        MockFb1Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
        DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
        FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override;
        void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock) override;
    };

    class MockFb2Impl final : public FunctionBlock
    {
    public:
        MockFb2Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    };

    class MockChannel1Impl final : public Channel
    {
    public:
        MockChannel1Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    };

    class MockChannel2Impl final : public Channel
    {
    public:
        MockChannel2Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    };

    class MockDevice1Impl final : public Device
    {
    public:
        MockDevice1Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
        DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
        FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override;
        void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock) override;
        DeviceInfoPtr onGetInfo() override;
        uint64_t onGetTicksSinceOrigin() override;
        DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;

    protected:
        bool clearFunctionBlocksOnUpdate() override
        {
            return false;
        }

    private:
        uint64_t ticksSinceOrigin;
    };

    class MockDevice2Impl final : public Device
    {
    public:
        MockDevice2Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);

        void addComponentHelper(const ComponentPtr& comp)
        {
            addExistingComponent(comp);
        }

        void removeComponentHelper(const std::string& id)
        {
            removeComponentById(id);
        }
        
        void setDeviceDomainHelper(const DeviceDomainPtr& deviceDomain)
        {
            setDeviceDomain(deviceDomain);
        }
        
        void setDeviceInfoHelper(const DeviceInfoPtr& deviceInfo)
        {
            this->deviceInfo = deviceInfo;
        }

    protected:
        bool clearFunctionBlocksOnUpdate() override
        {
            return false;
        }
        bool allowAddDevicesFromModules() override
        {
            return true;
        }

        DeviceInfoPtr onGetInfo() override
        {
            auto info = DeviceInfo("", this->localId);
            info.setLocation("loc");
            return info.detach();
        }

        ListPtr<IDeviceInfo> onGetAvailableDevices() override
        {
            auto availableDevices = List<IDeviceInfo>();

            for (int i = 0; i < 3; i++)
            {
                const auto num = std::to_string(i);
                const auto deviceInfo = DeviceInfo("mock://available_dev" + num, "AvailableMockDevice" + num);
                deviceInfo.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("manufacturer","Testing");
                availableDevices.pushBack(deviceInfo);
            }
            return availableDevices;
        }

        DictPtr<IString, IDevice> onAddDevices(const DictPtr<IString, IPropertyObject>& connectionArgs,
                                               DictPtr<IString, IInteger> errCodes,
                                               DictPtr<IString, IErrorInfo> errorInfos) override
        {
            auto addedDevices = Dict<IString, IDevice>();
            for (const auto& [connectionString, _] : connectionArgs)
            {
                if (connectionString == "mock://test")
                {
                    auto dev = createTestSubDevice();
                    this->devices.addItem(dev);
                    addedDevices[connectionString] = dev;
                    if (errCodes.assigned())
                        errCodes[connectionString] = OPENDAQ_SUCCESS;
                    if (errorInfos.assigned())
                        errorInfos[connectionString] = nullptr;
                }
                else
                {
                    addedDevices[connectionString] = nullptr;
                    ObjectPtr<IErrorInfo> errorInfo;
                    ErrCode errCode = DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);
                    daqGetErrorInfo(&errorInfo);
                    daqClearErrorInfo();

                    if (errCodes.assigned())
                        errCodes[connectionString] = errCode;
                    if (errorInfos.assigned())
                        errorInfos[connectionString] = errorInfo;
                }
            }
            return addedDevices;
        }

        DevicePtr onAddDevice(const StringPtr& connectionString, const PropertyObjectPtr& /*config*/ = nullptr) override
        {
            if (connectionString == "mock://test")
            {
                auto dev = createTestSubDevice();
                this->devices.addItem(dev);
                return dev;
            }
            throw NotFoundException();
        }

        void onRemoveDevice(const DevicePtr& device) override
        {
            devices.removeItem(device);
        }

    private:
        DevicePtr createTestSubDevice()
        {
            auto dev = createWithImplementation<IDevice, MockDevice2Impl>(this->context, this->devices, "newDevice");
            dev.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].connect(dev.getSignalsRecursive()[0]);
            return dev;
        }

        int callCnt = 0;
    };

    class MockSrvImpl final : public Server
    {
    public:
        MockSrvImpl(const ContextPtr& ctx, const DevicePtr& rootDev, const StringPtr& id);
    };

    class MockRecorderFb1Impl final : public FunctionBlockImpl<IFunctionBlock, IRecorder>
    {
    public:
        using Super = FunctionBlockImpl<IFunctionBlock, IRecorder>;

        MockRecorderFb1Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
        ErrCode INTERFACE_FUNC startRecording() override;
        ErrCode INTERFACE_FUNC stopRecording() override;
        ErrCode INTERFACE_FUNC getIsRecording(Bool* isRecording) override;
    private:
        bool isRecording{false};
    };
}
