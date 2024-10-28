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
#include <opendaq/mirrored_device_impl.h>
#include <config_protocol/config_client_component_impl.h>
#include <config_protocol/config_protocol_deserialize_context_impl.h>
#include <opendaq/component_holder_ptr.h>

namespace daq::config_protocol
{

template <typename... Interfaces>
using ConfigClientDeviceBase = MirroredDeviceBase<IConfigClientObject, Interfaces...>;

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
    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onAddDevice(const StringPtr& connectionString, const PropertyObjectPtr& config) override;
    void onRemoveDevice(const DevicePtr& device) override;
    PropertyObjectPtr onCreateDefaultAddDeviceConfig() override;

    ListPtr<ILogFileInfo> ongetLogFileInfos() override;
    StringPtr onGetLog(const StringPtr& id, Int size, Int offset) override;

    ErrCode INTERFACE_FUNC lock(IUser* user) override;
    ErrCode INTERFACE_FUNC unlock(IUser* user) override;
    ErrCode INTERFACE_FUNC isLocked(Bool* locked) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;
    void onRemoteUpdate(const SerializedObjectPtr& serialized) override;

private:
    void componentAdded(const CoreEventArgsPtr& args);
    void componentRemoved(const CoreEventArgsPtr& args);
    void deviceDomainChanged(const CoreEventArgsPtr& args);
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
ListPtr<IDeviceInfo> GenericConfigClientDeviceImpl<TDeviceBase>::onGetAvailableDevices()
{
    if (!(this->clientComm->getProtocolVersion() >= 4))
        throwExceptionFromErrorCode(OPENDAQ_ERR_NATIVE_CLIENT_CALL_NOT_AVAILABLE,
                                    "Operation not supported by the protocol version currently in use");
    return this->clientComm->sendComponentCommand(this->remoteGlobalId, "GetAvailableDevices");
}

template <class TDeviceBase>
DictPtr<IString, IDeviceType> GenericConfigClientDeviceImpl<TDeviceBase>::onGetAvailableDeviceTypes()
{
    if (!(this->clientComm->getProtocolVersion() >= 4))
        throwExceptionFromErrorCode(OPENDAQ_ERR_NATIVE_CLIENT_CALL_NOT_AVAILABLE,
                                    "Operation not supported by the protocol version currently in use");
    return this->clientComm->sendComponentCommand(this->remoteGlobalId, "GetAvailableDeviceTypes");
}

template <class TDeviceBase>
DevicePtr GenericConfigClientDeviceImpl<TDeviceBase>::onAddDevice(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    if (!(this->clientComm->getProtocolVersion() >= 4))
        throwExceptionFromErrorCode(OPENDAQ_ERR_NATIVE_CLIENT_CALL_NOT_AVAILABLE,
                                    "Operation not supported by the protocol version currently in use");

    auto params = Dict<IString, IBaseObject>({{"ConnectionString", connectionString}, {"Config", config}});
    const ComponentHolderPtr devHolder = this->clientComm->sendComponentCommand(this->remoteGlobalId, "AddDevice", params, this->devices);

    DevicePtr dev = devHolder.getComponent();
    if (!this->devices.hasItem(dev.getLocalId()))
    {
        this->clientComm->connectDomainSignals(dev);
        this->devices.addItem(dev);
        this->clientComm->connectInputPorts(dev);

        return dev;
    }
    return this->devices.getItem(dev.getLocalId());
}

template <class TDeviceBase>
void GenericConfigClientDeviceImpl<TDeviceBase>::onRemoveDevice(const DevicePtr& device)
{
    if (!(this->clientComm->getProtocolVersion() >= 4))
        throwExceptionFromErrorCode(OPENDAQ_ERR_NATIVE_CLIENT_CALL_NOT_AVAILABLE,
                                    "Operation not supported by the protocol version currently in use");

    if (!device.assigned())
        throw InvalidParameterException();

    auto params = Dict<IString, IBaseObject>({{"LocalId", device.getLocalId()}});
    this->clientComm->sendComponentCommand(this->remoteGlobalId, "RemoveDevice", params);

    if (this->devices.hasItem(device.getLocalId()))
    {
        this->devices.removeItem(device);
    }
}

template <class TDeviceBase>
PropertyObjectPtr GenericConfigClientDeviceImpl<TDeviceBase>::onCreateDefaultAddDeviceConfig()
{
    return PropertyObject();
}

template <class TDeviceBase>
ListPtr<ILogFileInfo> GenericConfigClientDeviceImpl<TDeviceBase>::ongetLogFileInfos()
{
    if (this->clientComm->getProtocolVersion() < 5)
        throwExceptionFromErrorCode(OPENDAQ_ERR_NATIVE_CLIENT_CALL_NOT_AVAILABLE,
                                    "Operation not supported by the protocol version currently in use");

    return this->clientComm->sendComponentCommand(this->remoteGlobalId, "getLogFileInfos");
}

template <class TDeviceBase>
StringPtr GenericConfigClientDeviceImpl<TDeviceBase>::onGetLog(const StringPtr& id, Int size, Int offset)
{
    if (this->clientComm->getProtocolVersion() < 5)
        throwExceptionFromErrorCode(OPENDAQ_ERR_NATIVE_CLIENT_CALL_NOT_AVAILABLE,
                                    "Operation not supported by the protocol version currently in use");

    auto params = Dict<IString, IBaseObject>({{"Id", id}, {"Size", size}, {"Offset", offset}});
    return this->clientComm->sendComponentCommand(this->remoteGlobalId, "GetLog", params);
}

template <class TDeviceBase>
ErrCode INTERFACE_FUNC GenericConfigClientDeviceImpl<TDeviceBase>::lock(IUser* user)
{
    if (user != nullptr)
    {
        DAQLOGF_I(this->loggerComponent, "The specified user was ignored when locking a remote device. A session user was used instead.");
    }

    return daqTry([this] { this->clientComm->lock(this->remoteGlobalId); });
}

template <class TDeviceBase>
ErrCode INTERFACE_FUNC GenericConfigClientDeviceImpl<TDeviceBase>::unlock(IUser* user)
{
    if (user != nullptr)
    {
        DAQLOGF_I(this->loggerComponent, "The specified user was ignored when unlocking a remote device. A session user was used instead.");
    }

    return daqTry([this] { this->clientComm->unlock(this->remoteGlobalId); });
}

template <class TDeviceBase>
inline ErrCode INTERFACE_FUNC GenericConfigClientDeviceImpl<TDeviceBase>::isLocked(Bool* locked)
{
    OPENDAQ_PARAM_NOT_NULL(locked);

    return daqTry([this, &locked] { *locked = this->clientComm->isLocked(this->remoteGlobalId); });
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
        case CoreEventId::DeviceDomainChanged:
            deviceDomainChanged(args);
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
            {
                DAQLOGF_D(this->loggerComponent, "The server does not provide default device component: {}", id);
            }
            else
            {
                toRemove.push_back(id);
            }
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
                this->clientComm, this->remoteGlobalId + "/" + key, this->context, nullptr, thisPtr, key, nullptr);

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

    if (serialized.hasKey("deviceDomain"))
    {
        this->setDeviceDomainNoCoreEvent(serialized.readObject("deviceDomain"));
    }

    if (serialized.hasKey("deviceInfo"))
    {
        this->deviceInfo = serialized.readObject("deviceInfo");
        this->deviceInfo.template asPtr<IOwnable>().setOwner(this->objPtr);
        this->deviceInfo.freeze();
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
        this->clientComm->connectInputPorts(comp);
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

template <class TDeviceBase>
void GenericConfigClientDeviceImpl<TDeviceBase>::deviceDomainChanged(const CoreEventArgsPtr& args)
{
    const DeviceDomainPtr domain = args.getParameters().get("DeviceDomain");
    this->setDeviceDomain(domain);
}
}
