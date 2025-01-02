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
#include <opendaq/channel.h>
#include <opendaq/device.h>
#include <opendaq/device_impl.h>

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(IMockDefaultDevice, IBaseObject)
{
    virtual void addCustomComponent(const ComponentPtr& component) = 0;
};

class DefaultDevice : public DeviceBase<IDevice, IMockDefaultDevice>
{
public:

    DefaultDevice(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : DeviceBase<IDevice, IMockDefaultDevice>(ctx, parent, localId)
    {
    }

    DeviceInfoPtr onGetInfo() override
    {
        deviceInfo = DeviceInfo("", "default_dev");
        deviceInfo.freeze();
        return deviceInfo;
    }

    void addCustomComponent(const ComponentPtr& component) override
    {
        this->components.emplace_back(component);
    }
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, DefaultChannel, IChannel,
    IFunctionBlockType*, fbType,
    IContext*, context,
    IComponent*, parent,
    IString*, localId
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, DefaultFunctionBlock, IFunctionBlock,
    IFunctionBlockType*, fbType,
    IContext*, context,
    IComponent*, parent,
    IString*, localId
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, DefaultDevice, IDevice,
    IContext*, context,
    IComponent*, parent,
    IString*, localId
)

END_NAMESPACE_OPENDAQ
