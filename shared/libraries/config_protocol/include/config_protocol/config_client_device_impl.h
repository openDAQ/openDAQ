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
#include <opendaq/device_impl.h>
#include <config_protocol/config_client_component_impl.h>
#include <config_protocol/component_holder_ptr.h>

namespace daq::config_protocol
{

class ConfigClientDeviceImpl : public ConfigClientComponentBaseImpl<DeviceBase<IConfigClientObject>>
{
public:
    using Super = ConfigClientComponentBaseImpl<DeviceBase<IConfigClientObject>>;

    explicit ConfigClientDeviceImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                    const std::string& remoteGlobalId,
                                    const ContextPtr& ctx,
                                    const ComponentPtr& parent,
                                    const StringPtr& localId);

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;

private:
    void componentAdded(const CoreEventArgsPtr& args);
    void componentRemoved(const CoreEventArgsPtr& args);
};

inline ConfigClientDeviceImpl::ConfigClientDeviceImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                      const std::string& remoteGlobalId,
                                                      const ContextPtr& ctx,
                                                      const ComponentPtr& parent,
                                                      const StringPtr& localId)
    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId)
{
}

inline DictPtr<IString, IFunctionBlockType> ConfigClientDeviceImpl::onGetAvailableFunctionBlockTypes()
{
    return clientComm->sendComponentCommand(remoteGlobalId, "GetAvailableFunctionBlockTypes");
}

inline FunctionBlockPtr ConfigClientDeviceImpl::onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)
{
    auto params = Dict<IString, IBaseObject>({{"TypeId", typeId}, {"Config", config}});
    const ComponentHolderPtr fbHolder = clientComm->sendComponentCommand(remoteGlobalId, "AddFunctionBlock", params, functionBlocks);

    const DevicePtr thisPtr = this->borrowPtr<DevicePtr>();

    FunctionBlockPtr fb = fbHolder.getComponent();
    if (!functionBlocks.hasItem(fb.getLocalId()))
    {
        addNestedFunctionBlock(fb);
        return fb;
    }

    return functionBlocks.getItem(fb.getLocalId());
}

inline ErrCode ConfigClientDeviceImpl::Deserialize(ISerializedObject* serialized,
                                                   IBaseObject* context,
                                                   IFunction* factoryCallback,
                                                   IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry([&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = DeserializeConfigComponent<IDevice, ConfigClientDeviceImpl>(serialized, context, factoryCallback).detach();
        });
}

inline void ConfigClientDeviceImpl::handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args)
{
    switch (static_cast<CoreEventId>(args.getEventId()))
    {
        case CoreEventId::ComponentAdded:
            componentAdded(args);
            break;
        case CoreEventId::ComponentRemoved:
            componentRemoved(args);
            break;
        case CoreEventId::PropertyValueChanged:
        case CoreEventId::PropertyObjectUpdateEnd:
        case CoreEventId::PropertyAdded:
        case CoreEventId::PropertyRemoved:
        case CoreEventId::SignalConnected:
        case CoreEventId::SignalDisconnected:
        case CoreEventId::DataDescriptorChanged:
        case CoreEventId::ComponentUpdateEnd:
        case CoreEventId::AttributeChanged:
        case CoreEventId::TagsChanged:
            break;
    }

    ConfigClientComponentBaseImpl::handleRemoteCoreObjectInternal(sender, args);
}

inline void ConfigClientDeviceImpl::componentAdded(const CoreEventArgsPtr& args)
{
    const ComponentPtr comp = args.getParameters().get("Component");
    Bool hasItem{false};
    checkErrorInfo(DeviceBase<IConfigClientObject>::hasItem(comp.getLocalId(), &hasItem));
    if (!hasItem)
        addExistingComponent(comp);
}


inline void ConfigClientDeviceImpl::componentRemoved(const CoreEventArgsPtr& args)
{
    const StringPtr id = args.getParameters().get("Id");
    Bool hasItem{false};
    checkErrorInfo(DeviceBase<IConfigClientObject>::hasItem(id, &hasItem));
    if (hasItem)
        removeComponentById(id);
}
}
