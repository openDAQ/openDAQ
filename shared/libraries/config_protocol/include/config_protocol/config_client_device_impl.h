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
    DictPtr<IString, IDevice> onAddDevices(const DictPtr<IString, IPropertyObject>& connectionArgs,
                                           DictPtr<IString, IInteger> errCodes,
                                           DictPtr<IString, IErrorInfo> errorInfos) override;
    void onRemoveDevice(const DevicePtr& device) override;
    PropertyObjectPtr onCreateDefaultAddDeviceConfig() override;

    ListPtr<ILogFileInfo> onGetLogFileInfos() override;
    StringPtr onGetLog(const StringPtr& id, Int size, Int offset) override;

    ErrCode INTERFACE_FUNC lock(IUser* user) override;
    ErrCode INTERFACE_FUNC unlock(IUser* user) override;
    ErrCode INTERFACE_FUNC forceUnlock() override;

    ErrCode INTERFACE_FUNC getAvailableOperationModes(IList** availableOpModes) override;
    ErrCode INTERFACE_FUNC setOperationMode(OperationModeType modeType) override;
    ErrCode INTERFACE_FUNC setOperationModeRecursive(OperationModeType modeType) override;
    ErrCode INTERFACE_FUNC getOperationMode(OperationModeType* modeType) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;
    void onRemoteUpdate(const SerializedObjectPtr& serialized) override;
    StringPtr onGetRemoteId() const override;
    void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                       const BaseObjectPtr& context,
                                       const FunctionPtr& factoryCallback) override;

    ErrCode serializeCustomValuesForUpdate(ISerializer* serializer) override;

private:
    void componentAdded(const CoreEventArgsPtr& args);
    void componentRemoved(const CoreEventArgsPtr& args);
    void deviceDomainChanged(const CoreEventArgsPtr& args);
    void deviceLockStatusChanged(const CoreEventArgsPtr& args);
    void connectionStatusChanged(const CoreEventArgsPtr& args);
    void operationModeChanged(const CoreEventArgsPtr& args);
    bool handleDeviceInfoPropertyValueChanged(const CoreEventArgsPtr& args);
    bool handleDeviceInfoPropertyAdded(const CoreEventArgsPtr& args);
    bool handleDeviceInfoPropertyRemoved(const CoreEventArgsPtr& args);
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
    if (this->clientComm->getProtocolVersion() < 9)
        this->updateOperationModeNoCoreEvent(OperationModeType::Operation);
}

template <class TDeviceBase>
DictPtr<IString, IFunctionBlockType> GenericConfigClientDeviceImpl<TDeviceBase>::onGetAvailableFunctionBlockTypes()
{
    return this->clientComm->getAvailableFunctionBlockTypes(this->remoteGlobalId);
}

template <class TDeviceBase>
FunctionBlockPtr GenericConfigClientDeviceImpl<TDeviceBase>::onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)
{
    const ComponentHolderPtr fbHolder = this->clientComm->addFunctionBlock(this->remoteGlobalId, typeId, config, this->functionBlocks);

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
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    this->clientComm->removeFunctionBlock(this->remoteGlobalId, functionBlock.getLocalId());

    if (this->functionBlocks.hasItem(functionBlock.getLocalId()))
    {
        this->removeNestedFunctionBlock(functionBlock);
    }
}

template <class TDeviceBase>
uint64_t GenericConfigClientDeviceImpl<TDeviceBase>::onGetTicksSinceOrigin()
{
    return this->clientComm->getTicksSinceOrigin(this->remoteGlobalId);
}

template <class TDeviceBase>
ListPtr<IDeviceInfo> GenericConfigClientDeviceImpl<TDeviceBase>::onGetAvailableDevices()
{
    return this->clientComm->getAvailableDevices(this->remoteGlobalId);
}

template <class TDeviceBase>
DictPtr<IString, IDeviceType> GenericConfigClientDeviceImpl<TDeviceBase>::onGetAvailableDeviceTypes()
{
    return this->clientComm->getAvailableDeviceTypes(this->remoteGlobalId);
}

template <class TDeviceBase>
DevicePtr GenericConfigClientDeviceImpl<TDeviceBase>::onAddDevice(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    const ComponentHolderPtr devHolder = this->clientComm->addDevice(this->remoteGlobalId, connectionString, config, this->devices);

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
DictPtr<IString, IDevice> GenericConfigClientDeviceImpl<TDeviceBase>::onAddDevices(const DictPtr<IString, IPropertyObject>& connectionArgs,
                                                                                   DictPtr<IString, IInteger> errCodes,
                                                                                   DictPtr<IString, IErrorInfo> errorInfos)
{
    const DictPtr<IString, IBaseObject> reply =
        this->clientComm->addDevices(this->remoteGlobalId, connectionArgs, errCodes.assigned(), errorInfos.assigned(), this->devices);

    if (!reply.hasKey("ErrorCode"))
        throw ConfigProtocolException("Invalid reply: \"ErrorCode\" is missing");

    if (errCodes.assigned())
    {
        const DictPtr<IString, IInteger> remoteErrCodes = reply.getOrDefault("ErrorCodes", Dict<IString, IInteger>());
        for (const auto& [connectionString, _] : connectionArgs)
            errCodes[connectionString] = remoteErrCodes.getOrDefault(connectionString, OPENDAQ_SUCCESS);
    }
    if (errorInfos.assigned())
    {
        const DictPtr<IString, IErrorInfo> remoteErrorInfos = reply.getOrDefault("ErrorInfos", Dict<IString, IErrorInfo>());
        for (const auto& [connectionString, _] : connectionArgs)
            errorInfos[connectionString] = remoteErrorInfos.getOrDefault(connectionString, nullptr);
    }

    const ErrCode errCode = reply["ErrorCode"];
    if (OPENDAQ_FAILED(errCode))
    {
        std::string msg = reply.getOrDefault("ErrorMessage", "");
        throwExceptionFromErrorCode(errCode, msg);
    }

    if (!reply.hasKey("AddedDevices"))
        throw ConfigProtocolException("Invalid reply: \"AddedDevices\" key is missing");
    const DictPtr<IString, IComponentHolder> devHolders = reply["AddedDevices"];
    DictPtr<IString, IDevice> devices = Dict<IString, IDevice>();

    for (const auto& [connectionString, _] : connectionArgs)
    {
        devices[connectionString] = nullptr;
        ComponentHolderPtr devHolder = devHolders.getOrDefault(connectionString, nullptr);
        if (!devHolder.assigned())
        {
            DAQLOGF_E(this->loggerComponent,
                      "The subdevice with specified connection string \"{}\" was not added within remote device.",
                      connectionString);
        }
        DevicePtr dev = devHolder.assigned() ? devHolder.getComponent() : nullptr;
        if (dev.assigned())
        {
            if (!this->devices.hasItem(dev.getLocalId()))
            {
                this->clientComm->connectDomainSignals(dev);
                this->devices.addItem(dev);
                this->clientComm->connectInputPorts(dev);
                devices[connectionString] = dev;
            }
            else
            {
                devices[connectionString] = this->devices.getItem(dev.getLocalId());
            }
        }
    }
    return devices;
}

template <class TDeviceBase>
void GenericConfigClientDeviceImpl<TDeviceBase>::onRemoveDevice(const DevicePtr& device)
{
    if (!device.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    this->clientComm->removeDevice(this->remoteGlobalId, device.getLocalId());

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
ListPtr<ILogFileInfo> GenericConfigClientDeviceImpl<TDeviceBase>::onGetLogFileInfos()
{
    return this->clientComm->getLogFileInfos(this->remoteGlobalId);
}

template <class TDeviceBase>
StringPtr GenericConfigClientDeviceImpl<TDeviceBase>::onGetLog(const StringPtr& id, Int size, Int offset)
{
    return this->clientComm->getLog(this->remoteGlobalId, id, size, offset);
}

template <class TDeviceBase>
ErrCode GenericConfigClientDeviceImpl<TDeviceBase>::lock(IUser* user)
{
    if (user != nullptr)
    {
        DAQLOGF_I(this->loggerComponent, "The specified user was ignored when locking a remote device. A session user was used instead.");
    }

    auto lock = this->getRecursiveConfigLock2();

    const ErrCode errCode = daqTry([this] { this->clientComm->lock(this->remoteGlobalId); });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <class TDeviceBase>
ErrCode GenericConfigClientDeviceImpl<TDeviceBase>::unlock(IUser* user)
{
    if (user != nullptr)
    {
        DAQLOGF_I(this->loggerComponent, "The specified user was ignored when unlocking a remote device. A session user was used instead.");
    }

    auto lock = this->getRecursiveConfigLock2();

    auto parentDevice = this->getParentDevice();

    if (parentDevice.assigned() && parentDevice.template asPtr<IDevicePrivate>().isLockedInternal())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_DEVICE_LOCKED);

    const ErrCode errCode = daqTry([this] { this->clientComm->unlock(this->remoteGlobalId); });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <class TDeviceBase>
inline ErrCode GenericConfigClientDeviceImpl<TDeviceBase>::forceUnlock()
{
    auto lock = this->getRecursiveConfigLock2();

    auto parentDevice = this->getParentDevice();

    if (parentDevice.assigned() && parentDevice.template asPtr<IDevicePrivate>().isLockedInternal())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_DEVICE_LOCKED);

    const ErrCode errCode = daqTry([this] { this->clientComm->forceUnlock(this->remoteGlobalId); });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <class TDeviceBase>
inline ErrCode GenericConfigClientDeviceImpl<TDeviceBase>::getAvailableOperationModes(IList** availableOpModes)
{
    OPENDAQ_PARAM_NOT_NULL(availableOpModes);
    const ErrCode errCode = daqTry([this, availableOpModes] 
    {
        const auto protocolVersion = this->clientComm->getProtocolVersion();
        if (protocolVersion > 8 && protocolVersion < 12)
            *availableOpModes = this->clientComm->getAvailableOperationModes(this->remoteGlobalId).detach();
        else
            checkErrorInfo(Super::getAvailableOperationModes(availableOpModes));   
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <class TDeviceBase>
inline ErrCode GenericConfigClientDeviceImpl<TDeviceBase>::setOperationMode(OperationModeType modeType)
{
    const ErrCode errCode = daqTry([this, modeType] 
    {
        this->clientComm->setOperationMode(this->remoteGlobalId, OperationModeTypeToString(modeType));
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <class TDeviceBase>
inline ErrCode GenericConfigClientDeviceImpl<TDeviceBase>::setOperationModeRecursive(OperationModeType modeType)
{
    const ErrCode errCode = daqTry([this, modeType] 
    { 
        this->clientComm->setOperationModeRecursive(this->remoteGlobalId, OperationModeTypeToString(modeType));
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <class TDeviceBase>
inline ErrCode GenericConfigClientDeviceImpl<TDeviceBase>::getOperationMode(OperationModeType* modeType)
{
    OPENDAQ_PARAM_NOT_NULL(modeType);
    const ErrCode errCode = daqTry([this, modeType] 
    { 
        const auto protocolVersion = this->clientComm->getProtocolVersion();
        if (protocolVersion >= 9 && protocolVersion < 12)
            *modeType = OperationModeTypeFromString(this->clientComm->getOperationMode(this->remoteGlobalId)); 
        else
            checkErrorInfo(Super::getOperationMode(modeType));   
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <class TDeviceBase>
inline ErrCode GenericConfigClientDeviceImpl<TDeviceBase>::serializeCustomValuesForUpdate(ISerializer* serializer)
{
    const ErrCode errCode = daqTry([this, &serializer]
    {
        this->serializeConnectionValues(serializer);
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <class TDeviceBase>
ErrCode GenericConfigClientDeviceImpl<TDeviceBase>::Deserialize(ISerializedObject* serialized,
                                                                IBaseObject* context,
                                                                IFunction* factoryCallback,
                                                                IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    const ErrCode errCode = daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        *obj = Super::template DeserializeConfigComponent<IDevice, ConfigClientDeviceImpl>(serialized, context, factoryCallback).detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
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
        case CoreEventId::DeviceLockStateChanged:
            deviceLockStatusChanged(args);
            break;
        case CoreEventId::ConnectionStatusChanged:
            connectionStatusChanged(args);
            break;
        case CoreEventId::DeviceOperationModeChanged:
            operationModeChanged(args);
            break;
        case CoreEventId::PropertyValueChanged:
        {
            if (handleDeviceInfoPropertyValueChanged(args))
                return;
            break;
        }
        case CoreEventId::PropertyAdded:
        {
            if (handleDeviceInfoPropertyAdded(args))
                return;
            break;
        }
        case CoreEventId::PropertyRemoved:
        {
            if (handleDeviceInfoPropertyRemoved(args))
                return;
            break;
        }
        case CoreEventId::PropertyObjectUpdateEnd:
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
    
    const std::set<std::string> ignoredKeys{
        "__type", "deviceDomain", "deviceUnit", "deviceResolution", "properties", "propValues", "ComponentConfig"};

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

            const PropertyObjectPtr deserializedObj = this->clientComm->deserializeConfigComponent(
                type,
                obj,
                deserializeContext,
                [&](const StringPtr& typeId,
                    const SerializedObjectPtr& object,
                    const BaseObjectPtr& context,
                    const FunctionPtr& factoryCallback)
                {
                    return this->clientComm->deserializeConfigComponent(typeId, object, context, factoryCallback);
                });


            if (deserializedObj.assigned())
            {
                if (key == "deviceInfo")
                    this->deviceInfo = deserializedObj;
                else
                    this->addExistingComponent(deserializedObj);
            }
        }
    }

    if (serialized.hasKey("deviceDomain"))
    {
        this->setDeviceDomainNoCoreEvent(serialized.readObject("deviceDomain"));
    }

    if (serialized.hasKey("OperationMode"))
    {
        Int mode = serialized.readInt("OperationMode");
        this->updateOperationModeNoCoreEvent(static_cast<OperationModeType>(mode));
    }
}

template <class TDeviceBase>
StringPtr GenericConfigClientDeviceImpl<TDeviceBase>::onGetRemoteId() const
{
    return String(this->remoteGlobalId).detach();
}

template <class TDeviceBase>
void GenericConfigClientDeviceImpl<TDeviceBase>::deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                                                               const BaseObjectPtr& context,
                                                                               const FunctionPtr& factoryCallback)
{
    TDeviceBase::deserializeCustomObjectValues(serializedObject, context, factoryCallback);
    if (serializedObject.hasKey("ComponentConfig"))
        this->componentConfig = serializedObject.readObject("ComponentConfig");
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

template <class TDeviceBase>
inline void GenericConfigClientDeviceImpl<TDeviceBase>::deviceLockStatusChanged(const CoreEventArgsPtr& args)
{
    const Bool isLocked = args.getParameters().get("IsLocked");

    this->userLock.forceUnlock();

    if (isLocked)
        this->userLock.lock();
}

template <class TDeviceBase>
void GenericConfigClientDeviceImpl<TDeviceBase>::connectionStatusChanged(const CoreEventArgsPtr& args)
{
    ComponentStatusContainerPtr connectionStatusContainer;
    checkErrorInfo(TDeviceBase::getConnectionStatusContainer(&connectionStatusContainer));
    const auto parameters = args.getParameters();
    const StringPtr connectionString = parameters.get("ConnectionString");
    const StringPtr statusName = parameters.get("StatusName");
    const EnumerationPtr value = parameters.get("StatusValue");
    const auto addedStatuses = connectionStatusContainer.getStatuses();

    StringPtr message = String("");
    if (parameters.hasKey("Message"))
        message = parameters.get("Message");

    // ignores status change if it was not added initially
    if (addedStatuses.hasKey(statusName))
        connectionStatusContainer.asPtr<IConnectionStatusContainerPrivate>().updateConnectionStatusWithMessage(connectionString, value, nullptr, message);
}

template <class TDeviceBase>
void GenericConfigClientDeviceImpl<TDeviceBase>::operationModeChanged(const CoreEventArgsPtr& args)
{
    const Int mode = args.getParameters().get("OperationMode");
    this->updateOperationModeInternal(static_cast<OperationModeType>(mode));
}

template <class TDeviceBase>
bool GenericConfigClientDeviceImpl<TDeviceBase>::handleDeviceInfoPropertyValueChanged(const CoreEventArgsPtr& args)
{
    const auto params = args.getParameters();
    const std::string path = params.get("Path");

    const std::string prefix = "DaqDeviceInfo";
    if (path.find(prefix) == std::string::npos)
        return false;

    std::string propName = params.get("Name");
    if (path.size() != prefix.size())
        propName = path.substr(prefix.size() + 1) + "." + propName;

    const auto val = params.get("Value");

    ScopedRemoteUpdate update(this->deviceInfo);
    this->deviceInfo.setPropertyValue(propName, val);
    return true;
}

template <class TDeviceBase>
bool GenericConfigClientDeviceImpl<TDeviceBase>::handleDeviceInfoPropertyAdded(const CoreEventArgsPtr& args)
{
    const auto params = args.getParameters();
    std::string path = params.get("Path");

    const std::string prefix = "DaqDeviceInfo";
    if (path.find(prefix) == std::string::npos)
        return false;

    PropertyObjectPtr propObjPtr;
    if (path.size() != prefix.size())
    {
        path = path.substr(prefix.size() + 1);
        propObjPtr = this->deviceInfo.getPropertyValue(path);
    }
    else
    {
        propObjPtr = this->deviceInfo;
    }
    const PropertyPtr prop = params.get("Property");
    if (propObjPtr.hasProperty(prop.getName()))
        return true;

    if (auto configObj = dynamic_cast<ConfigClientPropertyImpl*>(prop.getObject()); configObj)
        configObj->setRemoteGlobalId(this->remoteGlobalId);

    // fixme - nested property objects of DeviceInfo do not support IConfigClientObject interface
    //ScopedRemoteUpdate update(propObjPtr);
    propObjPtr.addProperty(prop);
    return true;
}

template <class TDeviceBase>
bool GenericConfigClientDeviceImpl<TDeviceBase>::handleDeviceInfoPropertyRemoved(const CoreEventArgsPtr& args)
{
    const auto params = args.getParameters();
    std::string path = params.get("Path");

    const std::string prefix = "DaqDeviceInfo";
    if (path.find(prefix) == std::string::npos)
        return false;

    PropertyObjectPtr propObjPtr;
    if (path.size() != prefix.size())
    {
        path = path.substr(prefix.size() + 1);
        propObjPtr = this->deviceInfo.getPropertyValue(path);
    }
    else
    {
        propObjPtr = this->deviceInfo;
    }
    const std::string propName = params.get("Name");
    if (!propObjPtr.hasProperty(propName))
        return true;

    // fixme - nested property objects of DeviceInfo do not support IConfigClientObject interface
    //ScopedRemoteUpdate update(propObjPtr);
    propObjPtr.removeProperty(propName);
    return true;
}

}
