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
#include <opendaq/device_ptr.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/device_impl.h>
#include <opendaq/channel_impl.h>
#include <opendaq/server_impl.h>
#include "opendaq/device_type_factory.h"

namespace daq::config_protocol::test_utils
{
    DevicePtr createTestDevice(const std::string& localId = "root_dev");
    ComponentPtr createAdvancedPropertyComponent(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);

    class MockFb1Impl final : public FunctionBlock
    {
    public:
        MockFb1Impl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
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
            if (!this->deviceInfo.isFrozen())
                this->deviceInfo.freeze();
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
            info.freeze();
            return info.detach();
        }

        ListPtr<IDeviceInfo> onGetAvailableDevices() override
        {
            auto availableDevices = List<IDeviceInfo>();

            for (int i = 0; i < 3; i++)
            {
                const auto num = std::to_string(i);
                const auto deviceInfo = DeviceInfo("mock://available_dev" + num, "AvailableMockDevice" + num);
                deviceInfo.asPtr<IDeviceInfoConfig>().setManufacturer("Testing");
                deviceInfo.freeze();
                availableDevices.pushBack(deviceInfo);
            }
            return availableDevices;
        }

        DevicePtr onAddDevice(const StringPtr& connectionString, const PropertyObjectPtr& config = nullptr) override
        {
            if (connectionString == "mock://test")
            {
                auto dev = createWithImplementation<IDevice, MockDevice2Impl>(this->context, this->devices, "newDevice");

                dev.getDevices()[0].getFunctionBlocks()[0].getInputPorts()[0].connect(dev.getSignalsRecursive()[0]);

                this->devices.addItem(dev);

                return dev;
            }
            return nullptr;
        }

        void onRemoveDevice(const DevicePtr& device) override
        {
            devices.removeItem(device);
        }

        DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override
        {
            auto devTypes =
                Dict<IString, IDeviceType>({{"mockDev1", DeviceType("mockDev1", "MockDev1", "Mock Device 1", "prefix", nullptr)}});
            return devTypes;
        }

    };

    class MockSrvImpl final : public Server
    {
    public:
        MockSrvImpl(const ContextPtr& ctx, const DevicePtr& rootDev, const StringPtr& id);
    };
}
