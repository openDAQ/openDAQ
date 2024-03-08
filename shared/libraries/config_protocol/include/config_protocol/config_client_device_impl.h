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
#include <config_protocol/config_protocol_deserialize_context_impl.h>
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
                                           const StringPtr& localId,
                                           const StringPtr& className = nullptr);

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override;
    void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock) override;
    uint64_t onGetTicksSinceOrigin() override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;
    void onRemoteUpdate(const SerializedObjectPtr& serialized) override;

private:
    void componentAdded(const CoreEventArgsPtr& args);
    void componentRemoved(const CoreEventArgsPtr& args);
};

template <class TDeviceBase>
GenericConfigClientDeviceImpl<TDeviceBase>::GenericConfigClientDeviceImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                                          const std::string& remoteGlobalId,
                                                                          const ContextPtr& ctx,
                                                                          const ComponentPtr& parent,
                                                                          const StringPtr& localId,
                                                                          const StringPtr& className)
    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId, className)
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
void GenericConfigClientDeviceImpl<TDeviceBase>::onRemoteUpdate(const SerializedObjectPtr& serialized)
{
    ConfigClientComponentBaseImpl<TDeviceBase>::onRemoteUpdate(serialized);

    std::vector<std::string> toRemove;
    for (const auto& comp : this->components)
    {
        const auto id = comp.getLocalId();
        if (!serialized.hasKey(id))
        {
            if (this->defaultComponents.count(id))
                throw InvalidOperationException{"Serialized remote object does not contain default device component: " + id};
            
            toRemove.push_back(id);
        }
        else
        {
            const auto serObj = serialized.readSerializedObject(id);
            comp.template asPtr<IConfigClientObject>()->remoteUpdate(serObj);
        }
    }

    for (const auto& id : toRemove)
        this->removeComponentById(id);
    
    const std::set<std::string> ignoredKeys{"__type", "deviceInfo", "deviceDomain", "deviceUnit", "deviceResolution", "properties", "propValues"};

    for (const auto& key : serialized.getKeys())
    {
        if (this->defaultComponents.count(key) || ignoredKeys.count(key) || serialized.getType(key) != ctObject)
            continue;
        
        const auto obj = serialized.readSerializedObject(key);
        auto compIterator = std::find_if(this->components.begin(), this->components.end(), [&key](const ComponentPtr& comp) { return comp.getLocalId() == key; });
        if (compIterator != this->components.end())
        {
            compIterator->template asPtr<IConfigClientObject>()->remoteUpdate(obj);
        }
        else
        {
            if (!obj.hasKey("__type"))
                continue;

            const StringPtr type = obj.readString("__type");
            const auto thisPtr = this->template borrowPtr<ComponentPtr>();
            const auto deserializeContext = createWithImplementation<IComponentDeserializeContext, ConfigProtocolDeserializeContextImpl>(
                this->clientComm, this->remoteGlobalId, this->context, nullptr, thisPtr, key, nullptr);

            const ComponentPtr deserializedObj = this->clientComm->deserializeConfigComponent(
                type,
                obj,
                deserializeContext,
                [&](const StringPtr& typeId,
                    const SerializedObjectPtr& object,
                    const BaseObjectPtr& context,
                    const FunctionPtr& factoryCallback)
                {
                    return this->clientComm->deserializeConfigComponent(typeId, object, context, factoryCallback, nullptr);
                },
                nullptr);

            if (deserializedObj.assigned())
                this->addExistingComponent(deserializedObj);
        }
    }
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
