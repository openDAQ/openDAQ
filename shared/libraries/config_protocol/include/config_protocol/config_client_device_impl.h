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
#include <opendaq/component_holder_ptr.h>

namespace daq::config_protocol
{

template <typename... Interfaces>
using ConfigClientDeviceBase = DeviceBase<IConfigClientObject, Interfaces...>;

using ConfigClientDevice = ConfigClientDeviceBase<>;

template <class TDeviceBase>
class GenericConfigClientDeviceImpl;

using ConfigClientDeviceImpl = GenericConfigClientDeviceImpl<ConfigClientDevice>;

template <class TDeviceBase>
class GenericConfigClientDeviceImpl : public ConfigClientComponentBaseImpl<TDeviceBase>
{
public:
    using Super = ConfigClientComponentBaseImpl<TDeviceBase>;

    explicit GenericConfigClientDeviceImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                           const std::string& remoteGlobalId,
                                           const ContextPtr& ctx,
                                           const ComponentPtr& parent,
                                           const StringPtr& localId);

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override;
    void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock) override;
    uint64_t onGetTicksSinceOrigin() override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;

private:
    void componentAdded(const CoreEventArgsPtr& args);
    void componentRemoved(const CoreEventArgsPtr& args);
};

template <class TDeviceBase>
GenericConfigClientDeviceImpl<TDeviceBase>::GenericConfigClientDeviceImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                                          const std::string& remoteGlobalId,
                                                                          const ContextPtr& ctx,
                                                                          const ComponentPtr& parent,
                                                                          const StringPtr& localId)
    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId)
{
}

template <class TDeviceBase>
DictPtr<IString, IFunctionBlockType> GenericConfigClientDeviceImpl<TDeviceBase>::onGetAvailableFunctionBlockTypes()
{
    return this->clientComm->sendComponentCommand(this->remoteGlobalId, "GetAvailableFunctionBlockTypes");
}

template <class TDeviceBase>
FunctionBlockPtr GenericConfigClientDeviceImpl<TDeviceBase>::onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)
{
    auto params = Dict<IString, IBaseObject>({{"TypeId", typeId}, {"Config", config}});
    const ComponentHolderPtr fbHolder = this->clientComm->sendComponentCommand(this->remoteGlobalId, "AddFunctionBlock", params, this->functionBlocks);

    const DevicePtr thisPtr = this->template borrowPtr<DevicePtr>();

    FunctionBlockPtr fb = fbHolder.getComponent();
    if (!this->functionBlocks.hasItem(fb.getLocalId()))
    {
        this->clientComm->connectDomainSignals(fb);
        this->addNestedFunctionBlock(fb);
        this->clientComm->connectInputPorts(fb);
        return fb;
    }
    return this->functionBlocks.getItem(fb.getLocalId());
}

template <class TDeviceBase>
void GenericConfigClientDeviceImpl<TDeviceBase>::onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock)
{
    if (!functionBlock.assigned())
        throw InvalidParameterException();

    auto params = Dict<IString, IBaseObject>({{"LocalId", functionBlock.getLocalId()}});
    this->clientComm->sendComponentCommand(this->remoteGlobalId, "RemoveFunctionBlock", params);

    const DevicePtr thisPtr = this->template borrowPtr<DevicePtr>();

    if (this->functionBlocks.hasItem(functionBlock.getLocalId()))
    {
        this->removeNestedFunctionBlock(functionBlock);
    }
}

template <class TDeviceBase>
uint64_t GenericConfigClientDeviceImpl<TDeviceBase>::onGetTicksSinceOrigin()
{
    uint64_t ticks = this->clientComm->sendComponentCommand(this->remoteGlobalId, "GetTicksSinceOrigin");
    return ticks;
}

template <class TDeviceBase>
ErrCode GenericConfigClientDeviceImpl<TDeviceBase>::Deserialize(ISerializedObject* serialized,
                                                                IBaseObject* context,
                                                                IFunction* factoryCallback,
                                                                IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry([&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::template DeserializeConfigComponent<IDevice, ConfigClientDeviceImpl>(serialized, context, factoryCallback).detach();
        });
}

template <class TDeviceBase>
void GenericConfigClientDeviceImpl<TDeviceBase>::handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args)
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
        case CoreEventId::StatusChanged:
        case CoreEventId::TypeAdded:
        case CoreEventId::TypeRemoved:
        default:
            break;
    }

    Super::handleRemoteCoreObjectInternal(sender, args);
}

template <class TDeviceBase>
void GenericConfigClientDeviceImpl<TDeviceBase>::componentAdded(const CoreEventArgsPtr& args)
{
    const ComponentPtr comp = args.getParameters().get("Component");
    Bool hasItem{false};
    checkErrorInfo(TDeviceBase::hasItem(comp.getLocalId(), &hasItem));
    if (!hasItem)
    {
        this->clientComm->connectDomainSignals(comp);
        this->addExistingComponent(comp);
    }
}

template <class TDeviceBase>
void GenericConfigClientDeviceImpl<TDeviceBase>::componentRemoved(const CoreEventArgsPtr& args)
{
    const StringPtr id = args.getParameters().get("Id");
    Bool hasItem{false};
    checkErrorInfo(TDeviceBase::hasItem(id, &hasItem));
    if (hasItem)
        this->removeComponentById(id);
}
}
