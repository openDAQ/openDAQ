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
#include <opendaq/context_ptr.h>
#include <opendaq/channel_ptr.h>
#include <opendaq/device.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_info_ptr.h>
#include <opendaq/device_ptr.h>
#include <opendaq/signal_container_impl.h>
#include <opendaq/signal_ptr.h>
#include <coreobjects/unit_ptr.h>
#include <opendaq/utility_sync.h>
#include <opendaq/folder_factory.h>
#include <opendaq/io_folder_factory.h>
#include <coreobjects/property_object_impl.h>
#include <coretypes/validation.h>
#include <opendaq/device_private.h>
#include <opendaq/streaming_info_factory.h>
#include <tsl/ordered_set.h>
#include <opendaq/component_keys.h>
#include <opendaq/core_opendaq_event_args_factory.h>
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IDevice, typename... Interfaces>
class GenericDevice;

template <typename... TTraits>
using DeviceBase = GenericDevice<IDevice, TTraits...>;

using Device = DeviceBase<>;

template <typename TInterface, typename... Interfaces>
class GenericDevice : public SignalContainerImpl<TInterface, IDeviceDomain, IDevicePrivate, Interfaces...>
{
public:
    using Super = SignalContainerImpl<TInterface, IDeviceDomain, IDevicePrivate, Interfaces...>;
    using Self = GenericDevice<TInterface, Interfaces...>;

    GenericDevice(const ContextPtr& ctx,
                  const ComponentPtr& parent,
                  const StringPtr& localId,
                  const StringPtr& className = nullptr,
                  ComponentStandardProps propsMode = ComponentStandardProps::Add);

    virtual DeviceInfoPtr onGetInfo() = 0;

    virtual RatioPtr onGetResolution();
    virtual uint64_t onGetTicksSinceOrigin();
    virtual std::string onGetOrigin();
    virtual UnitPtr onGetDomainUnit();

    virtual DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes();
    virtual FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config);
    virtual void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock);

    virtual ListPtr<IDeviceInfo> onGetAvailableDevices();
    virtual DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes();
    virtual DevicePtr onAddDevice(const StringPtr& connectionString, const PropertyObjectPtr& config);
    virtual void onRemoveDevice(const DevicePtr& device);

    // IDevice
    ErrCode INTERFACE_FUNC getInfo(IDeviceInfo** info) override;
    ErrCode INTERFACE_FUNC getDomain(IDeviceDomain** deviceDomain) override;

    ErrCode INTERFACE_FUNC getInputsOutputsFolder(IFolder** inputsOutputsFolder) override;
    ErrCode INTERFACE_FUNC getCustomComponents(IList** customComponents) override;
    ErrCode INTERFACE_FUNC getSignals(IList** signals) override;
    ErrCode INTERFACE_FUNC getSignalsRecursive(IList** signals) override;
    ErrCode INTERFACE_FUNC getChannels(IList** channels) override;
    ErrCode INTERFACE_FUNC getChannelsRecursive(IList** channels) override;

    // IDevicePrivate
    ErrCode INTERFACE_FUNC addStreamingOption(IStreamingInfo* info) override;
    ErrCode INTERFACE_FUNC removeStreamingOption(IString* protocolId) override;
    ErrCode INTERFACE_FUNC getStreamingOptions(IList** streamingOptions) override;

    // Function block devices
    ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) override;
    ErrCode INTERFACE_FUNC addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config) override;
    ErrCode INTERFACE_FUNC removeFunctionBlock(IFunctionBlock* functionBlock) override;
    ErrCode INTERFACE_FUNC getFunctionBlocks(IList** functionBlocks) override;

    // Client devices
    ErrCode INTERFACE_FUNC getAvailableDevices(IList** availableDevices) override;
    ErrCode INTERFACE_FUNC getAvailableDeviceTypes(IDict** deviceTypes) override;
    ErrCode INTERFACE_FUNC addDevice(IDevice** device, IString* connectionString, IPropertyObject* config = nullptr) override;
    ErrCode INTERFACE_FUNC removeDevice(IDevice* device) override;
    ErrCode INTERFACE_FUNC getDevices(IList** devices) override;

    ErrCode INTERFACE_FUNC saveConfiguration(IString** configuration) override;
    ErrCode INTERFACE_FUNC loadConfiguration(IString* configuration) override;

    // IDeviceDomain
    ErrCode INTERFACE_FUNC getTickResolution(IRatio** resolution) override;
    ErrCode INTERFACE_FUNC getTicksSinceOrigin(uint64_t* ticks) override;
    ErrCode INTERFACE_FUNC getOrigin(IString** origin) override;
    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IBaseObject** obj);

protected:
    DeviceInfoPtr deviceInfo;
    FolderConfigPtr devices;
    IoFolderConfigPtr ioFolder;
    std::vector<StreamingInfoPtr> streamingOptions;
    LoggerComponentPtr loggerComponent;

    template <class ChannelImpl, class... Params>
    ChannelPtr createAndAddChannel(const FolderConfigPtr& parentFolder, const StringPtr& localId, Params&&... params);
    void removeChannel(const FolderConfigPtr& parentFolder, const ChannelPtr& channel);

    void addSubDevice(const DevicePtr& device);
    void removeSubDevice(const DevicePtr& device);

    IoFolderConfigPtr addIoFolder(const std::string& localId,
                                  const IoFolderConfigPtr& parent = nullptr,
                                  ComponentStandardProps standardPropsConfig = ComponentStandardProps::Add);

    void serializeCustomObjectValues(const SerializerPtr& serializer) override;
    void deserializeFunctionBlock(const std::string& fbId,
                                  const SerializedObjectPtr& serializedFunctionBlock) override;
    void updateDevice(const std::string& deviceId, const SerializedObjectPtr& serializedDevice);
    void updateIoFolderItem(const FolderPtr& ioFolder,
                            const std::string& localId,
                            const SerializedObjectPtr& item);

    void updateObject(const SerializedObjectPtr& obj) override;
    bool clearFunctionBlocksOnUpdate() override;

private:
    void getChannelsFromFolder(const FolderPtr& folder, ListPtr<IChannel>& channels);
};

template <typename TInterface, typename... Interfaces>
GenericDevice<TInterface, Interfaces...>::GenericDevice(const ContextPtr& ctx,
                                                        const ComponentPtr& parent,
                                                        const StringPtr& localId,
                                                        const StringPtr& className,
                                                        const ComponentStandardProps propsMode)
    : Super(ctx, parent, localId, className, propsMode)
    , loggerComponent(this->context.getLogger().assigned() ? this->context.getLogger().getOrAddComponent(this->globalId)
                                                           : throw ArgumentNullException("Logger must not be null"))

{
    this->defaultComponents.insert("Dev");
    this->defaultComponents.insert("IO");
    this->allowNonDefaultComponents = true;

    devices = this->template addFolder<IDevice>("Dev", nullptr, ComponentStandardProps::Skip);
    ioFolder = this->addIoFolder("IO", nullptr, ComponentStandardProps::Skip);

    this->addProperty(StringProperty("UserName", ""));
    this->addProperty(StringProperty("Location", ""));
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getInfo(IDeviceInfo** info)
{
    OPENDAQ_PARAM_NOT_NULL(info);

    DeviceInfoPtr devInfo;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetInfo, devInfo);

    *info = devInfo.detach();

    return errCode;
}

/*template <typename TInterface, typename... Interfaces>
DeviceInfoPtr GenericDevice<TInterface, Interfaces...>::onGetInfo()
{
    return deviceInfo;
}
*/

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getDomain(IDeviceDomain** deviceDomain)
{
    OPENDAQ_PARAM_NOT_NULL(deviceDomain);

    *deviceDomain = this->template thisInterface<IDeviceDomain>();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::addStreamingOption(IStreamingInfo* info)
{
    OPENDAQ_PARAM_NOT_NULL(info);

    auto infoPtr = StreamingInfoPtr::Borrow(info);

    std::scoped_lock lock(this->sync);
    auto it = std::find_if(this->streamingOptions.begin(),
                           this->streamingOptions.end(),
                           [&infoPtr](const StreamingInfoPtr& option)
                           {
                               return option.getProtocolId() == infoPtr.getProtocolId();
                           });
    if (it != this->streamingOptions.end())
        return OPENDAQ_ERR_DUPLICATEITEM;

    streamingOptions.push_back(infoPtr);
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::removeStreamingOption(IString* protocolId)
{
    OPENDAQ_PARAM_NOT_NULL(protocolId);

    const auto protocolIdPtr = StringPtr::Borrow(protocolId);

    std::scoped_lock lock(this->sync);
    auto it = std::find_if(this->streamingOptions.begin(),
                           this->streamingOptions.end(),
                           [&protocolIdPtr](const StreamingInfoPtr& option)
                           {
                               return option.getProtocolId() == protocolIdPtr;
                           });
    if (it == this->streamingOptions.end())
        return OPENDAQ_ERR_NOTFOUND;

    this->streamingOptions.erase(it);
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getStreamingOptions(IList** streamingOptions)
{
    OPENDAQ_PARAM_NOT_NULL(streamingOptions);

    std::scoped_lock lock(this->sync);
    ListPtr<IStreamingInfo> streamingOptionsPtr{this->streamingOptions};
    *streamingOptions = streamingOptionsPtr.detach();

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getInputsOutputsFolder(IFolder** inputsOutputsFolder)
{
    OPENDAQ_PARAM_NOT_NULL(inputsOutputsFolder);

    *inputsOutputsFolder = ioFolder.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getCustomComponents(IList** customComponents)
{
    OPENDAQ_PARAM_NOT_NULL(customComponents);

    auto componentsList = List<IComponent>();
    for (const auto& component : this->components)
    {
        if (!this->defaultComponents.count(component.getLocalId()))
            componentsList.pushBack(component);
    }

    *customComponents = componentsList.detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getSignals(IList** signals)
{
    return this->signals->getItems(signals);
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getSignalsRecursive(IList** signals)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    tsl::ordered_set<SignalPtr, ComponentHash, ComponentEqualTo> allSignals;

    auto channels = List<IChannel>();
    getChannelsFromFolder(ioFolder, channels);
    for (const auto& ch : channels)
    {
        auto chSignals = ch.getSignalsRecursive();
        for (const auto& signal : chSignals)
            allSignals.insert(signal);
    }

    auto deviceSignals = this->signals.getItems();
    for (const auto& devSig : deviceSignals)
        allSignals.insert(devSig.template asPtr<ISignal>());

    const auto functionBlocks = this->functionBlocks.getItems();
    for (const auto& functionBlock : functionBlocks)
    {
        auto functionBlockSignals = functionBlock.template asPtr<IFunctionBlock>().getSignalsRecursive();
        for (const auto& signal : functionBlockSignals)
            allSignals.insert(signal);
    }

    for (const auto& device : this->devices.getItems())
        for (const auto& signal : device.template asPtr<IDevice>().getSignalsRecursive())
            allSignals.insert(signal);

    // Copy tsl set to openDAQ list
    auto itemList = List<ISignal>();
    for (const auto& signal : allSignals)
        itemList.pushBack(signal);
    *signals = itemList.detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
template <class ChannelImpl, class... Params>
ChannelPtr GenericDevice<TInterface, Interfaces...>::createAndAddChannel(const FolderConfigPtr& parentFolder,
                                                                const StringPtr& localId,
                                                                Params&&... params)
{
    auto ch = createWithImplementation<IChannel, ChannelImpl>(
        this->context, parentFolder, localId, std::forward<Params>(params)...);

    parentFolder.addItem(ch);
    return ch;
}

template <typename TInterface, typename ... Interfaces>
void GenericDevice<TInterface, Interfaces...>::removeChannel(const FolderConfigPtr& parentFolder, const ChannelPtr& channel)
{
    if (parentFolder == nullptr)
    {
        const auto folder = channel.getParent().asPtr<IFolderConfig>();
        folder.removeItem(channel);
    }
    else
        parentFolder.removeItem(channel);
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getTickResolution(IRatio** resolution)
{
    OPENDAQ_PARAM_NOT_NULL(resolution);

    RatioPtr resolutionPtr;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetResolution, resolutionPtr);

    *resolution = resolutionPtr.detach();

    return errCode;
}

template <typename TInterface, typename... Interfaces>
RatioPtr GenericDevice<TInterface, Interfaces...>::onGetResolution()
{
    throw NotImplementedException();
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getTicksSinceOrigin(uint64_t* ticks)
{
    OPENDAQ_PARAM_NOT_NULL(ticks);

    ErrCode errCode = wrapHandlerReturn(this, &Self::onGetTicksSinceOrigin, *ticks);

    return errCode;
}

template <typename TInterface, typename... Interfaces>
uint64_t GenericDevice<TInterface, Interfaces...>::onGetTicksSinceOrigin()
{
    throw NotImplementedException();
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getOrigin(IString** origin)
{
    OPENDAQ_PARAM_NOT_NULL(origin);

    std::string originStr;
    ErrCode errCode = wrapHandlerReturn(this, &Self::onGetOrigin, originStr);

    *origin = String(originStr).detach();

    return errCode;
}

template <typename TInterface, typename... Interfaces>
std::string GenericDevice<TInterface, Interfaces...>::onGetOrigin()
{
    return {};
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getUnit(IUnit** unit)
{
    OPENDAQ_PARAM_NOT_NULL(unit);

    UnitPtr unitPtr;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetDomainUnit, unitPtr);

    *unit = unitPtr.detach();

    return errCode;
}

template <typename TInterface, typename... Interfaces>
ErrCode INTERFACE_FUNC GenericDevice<TInterface, Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ConstCharPtr GenericDevice<TInterface, Interfaces...>::SerializeId()
{
    return "Device";
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::Deserialize(ISerializedObject* serialized, IBaseObject* context, IBaseObject** obj)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

template <typename TInterface, typename... Interfaces>
UnitPtr GenericDevice<TInterface, Interfaces...>::onGetDomainUnit()
{
    throw NotImplementedException();
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getAvailableFunctionBlockTypes(IDict** functionBlockTypes)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlockTypes);

    DictPtr<IString, IFunctionBlockType> dict;
    const ErrCode errCode = wrapHandlerReturn(this, &GenericDevice<TInterface, Interfaces...>::onGetAvailableFunctionBlockTypes, dict);

    *functionBlockTypes = dict.detach();
    return errCode;
}

template <typename TInterface, typename... Interfaces>
DictPtr<IString, IFunctionBlockType> GenericDevice<TInterface, Interfaces...>::onGetAvailableFunctionBlockTypes()
{
    return Dict<IString, IFunctionBlockType>();
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlock);
    OPENDAQ_PARAM_NOT_NULL(typeId);

    FunctionBlockPtr functionBlockPtr;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onAddFunctionBlock, functionBlockPtr, typeId, config);

    *functionBlock = functionBlockPtr.detach();
    return errCode;
}

template <typename TInterface, typename... Interfaces>
FunctionBlockPtr GenericDevice<TInterface, Interfaces...>::onAddFunctionBlock(const StringPtr& /*typeId*/,
                                                                              const PropertyObjectPtr& /*config*/)
{
    throw NotFoundException("Function block not found");
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::removeFunctionBlock(IFunctionBlock* functionBlock)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlock);

    const auto fbPtr = FunctionBlockPtr::Borrow(functionBlock);
    const ErrCode errCode = wrapHandler(this, &Self::onRemoveFunctionBlock, fbPtr);

    return errCode;
}

template <typename TInterface, typename... Interfaces>
void GenericDevice<TInterface, Interfaces...>::onRemoveFunctionBlock(const FunctionBlockPtr& /*functionBlock*/)
{
    throw NotFoundException("Function block not found");
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getFunctionBlocks(IList** functionBlocks)
{
    return this->functionBlocks->getItems(functionBlocks);
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getChannels(IList** channels)
{
    OPENDAQ_PARAM_NOT_NULL(channels);

    auto chList = List<IChannel>();
    getChannelsFromFolder(ioFolder, chList);

    *channels = chList.detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getChannelsRecursive(IList** channels)
{
    auto chList = List<IChannel>();
    getChannelsFromFolder(ioFolder, chList);

    for (const auto& dev : devices.getItems())
    {
        auto devChs = dev.template asPtr<IDevice>().getChannelsRecursive();
        for (const auto& ch : devChs)
            chList.pushBack(ch);
    }

    *channels = chList.detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
void GenericDevice<TInterface, Interfaces...>::getChannelsFromFolder(const FolderPtr& folder, ListPtr<IChannel>& channels)
{
    for (const auto& component: folder.getItems())
    {
        if (component.supportsInterface<IChannel>())
            channels.pushBack(component);
        else if (component.supportsInterface<IFolder>())
            getChannelsFromFolder(component, channels);
    }
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getAvailableDevices(IList** availableDevices)
{
    OPENDAQ_PARAM_NOT_NULL(availableDevices);

    ListPtr<IDeviceInfo> availableDevicesPtr;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetAvailableDevices, availableDevicesPtr);

    *availableDevices = availableDevicesPtr.detach();

    return errCode;
}

template <typename TInterface, typename... Interfaces>
ListPtr<IDeviceInfo> GenericDevice<TInterface, Interfaces...>::onGetAvailableDevices()
{
    return List<IDeviceInfo>();
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getAvailableDeviceTypes(IDict** deviceTypes)
{
    OPENDAQ_PARAM_NOT_NULL(deviceTypes);

    DictPtr<IString, IDeviceType> dict;
    const ErrCode errCode = wrapHandlerReturn(this, &GenericDevice<TInterface, Interfaces...>::onGetAvailableDeviceTypes, dict);

    *deviceTypes = dict.detach();
    return errCode;
}

template <typename TInterface, typename... Interfaces>
DictPtr<IString, IDeviceType> GenericDevice<TInterface, Interfaces...>::onGetAvailableDeviceTypes()
{
    return Dict<IString, IDeviceType>();
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::addDevice(IDevice** device, IString* connectionString, IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);
    OPENDAQ_PARAM_NOT_NULL(device);

    DevicePtr devicePtr;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onAddDevice, devicePtr, connectionString, config);

    *device = devicePtr.detach();

    return errCode;
}

template <typename TInterface, typename... Interfaces>
DevicePtr GenericDevice<TInterface, Interfaces...>::onAddDevice(const StringPtr& /*connectionString*/, const PropertyObjectPtr& /*config*/)
{
    return nullptr;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::removeDevice(IDevice* device)
{
    OPENDAQ_PARAM_NOT_NULL(device);

    const auto devicePtr = DevicePtr::Borrow(device);
    const ErrCode errCode = wrapHandler(this, &Self::onRemoveDevice, devicePtr);

    return errCode;
}

template <typename TInterface, typename... Interfaces>
void GenericDevice<TInterface, Interfaces...>::onRemoveDevice(const DevicePtr& /*device*/)
{
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getDevices(IList** devices)
{
    return this->devices->getItems(devices);
}

template <typename TInterface, typename ... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::saveConfiguration(IString** configuration)
{
    OPENDAQ_PARAM_NOT_NULL(configuration);

    return daqTry(
        [this, &configuration]() {
            auto serializer = JsonSerializer(True);

            checkErrorInfo(this->serialize(serializer));

            auto str = serializer.getOutput();

            *configuration = str.detach();

            return OPENDAQ_SUCCESS;
        });
}

template <typename TInterface, typename ... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::loadConfiguration(IString* configuration)
{
    OPENDAQ_PARAM_NOT_NULL(configuration);

    return daqTry(
        [this, &configuration]()
        {
            const auto deserializer = JsonDeserializer();

            auto updatable = this->template borrowInterface<IUpdatable>();

            deserializer.update(updatable, configuration);

            return OPENDAQ_SUCCESS;
        });
}

template <class TInterface, class... Interfaces>
void GenericDevice<TInterface, Interfaces...>::addSubDevice(const DevicePtr& device)
{
    if (device.getParent() != devices)
        throw InvalidParameterException("Invalid parent of device");

    try
    {
        devices.addItem(device);
    }
    catch (DuplicateItemException&)
    {
        throw DuplicateItemException("Device with the same local ID already exists.");
    }
}

template <class TInterface, class... Interfaces>
void GenericDevice<TInterface, Interfaces...>::removeSubDevice(const DevicePtr& device)
{
    devices.removeItem(device);
}

template <typename TInterface, typename... Interfaces>
inline IoFolderConfigPtr GenericDevice<TInterface, Interfaces...>::addIoFolder(const std::string& localId,
                                                                               const IoFolderConfigPtr& parent,
                                                                               const ComponentStandardProps standardPropsConfig)
{
    if (!parent.assigned())
    {
        this->validateComponentNotExists(localId);

        auto folder = IoFolder(this->context, this->template thisPtr<ComponentPtr>(), localId, standardPropsConfig);
        this->components.push_back(folder);

        if (!this->coreEventMuted && this->coreEvent.assigned())
        {
             this->triggerCoreEvent(CoreEventArgsComponentAdded(folder));
             folder.template asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
        }

        return folder;
    }

    auto folder = IoFolder(this->context, parent, localId, standardPropsConfig);
    parent.addItem(folder);
    return folder;
}

template <typename TInterface, typename ... Interfaces>
void GenericDevice<TInterface, Interfaces...>::serializeCustomObjectValues(const SerializerPtr& serializer)
{
    Super::serializeCustomObjectValues(serializer);

    if (!ioFolder.isEmpty())
    {
        serializer->key("io");
        ioFolder.serialize(serializer);
    }

    if (!devices.isEmpty())
    {
        serializer->key("dev");
        devices.serialize(serializer);
    }

    for (const auto& component : this->components)
    {
        if (!this->defaultComponents.count(component.getLocalId()))
        {
            serializer->key(component.getLocalId().getCharPtr());
            component.serialize(serializer);
        }
    }
}

template <typename TInterface, typename... Interfaces>
void GenericDevice<TInterface, Interfaces...>::deserializeFunctionBlock(const std::string& fbId,
                                                                        const SerializedObjectPtr& serializedFunctionBlock)
{
    auto typeId = serializedFunctionBlock.readString("typeId");

    auto config = PropertyObject();
    config.addProperty(StringProperty("LocalId", fbId));

    auto fb = onAddFunctionBlock(typeId, config);

    const auto updatableFb = fb.template asPtr<IUpdatable>(true);

    updatableFb.update(serializedFunctionBlock);
}

template <typename TInterface, typename... Interfaces>
void GenericDevice<TInterface, Interfaces...>::updateDevice(const std::string& deviceId,
                                                            const SerializedObjectPtr& serializedDevice)
{
    if (!devices.hasItem(deviceId))
    {
        LOG_W("Device {} not found", deviceId)
        return;
    }

    const auto device = devices.getItem(deviceId);
    const auto updatableDevice = device.template asPtr<IUpdatable>(true);

    try
    {
        updatableDevice.update(serializedDevice);
    }
    catch (const std::exception& e)
    {
        LOG_W("Failed to update device: {}", e.what());
    }
}

template <typename TInterface, typename... Interfaces>
void GenericDevice<TInterface, Interfaces...>::updateIoFolderItem(const FolderPtr& parentIoFolder,
                                                                  const std::string& localId,
                                                                  const SerializedObjectPtr& serializedItem)
{
    if (!parentIoFolder.hasItem(localId))
    {
        //        LOG_W("IoFolder {} not found", localId)
        return;
    }

    const auto item = parentIoFolder.getItem(localId);
    if (item.supportsInterface<IChannel>())
    {
        const auto updatableChannel = item.asPtr<IUpdatable>(true);

        updatableChannel.update(serializedItem);
    }
    else if (item.supportsInterface<IFolder>())
    {
        const auto updatableFolder = item.asPtr<IUpdatable>(true);
        updatableFolder.update(serializedItem);

        this->updateFolder(serializedItem,
                           "IoFolder",
                           "",
                           [this, &item](const std::string& itemId, const SerializedObjectPtr& obj)
                           { updateIoFolderItem(item, itemId, obj); });
    }
}

template <typename TInterface, typename... Interfaces>
void GenericDevice<TInterface, Interfaces...>::updateObject(const SerializedObjectPtr& obj)
{
    Super::updateObject(obj);

    if (obj.hasKey("dev"))
    {
        const auto devicesFolder = obj.readSerializedObject("dev");
        devicesFolder.checkObjectType("Folder");

        this->updateFolder(devicesFolder,
                           "Folder",
                           "Device",
                           [this](const std::string& localId, const SerializedObjectPtr& obj)
                           { updateDevice(localId, obj); });
    }

    if (obj.hasKey("io"))
    {
        const auto ioFolder = obj.readSerializedObject("io");
        ioFolder.checkObjectType("IoFolder");

        this->updateFolder(ioFolder,
                           "IoFolder",
                           "",
                           [this](const std::string& localId, const SerializedObjectPtr& obj) { updateIoFolderItem(this->ioFolder, localId, obj); });
    }

    const auto keys = obj.getKeys();
    for (const auto& key: keys)
    {
        if (!this->defaultComponents.count(key))
        {
            auto compIterator = std::find_if(
                this->components.begin(), this->components.end(), [&key](const ComponentPtr& comp) { return comp.getLocalId() == key; });
            if (compIterator != this->components.end())
            {
                const auto componentObject = obj.readSerializedObject(key);
                const auto updatableComponent = compIterator->template asPtr<IUpdatable>(true);
                updatableComponent.update(componentObject);
            }
        }
    }
}

template <typename TInterface, typename... Interfaces>
bool GenericDevice<TInterface, Interfaces...>::clearFunctionBlocksOnUpdate()
{
    return true;
}


END_NAMESPACE_OPENDAQ
