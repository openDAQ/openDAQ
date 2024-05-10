/*
 * Copyright 2022-2024 Blueberry d.o.o.
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

namespace daq::config_protocol::test_utils
{
    DevicePtr createServerDevice();
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
    };

}
