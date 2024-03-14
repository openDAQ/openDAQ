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
#include <opendaq/search_filter_factory.h>
#include <opendaq/io_folder_factory.h>
#include <coreobjects/property_object_impl.h>
#include <coretypes/validation.h>
#include <opendaq/device_private.h>
#include <tsl/ordered_set.h>
#include <opendaq/component_keys.h>
#include <opendaq/core_opendaq_event_args_factory.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/module_manager_ptr.h>
#include <set>

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
                  const StringPtr& className = nullptr);

    virtual DeviceInfoPtr onGetInfo();

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
    ErrCode INTERFACE_FUNC getSignals(IList** signals, ISearchFilter* searchFilter = nullptr) override;
    ErrCode INTERFACE_FUNC getSignalsRecursive(IList** signals, ISearchFilter* searchFilter = nullptr) override;
    ErrCode INTERFACE_FUNC getChannels(IList** channels, ISearchFilter* searchFilter = nullptr) override;
    ErrCode INTERFACE_FUNC getChannelsRecursive(IList** channels, ISearchFilter* searchFilter = nullptr) override;

    // IDevicePrivate
    ErrCode INTERFACE_FUNC setAsRoot() override;

    // Function block devices
    ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) override;
    ErrCode INTERFACE_FUNC addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config) override;
    ErrCode INTERFACE_FUNC removeFunctionBlock(IFunctionBlock* functionBlock) override;
    ErrCode INTERFACE_FUNC getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter = nullptr) override;

    // Client devices
    ErrCode INTERFACE_FUNC getAvailableDevices(IList** availableDevices) override;
    ErrCode INTERFACE_FUNC getAvailableDeviceTypes(IDict** deviceTypes) override;
    ErrCode INTERFACE_FUNC addDevice(IDevice** device, IString* connectionString, IPropertyObject* config = nullptr) override;
    ErrCode INTERFACE_FUNC removeDevice(IDevice* device) override;
    ErrCode INTERFACE_FUNC getDevices(IList** subDevices, ISearchFilter* searchFilter = nullptr) override;

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
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    DeviceInfoPtr deviceInfo;
    FolderConfigPtr devices;
    IoFolderConfigPtr ioFolder;
    LoggerComponentPtr loggerComponent;
    bool isRootDevice;
    UnitPtr domainUnit;
    RatioPtr domainTickResolution;
    StringPtr domainOrigin;

    template <class ChannelImpl, class... Params>
    ChannelPtr createAndAddChannel(const FolderConfigPtr& parentFolder, const StringPtr& localId, Params&&... params);
    void removeChannel(const FolderConfigPtr& parentFolder, const ChannelPtr& channel);
    bool hasChannel(const FolderConfigPtr& parentFolder, const ChannelPtr& channel);

    void addSubDevice(const DevicePtr& device);
    void removeSubDevice(const DevicePtr& device);

    DevicePtr createAndAddSubDevice(const StringPtr& connectionString, const PropertyObjectPtr& config);

    IoFolderConfigPtr addIoFolder(const std::string& localId,
                                  const IoFolderConfigPtr& parent = nullptr);

    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override;
    void updateFunctionBlock(const std::string& fbId,
                                  const SerializedObjectPtr& serializedFunctionBlock) override;
    void updateDevice(const std::string& deviceId, const SerializedObjectPtr& serializedDevice);
    void updateIoFolderItem(const FolderPtr& ioFolder,
                            const std::string& localId,
                            const SerializedObjectPtr& item);

    void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                       const BaseObjectPtr& context,
                                       const FunctionPtr& factoryCallback) override;

    void updateObject(const SerializedObjectPtr& obj) override;
    bool clearFunctionBlocksOnUpdate() override;

private:
    void getChannelsFromFolder(ListPtr<IChannel>& channelList, const FolderPtr& folder, const SearchFilterPtr& searchFilter, bool filterChannels = true);
    ListPtr<ISignal> getSignalsRecursiveInternal(const SearchFilterPtr& searchFilter);
    ListPtr<IChannel> getChannelsRecursiveInternal(const SearchFilterPtr& searchFilter);
    ListPtr<IFunctionBlock> getFunctionBlocksRecursive(const SearchFilterPtr& searchFilter);
    ListPtr<IDevice> getDevicesRecursive(const SearchFilterPtr& searchFilter);

    std::unordered_map<std::string, size_t> functionBlockCountMap;
};

template <typename TInterface, typename... Interfaces>
GenericDevice<TInterface, Interfaces...>::GenericDevice(const ContextPtr& ctx,
                                                        const ComponentPtr& parent,
                                                        const StringPtr& localId,
                                                        const StringPtr& className)
    : Super(ctx, parent, localId, className)
    , loggerComponent(this->context.getLogger().assigned() ? this->context.getLogger().getOrAddComponent(this->globalId)
                                                           : throw ArgumentNullException("Logger must not be null"))
    , isRootDevice(false)

{
    this->defaultComponents.insert("Dev");
    this->defaultComponents.insert("IO");
    this->allowNonDefaultComponents = true;

    devices = this->template addFolder<IDevice>("Dev", nullptr);
    ioFolder = this->addIoFolder("IO", nullptr);

    devices.asPtr<IComponentPrivate>().lockAllAttributes();
    ioFolder.asPtr<IComponentPrivate>().lockAllAttributes();

    this->addProperty(StringProperty("UserName", ""));
    this->addProperty(StringProperty("Location", ""));
}

template <typename TInterface, typename... Interfaces>
DeviceInfoPtr GenericDevice<TInterface, Interfaces...>::onGetInfo()
{
    return deviceInfo;
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

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getDomain(IDeviceDomain** deviceDomain)
{
    OPENDAQ_PARAM_NOT_NULL(deviceDomain);

    *deviceDomain = this->template thisInterface<IDeviceDomain>();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::setAsRoot()
{
    std::scoped_lock lock(this->sync);

    this->isRootDevice = true;
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
ErrCode GenericDevice<TInterface, Interfaces...>::getSignals(IList** signals, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    if (!searchFilter)
        return this->signals->getItems(signals);
    
    const auto searchFilterPtr = SearchFilterPtr::Borrow(searchFilter);
    if(searchFilterPtr.asPtrOrNull<IRecursiveSearch>().assigned())
    {
        return daqTry([&]
        {
            *signals = getSignalsRecursiveInternal(searchFilter).detach();
            return OPENDAQ_SUCCESS;
        });
    }

    return this->signals->getItems(signals, searchFilter);
}

template <typename TInterface, typename ... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getSignalsRecursive(IList** signals, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(signals);
    return daqTry([&]
    {
        SearchFilterPtr filter;
        if (!searchFilter)
            filter = search::Recursive(search::Visible());
        else
            filter = search::Recursive(searchFilter);

        *signals = getSignalsRecursiveInternal(filter).detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ListPtr<ISignal> GenericDevice<TInterface, Interfaces...>::getSignalsRecursiveInternal(const SearchFilterPtr& searchFilter)
{
    tsl::ordered_set<SignalPtr, ComponentHash, ComponentEqualTo> allSignals;

    ListPtr<IChannel> chList = List<IChannel>();
    getChannelsFromFolder(chList, this->ioFolder, searchFilter, false);
    for (const auto& ch : chList)
        if (searchFilter.visitChildren(ch))
            for (const auto& signal : ch.getSignals(searchFilter))
                allSignals.insert(signal);

    for (const SignalPtr& devSig : this->signals.getItems(searchFilter))
        allSignals.insert(devSig);

    for (const FunctionBlockPtr& functionBlock : this->functionBlocks.getItems(search::Any()))
        if (searchFilter.visitChildren(functionBlock))
            for (const auto& signal : functionBlock.getSignals(searchFilter))
                allSignals.insert(signal);

    for (const DevicePtr& device : this->devices.getItems(search::Any()))
        if (searchFilter.visitChildren(device))
            for (const auto& signal : device.getSignals(searchFilter))
                allSignals.insert(signal);

    // Copy tsl set to openDAQ list
    auto signalList = List<ISignal>();
    for (const auto& signal : allSignals)
        signalList.pushBack(signal);

    return signalList;
}

template <typename TInterface, typename... Interfaces>
template <class ChannelImpl, class... Params>
ChannelPtr GenericDevice<TInterface, Interfaces...>::createAndAddChannel(const FolderConfigPtr& parentFolder,
                                                                         const StringPtr& localId,
                                                                         Params&&... params)
{
    auto ch = createWithImplementation<IChannel, ChannelImpl>(
        this->context,
        parentFolder,
        localId,
        std::forward<Params>(params)...
    );

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
bool GenericDevice<TInterface, Interfaces...>::hasChannel(const FolderConfigPtr& parentFolder, const ChannelPtr& channel)
{
    if (parentFolder == nullptr)
    {
        const auto folder = channel.getParent().asPtr<IFolderConfig>();
        return folder.hasItem(channel);
    }
    else
        return parentFolder.hasItem(channel);
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
    return domainTickResolution;
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
    return 0;
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
    if (domainOrigin.assigned())
        return domainOrigin.toStdString();

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
ErrCode GenericDevice<TInterface, Interfaces...>::Deserialize(ISerializedObject* serialized,
                                                              IBaseObject* context,
                                                              IFunction* factoryCallback,
                                                              IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::DeserializeComponent(
                serialized,
                context,
                factoryCallback,
                [](const SerializedObjectPtr&,
                   const ComponentDeserializeContextPtr& deserializeContext,
                   const StringPtr& className) -> BaseObjectPtr
                {
                   return createWithImplementation<IDevice, Device>(
                       deserializeContext.getContext(),
                       deserializeContext.getParent(),
                       deserializeContext.getLocalId(),
                       className);
                }).detach();
        });
}

template <typename TInterface, typename... Interfaces>
UnitPtr GenericDevice<TInterface, Interfaces...>::onGetDomainUnit()
{
    return domainUnit;
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
    std::scoped_lock lock(this->sync);
    auto availableTypes = Dict<IString, IFunctionBlockType>();

    if (!this->isRootDevice)
        return availableTypes;

    ObjectPtr<IBaseObject> obj;
    checkErrorInfo(this->context->getModuleManager(&obj));

    if (obj == nullptr)
        throw NotAssignedException{"Module Manager is not available in the Context."};

    IModuleManager* manager;
    obj->borrowInterface(IModuleManager::Id, reinterpret_cast<void**>(&manager));

    ListPtr<IBaseObject> modules;
    manager->getModules(&modules);


    for (const auto& moduleObj : modules)
    {
        DictPtr<IString, IFunctionBlockType> moduleFbTypes;
        IModule* module;
        moduleObj->borrowInterface(IModule::Id, reinterpret_cast<void**>(&module));

        DictPtr<IString, IFunctionBlockType> types;
        ErrCode err = module->getAvailableFunctionBlockTypes(&types);
        if (OPENDAQ_FAILED(err))
        {
            StringPtr moduleName;
            module->getName(&moduleName);
            if (err == OPENDAQ_ERR_NOTIMPLEMENTED)
            {
                LOG_I("{}: GetAvailableFunctionBlockTypes not implemented", moduleName)
            }
            else
            {
                LOG_W("{}: GetAvailableFunctionBlockTypes failed", moduleName)
            }
        }

        if (!types.assigned())
            continue;

        for (const auto& [id, type] : types)
            availableTypes.set(id, type);
    }

    return availableTypes.detach();
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
FunctionBlockPtr GenericDevice<TInterface, Interfaces...>::onAddFunctionBlock(const StringPtr& typeId,
                                                                              const PropertyObjectPtr& config)
{
    std::scoped_lock lock(this->sync);
    if (!this->isRootDevice)
        return nullptr;

    ObjectPtr<IBaseObject> obj;
    this->context->getModuleManager(&obj);

    if (obj == nullptr)
        throw NotAssignedException{"Module Manager is not available in the Context."};

    IModuleManager* manager;
    obj->borrowInterface(IModuleManager::Id, reinterpret_cast<void**>(&manager));

    ListPtr<IBaseObject> modules;
    manager->getModules(&modules);

    for (const BaseObjectPtr& moduleObj : modules)
    {
        IModule* module;
        moduleObj->borrowInterface(IModule::Id, reinterpret_cast<void**>(&module));

        DictPtr<IString, IFunctionBlockType> types;
        ErrCode err = module->getAvailableFunctionBlockTypes(&types);
        if (OPENDAQ_FAILED(err))
        {
            StringPtr moduleName;
            module->getName(&moduleName);
            if (err == OPENDAQ_ERR_NOTIMPLEMENTED)
            {
                LOG_I("{}: GetAvailableFunctionBlockTypes not implemented", moduleName)
            }
            else
            {
                LOG_W("{}: GetAvailableFunctionBlockTypes failed", moduleName)
            }
        }

        if (!types.assigned())
            continue;
        if (!types.hasKey(typeId))
            continue;

        std::string localId;
        if (config.assigned() && config.hasProperty("LocalId"))
        {
            localId = static_cast<std::string>(config.getPropertyValue("LocalId"));
        }
        else
        {
            if (!functionBlockCountMap.count(typeId))
                functionBlockCountMap.insert(std::pair<std::string, size_t>(typeId, 0));

            localId = fmt::format("{}_{}", typeId, functionBlockCountMap[typeId]++);
        }

        FunctionBlockPtr fb;
        module->createFunctionBlock(&fb, typeId, this->functionBlocks, String(localId), config);
        this->functionBlocks.addItem(fb);
        return fb;
    }

    throw NotFoundException{"Function block with given uid is not available."};
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
void GenericDevice<TInterface, Interfaces...>::onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock)
{
    if (!this->isRootDevice)
        throw NotFoundException("Function block not found");

    this->functionBlocks.removeItem(functionBlock);
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlocks);

    if (!searchFilter)
        return this->functionBlocks->getItems(functionBlocks);
    
    const auto searchFilterPtr = SearchFilterPtr::Borrow(searchFilter);
    if(searchFilterPtr.asPtrOrNull<IRecursiveSearch>().assigned())
    {
        return daqTry([&]
        {
            *functionBlocks = getFunctionBlocksRecursive(searchFilter).detach();
            return OPENDAQ_SUCCESS;
        });
    }

    return this->functionBlocks->getItems(functionBlocks, searchFilter);
}

template <typename TInterface, typename ... Interfaces>
ListPtr<IFunctionBlock> GenericDevice<TInterface, Interfaces...>::getFunctionBlocksRecursive(const SearchFilterPtr& searchFilter)
{
    tsl::ordered_set<FunctionBlockPtr, ComponentHash, ComponentEqualTo> allFbs;

    for (const FunctionBlockPtr& fb : this->functionBlocks.getItems(search::Any()))
        if (searchFilter.acceptsComponent(fb))
            allFbs.insert(fb);

    for (const DevicePtr& dev : this->devices.getItems(search::Any()))
        if (searchFilter.visitChildren(dev))
            for (const auto& fb : dev.getFunctionBlocks(searchFilter))
                allFbs.insert(fb);

    auto fbList = List<IFunctionBlock>();
    for (const auto& ch : allFbs)
        fbList.pushBack(ch);

    return fbList;
}

template <typename TInterface, typename... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getChannels(IList** channels, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(channels);

    if (!searchFilter)
    {
        ListPtr<IChannel> chList = List<IChannel>();
        getChannelsFromFolder(chList, this->ioFolder, search::Visible());
        *channels = chList.detach();
        return OPENDAQ_SUCCESS;
    }
    
    const auto searchFilterPtr = SearchFilterPtr::Borrow(searchFilter);
    if(searchFilterPtr.asPtrOrNull<IRecursiveSearch>().assigned())
    {
        return daqTry([&]
        {
            *channels = getChannelsRecursiveInternal(searchFilter).detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ListPtr<IChannel> chList = List<IChannel>();
    getChannelsFromFolder(chList, this->ioFolder, searchFilter);
    *channels = chList.detach();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename ... Interfaces>
ErrCode GenericDevice<TInterface, Interfaces...>::getChannelsRecursive(IList** channels, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(channels);
    return daqTry([&]
    {
        SearchFilterPtr filter;
        if (!searchFilter)
            filter = search::Recursive(search::Visible());
        else
            filter = search::Recursive(searchFilter);

        *channels = getChannelsRecursiveInternal(filter).detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename ... Interfaces>
ListPtr<IChannel> GenericDevice<TInterface, Interfaces...>::getChannelsRecursiveInternal(const SearchFilterPtr& searchFilter)
{
    tsl::ordered_set<ChannelPtr, ComponentHash, ComponentEqualTo> allChannels;

    ListPtr<IChannel> chList = List<IChannel>();
    getChannelsFromFolder(chList, this->ioFolder, searchFilter);
    for (const ChannelPtr& ch : chList)
        allChannels.insert(ch);

    for (const DevicePtr& dev : this->devices.getItems(search::Any()))
        if (searchFilter.visitChildren(dev))
            for (const ChannelPtr& ch : dev.getChannels(searchFilter))
                allChannels.insert(ch);

    chList = List<IChannel>();
    for (const auto& ch : allChannels)
        chList.pushBack(ch);

    return chList;
}

template <typename TInterface, typename... Interfaces>
void GenericDevice<TInterface, Interfaces...>::getChannelsFromFolder(ListPtr<IChannel>& channelList, const FolderPtr& folder, const SearchFilterPtr& searchFilter, bool filterChannels)
{
    for (const auto& item : folder.getItems(search::Any()))
    {
        if (item.supportsInterface<IChannel>())
        {
            if (filterChannels && !searchFilter.acceptsComponent(item))
                continue;

            channelList.pushBack(item);
        }
        else if (item.supportsInterface<IIoFolderConfig>() && searchFilter.visitChildren(item))
            getChannelsFromFolder(channelList, item, searchFilter, filterChannels);
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
ErrCode GenericDevice<TInterface, Interfaces...>::getDevices(IList** subDevices, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(subDevices);

    if (!searchFilter)
        return devices->getItems(subDevices);
    
    const auto searchFilterPtr = SearchFilterPtr::Borrow(searchFilter);
    if(searchFilterPtr.asPtrOrNull<IRecursiveSearch>().assigned())
    {
        return daqTry([&]
        {
            *subDevices = getDevicesRecursive(searchFilter).detach();
            return OPENDAQ_SUCCESS;
        });
    }

    return devices->getItems(subDevices, searchFilter);
}

template <typename TInterface, typename ... Interfaces>
ListPtr<IDevice> GenericDevice<TInterface, Interfaces...>::getDevicesRecursive(const SearchFilterPtr& searchFilter)
{
    tsl::ordered_set<DevicePtr, ComponentHash, ComponentEqualTo> allDevices;

    for (const DevicePtr& dev : this->devices.getItems(search::Any()))
        if (searchFilter.acceptsComponent(dev))
            allDevices.insert(dev);

    for (const DevicePtr& dev : this->devices.getItems(search::Any()))
        if (searchFilter.visitChildren(dev))
            for (const DevicePtr& childDev : dev.getDevices(searchFilter))
                allDevices.insert(childDev);

    auto devList = List<IDevice>();
    for (const auto& dev : allDevices)
        devList.pushBack(dev);

    return devList;
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
DevicePtr GenericDevice<TInterface, Interfaces...>::createAndAddSubDevice(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    ModuleManagerPtr manager = this->context.getModuleManager().template asPtr<IModuleManager>();
    for (const auto module: manager.getModules())
    {
        bool accepted;
        try
        {
            accepted = module.acceptsConnectionParameters(connectionString, config);
        }
        catch (NotImplementedException &)
        {
            LOG_I("{}: AcceptsConnectionString not implemented", module.getName())
            accepted = false;
        }
        catch (const std::exception &e)
        {
            LOG_W("{}: AcceptsConnectionString failed: {}", module.getName(), e.what())
            accepted = false;
        }

        if (accepted)
        {
            auto device = module.createDevice(connectionString, devices, config);
            addSubDevice(device);

            return device;
        }
    }

    return nullptr;
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
                                                                               const IoFolderConfigPtr& parent)
{
    if (!parent.assigned())
    {
        this->validateComponentNotExists(localId);

        auto folder = IoFolder(this->context, this->template thisPtr<ComponentPtr>(), localId);
        this->components.push_back(folder);

        if (!this->coreEventMuted && this->coreEvent.assigned())
        {
             this->triggerCoreEvent(CoreEventArgsComponentAdded(folder));
             folder.template asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
        }

        return folder;
    }

    auto folder = IoFolder(this->context, parent, localId);
    parent.addItem(folder);
    return folder;
}

template <typename TInterface, typename ... Interfaces>
void GenericDevice<TInterface, Interfaces...>::serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate)
{
    Super::serializeCustomObjectValues(serializer, forUpdate);

    this->serializeFolder(serializer, ioFolder, "IO", forUpdate);
    this->serializeFolder(serializer, devices, "Dev", forUpdate);

    for (const auto& component : this->components)
    {
        if (!this->defaultComponents.count(component.getLocalId()))
        {
            serializer->key(component.getLocalId().getCharPtr());
            if (forUpdate)
                component.template asPtr<IUpdatable>(true).serializeForUpdate(serializer);
            else
                component.serialize(serializer);
        }
    }

    if (!forUpdate)
    {
        DeviceInfoPtr deviceInfo;
        checkErrorInfo(this->getInfo(&deviceInfo));
        if (deviceInfo.assigned())
        {
            serializer.key("deviceInfo");
            deviceInfo.serialize(serializer);
        }

        DeviceDomainPtr deviceDomain;
        checkErrorInfo(this->getDomain(&deviceDomain));
        if (deviceDomain.assigned())
        {
            const auto origin = deviceDomain.getOrigin();
            if (origin.assigned())
            {
                serializer.key("domainOrigin");
                serializer.writeString(deviceDomain.getOrigin());
            }

            const auto resolution = deviceDomain.getTickResolution();
            if (resolution.assigned())
            {
                serializer.key("domainResolution");
                resolution.serialize(serializer);
            }

            const auto unit = deviceDomain.getUnit();
            if (unit.assigned())
            {
                serializer.key("domainUnit");
                unit.serialize(serializer);
            }
        }
    }
}

template <typename TInterface, typename... Interfaces>
void GenericDevice<TInterface, Interfaces...>::updateFunctionBlock(const std::string& fbId,
                                                                   const SerializedObjectPtr& serializedFunctionBlock)
{
    UpdatablePtr updatableFb;
    if (!this->functionBlocks.hasItem(fbId))
    {
        auto typeId = serializedFunctionBlock.readString("typeId");

        auto config = PropertyObject();
        config.addProperty(StringProperty("LocalId", fbId));

        auto fb = onAddFunctionBlock(typeId, config);
        updatableFb = fb.template asPtr<IUpdatable>(true);
    }
    else
    {
        updatableFb = this->functionBlocks.getItem(fbId).template asPtr<IUpdatable>(true);
    }

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
void GenericDevice<TInterface, Interfaces...>::deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                                                             const BaseObjectPtr& context,
                                                                             const FunctionPtr& factoryCallback)
{
    Super::deserializeCustomObjectValues(serializedObject, context, factoryCallback);

    if (serializedObject.hasKey("deviceInfo"))
    {
        deviceInfo = serializedObject.readObject("deviceInfo");
        deviceInfo.freeze();
    }

    if (serializedObject.hasKey("domainOrigin"))
        domainOrigin = serializedObject.readString("domainOrigin");

    if (serializedObject.hasKey("domainResolution"))
        domainTickResolution = serializedObject.readObject("domainResolution");

    if (serializedObject.hasKey("domainUnit"))
        domainUnit = serializedObject.readObject("domainUnit");

    this->template deserializeDefaultFolder<IComponent>(serializedObject, context, factoryCallback, ioFolder, "IO");
    this->template deserializeDefaultFolder<IDevice>(serializedObject, context, factoryCallback, devices, "Dev");

    const std::set<std::string> ignoredKeys{"__type", "deviceInfo", "deviceDomain", "deviceUnit", "deviceResolution", "properties", "propValues"};

    const auto keys = serializedObject.getKeys();
    for (const auto& key : serializedObject.getKeys())
    {
        if (!this->defaultComponents.count(key) && !ignoredKeys.count(key) && serializedObject.getType(key) == ctObject)
        {
            const auto deserializeContext = context.asPtr<IComponentDeserializeContext>(true);
            const auto newDeserializeContext = deserializeContext.clone(this->template borrowPtr<ComponentPtr>(), key, nullptr);
            const BaseObjectPtr obj = serializedObject.readObject(key, newDeserializeContext, factoryCallback);
            if (const auto component = obj.asPtrOrNull<IComponent>(); component.assigned())
                this->addExistingComponent(component);
        }
    }
}

template <typename TInterface, typename... Interfaces>
void GenericDevice<TInterface, Interfaces...>::updateObject(const SerializedObjectPtr& obj)
{
    Super::updateObject(obj);

    if (obj.hasKey("Dev"))
    {
        const auto devicesFolder = obj.readSerializedObject("Dev");
        devicesFolder.checkObjectType("Folder");

        this->updateFolder(devicesFolder,
                           "Folder",
                           "Device",
                           [this](const std::string& localId, const SerializedObjectPtr& obj)
                           { updateDevice(localId, obj); });
    }

    if (obj.hasKey("IO"))
    {
        const auto ioFolder = obj.readSerializedObject("IO");
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

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(Device)

END_NAMESPACE_OPENDAQ
