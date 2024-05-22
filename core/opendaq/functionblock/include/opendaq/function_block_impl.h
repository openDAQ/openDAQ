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
#include <opendaq/context_ptr.h>
#include <opendaq/folder_factory.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/input_port_notifications.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/signal_container_impl.h>
#include <opendaq/search_filter_factory.h>
#include <coreobjects/property_object_factory.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IFunctionBlock, typename... Interfaces>
class FunctionBlockImpl;

using FunctionBlock = FunctionBlockImpl<>;

template <typename TInterface, typename... Interfaces>
class FunctionBlockImpl : public SignalContainerImpl<TInterface, IInputPortNotifications, Interfaces...>
{
public:
    using Self = FunctionBlockImpl<TInterface, Interfaces...>;
    using Super = SignalContainerImpl<TInterface, IInputPortNotifications, Interfaces...>;

    FunctionBlockImpl(const FunctionBlockTypePtr& type,
                      const ContextPtr& context,
                      const ComponentPtr& parent,
                      const StringPtr& localId,
                      const StringPtr& className = nullptr,
                      const PropertyObjectPtr& config = nullptr);

    ErrCode INTERFACE_FUNC getFunctionBlockType(IFunctionBlockType** type) override;

    ErrCode INTERFACE_FUNC getInputPorts(IList** ports, ISearchFilter* searchFilter = nullptr) override;
    ErrCode INTERFACE_FUNC getSignals(IList** signals, ISearchFilter* searchFilter = nullptr) override;
    ErrCode INTERFACE_FUNC getSignalsRecursive(IList** signals, ISearchFilter* searchFilter = nullptr) override;
    ErrCode INTERFACE_FUNC getStatusSignal(ISignal** statusSignal) override;
    ErrCode INTERFACE_FUNC getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter = nullptr) override;

    // IInputPortNotifications
    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept) override;
    ErrCode INTERFACE_FUNC connected(IInputPort* port) override;
    ErrCode INTERFACE_FUNC disconnected(IInputPort* port) override;
    ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IBaseObject** obj);

    // IBaseObject
    ErrCode INTERFACE_FUNC queryInterface(const IntfID& id, void** intf) override;
    ErrCode INTERFACE_FUNC borrowInterface(const IntfID& id, void** intf) const override;

    virtual SignalPtr onGetStatusSignal();

    virtual bool onAcceptsSignal(const InputPortPtr& port, const SignalPtr& signal);
    virtual void onConnected(const InputPortPtr& port);
    virtual void onDisconnected(const InputPortPtr& port);
    virtual void onPacketReceived(const InputPortPtr& port);

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    FunctionBlockTypePtr type;
    LoggerComponentPtr loggerComponent;
    FolderConfigPtr inputPorts;
    PropertyObjectPtr config;

    InputPortConfigPtr createAndAddInputPort(const std::string& localId,
                                             PacketReadyNotification notificationMethod,
                                             BaseObjectPtr customData = nullptr,
                                             bool requestGapPackets = false);

    void addInputPort(const InputPortPtr& inputPort);
    void removeInputPort(const InputPortConfigPtr& inputPort);

    void removed() override;

    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override;
    void updateInputPort(const std::string& localId, const SerializedObjectPtr& obj);
    void updateFunctionBlock(const std::string& fbId, const SerializedObjectPtr& serializedFunctionBlock) override;

    void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                       const BaseObjectPtr& context,
                                       const FunctionPtr& factoryCallback) override;

    void updateObject(const SerializedObjectPtr& obj) override;
    void initConfigObject(const daq::PropertyObjectPtr& userConfig);

    template <class Impl>
    static BaseObjectPtr DeserializeFunctionBlock(const SerializedObjectPtr& serialized,
                                                  const BaseObjectPtr& context,
                                                  const FunctionPtr& factoryCallback);
private:
    ListPtr<ISignal> getSignalsRecursiveInternal(const SearchFilterPtr& searchFilter);
    ListPtr<IFunctionBlock> getFunctionBlocksRecursiveInternal(const SearchFilterPtr& searchFilter);
    ListPtr<IInputPort> getInputPortsRecursiveInternal(const SearchFilterPtr& searchFilter);
};



template <typename TInterface, typename... Interfaces>
FunctionBlockImpl<TInterface, Interfaces...>::FunctionBlockImpl(const FunctionBlockTypePtr& type,
                                                                const ContextPtr& context,
                                                                const ComponentPtr& parent,
                                                                const StringPtr& localId,
                                                                const StringPtr& className,
                                                                const PropertyObjectPtr& config)
    : Super(context, parent, localId, className)
    , type(type)
    , loggerComponent(this->context.getLogger().assigned() ? this->context.getLogger().getOrAddComponent(this->globalId)
                                                           : throw ArgumentNullException("Logger must not be null"))
{
    initConfigObject(config);

    this->defaultComponents.insert("IP");
    inputPorts = this->template addFolder<IInputPort>("IP", nullptr);
    inputPorts.asPtr<IComponentPrivate>().lockAllAttributes();
    inputPorts.asPtr<IComponentPrivate>().unlockAttributes(List<IString>("Active"));
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::getFunctionBlockType(IFunctionBlockType** type)
{
    if (type == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *type = this->type.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::getSignals(IList** signals, ISearchFilter* searchFilter)
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
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::getSignalsRecursive(IList** signals, ISearchFilter* searchFilter)
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
ListPtr<ISignal> FunctionBlockImpl<TInterface, Interfaces...>::getSignalsRecursiveInternal(const SearchFilterPtr& searchFilter)
{
    tsl::ordered_set<SignalPtr, ComponentHash, ComponentEqualTo> allSignals;

    for (const auto& sig : this->signals.getItems(searchFilter))
        allSignals.insert(sig.template asPtr<ISignal>());

    for (const auto& functionBlock : this->functionBlocks.getItems(search::Any()))
        if (searchFilter.visitChildren(functionBlock))
            for (const auto& signal : functionBlock.template asPtr<IFunctionBlock>().getSignals(searchFilter))
                allSignals.insert(signal);

    auto signalList = List<ISignal>();
    for (const auto& signal : allSignals)
        signalList.pushBack(signal);

    return signalList;
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::getInputPorts(IList** ports, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(ports);

    if (!searchFilter)
        return this->inputPorts->getItems(ports);

    const auto searchFilterPtr = SearchFilterPtr::Borrow(searchFilter);
    if(searchFilterPtr.asPtrOrNull<IRecursiveSearch>().assigned())
    {
        return daqTry([&]
        {
            *ports = getInputPortsRecursiveInternal(searchFilter).detach();
            return OPENDAQ_SUCCESS;
        });
    }

    return this->inputPorts->getItems(ports, searchFilter);
}

template <typename TInterface, typename... Interfaces>
ListPtr<IInputPort> FunctionBlockImpl<TInterface, Interfaces...>::getInputPortsRecursiveInternal(const SearchFilterPtr& searchFilter)
{
    tsl::ordered_set<InputPortPtr, ComponentHash, ComponentEqualTo> allPorts;

    for (const auto& ip : this->inputPorts.getItems(searchFilter))
        allPorts.insert(ip.template asPtr<IInputPort>());

    for (const auto& functionBlock : this->functionBlocks.getItems(search::Any()))
        if (searchFilter.visitChildren(functionBlock))
            for (const auto& ip : functionBlock.template asPtr<IFunctionBlock>().getInputPorts(searchFilter))
                allPorts.insert(ip);

    auto portsList = List<IInputPort>();
    for (const auto& port : allPorts)
        portsList.pushBack(port);

    return portsList;
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::getStatusSignal(ISignal** statusSignal)
{
    if (statusSignal == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    SignalPtr statusSig;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetStatusSignal, statusSig);

    *statusSignal = statusSig.detach();

    return errCode;
}

template <typename TInterface, typename... Interfaces>
void FunctionBlockImpl<TInterface, Interfaces...>::updateObject(const SerializedObjectPtr& obj)
{
    if (obj.hasKey("IP"))
    {
        const auto ipFolder = obj.readSerializedObject("IP");
        this->updateFolder(ipFolder,
                     "Folder",                    
                     "InputPort",
                     [this](const std::string& localId, const SerializedObjectPtr& obj)
                     { updateInputPort(localId, obj); });
    }

    return Super::updateObject(obj);
}

template <typename TInterface, typename... Interfaces>
void FunctionBlockImpl<TInterface, Interfaces...>::initConfigObject(const daq::PropertyObjectPtr& userConfig)
{
    if (!type.assigned())
    {
        config = PropertyObject();
        return;
    }

    config = type.createDefaultConfig();

    if (!userConfig.assigned())
        return;

    for (const auto& prop : config.getAllProperties())
    {
        const auto name = prop.getName();

        if (userConfig.hasProperty(name))
            config.setPropertyValue(name, userConfig.getPropertyValue(name));
    }
}

template <class Intf, class... Intfs>
void FunctionBlockImpl<Intf, Intfs...>::updateInputPort(const std::string& localId,
                                                             const SerializedObjectPtr& obj)
{
    InputPortPtr inputPort;
    if (!inputPorts.hasItem(localId))
    {
        LOG_W("Input port {} not found", localId);
        for (const auto& ip : inputPorts.getItems(search::Any()))
        {
            inputPort = ip.template asPtr<IInputPort>(true);
            if (!inputPort.getSignal().assigned())
            {
                LOG_W("Using input port {}", inputPort.getLocalId());
                break;
            }

        }
        if (!inputPort.assigned())
            return;
    }
    else
        inputPort = inputPorts.getItem(localId);

    const auto updatableIp = inputPort.asPtr<IUpdatable>(true);

    updatableIp.update(obj);
}

template <typename TInterface, typename ... Interfaces>
void FunctionBlockImpl<TInterface, Interfaces...>::updateFunctionBlock(const std::string& fbId,
    const SerializedObjectPtr& serializedFunctionBlock)
{
    if (!this->functionBlocks.hasItem(fbId))
    {
        DAQLOGF_W(loggerComponent,
                  "Sub function block "
                  "{}"
                  "not found",
                  fbId);
        return;
    }

    const auto fb = this->functionBlocks.getItem(fbId);

    const auto updatableFb = fb.template asPtr<IUpdatable>(true);

    updatableFb.update(serializedFunctionBlock);
}

template <typename TInterface, typename... Interfaces>
void FunctionBlockImpl<TInterface, Interfaces...>::deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                                                                 const BaseObjectPtr& context,
                                                                                 const FunctionPtr& factoryCallback)
{
    Super::deserializeCustomObjectValues(serializedObject, context, factoryCallback);

    this->template deserializeDefaultFolder<IInputPort>(serializedObject, context, factoryCallback, this->inputPorts, "IP");
}

template <typename TInterface, typename... Interfaces>
SignalPtr FunctionBlockImpl<TInterface, Interfaces...>::onGetStatusSignal()
{
    return nullptr;
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlocks);

    if (!searchFilter)
        return this->functionBlocks->getItems(functionBlocks);
    
    const auto searchFilterPtr = SearchFilterPtr::Borrow(searchFilter);
    if(searchFilterPtr.asPtrOrNull<IRecursiveSearch>().assigned())
    {
        return daqTry([&]
        {
            *functionBlocks = getFunctionBlocksRecursiveInternal(searchFilter).detach();
            return OPENDAQ_SUCCESS;
        });
    }

    return this->functionBlocks->getItems(functionBlocks, searchFilter);
}

template <typename TInterface, typename... Interfaces>
ListPtr<IFunctionBlock> FunctionBlockImpl<TInterface, Interfaces...>::getFunctionBlocksRecursiveInternal(const SearchFilterPtr& searchFilter)
{
    tsl::ordered_set<FunctionBlockPtr, ComponentHash, ComponentEqualTo> allFbs;

    for (const auto& fb : this->functionBlocks.getItems(searchFilter))
        allFbs.insert(fb.template asPtr<IFunctionBlock>());

    for (const auto& functionBlock : this->functionBlocks.getItems(search::Any()))
        if (searchFilter.visitChildren(functionBlock))
            for (const auto& fb : functionBlock.template asPtr<IFunctionBlock>().getFunctionBlocks(searchFilter))
                allFbs.insert(fb);

    auto fbList = List<IFunctionBlock>();
    for (const auto& fb : allFbs)
        fbList.pushBack(fb);

    return fbList;
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept)
{
    if (accept == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    Bool accepted;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onAcceptsSignal, accepted, port, signal);

    *accept = accepted;

    return errCode;
}

template <typename TInterface, typename... Interfaces>
bool FunctionBlockImpl<TInterface, Interfaces...>::onAcceptsSignal(const InputPortPtr& /*port*/, const SignalPtr& /*signal*/)
{
    return true;
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::connected(IInputPort* port)
{
    const ErrCode errCode = wrapHandler(this, &Self::onConnected, port);
    return errCode;
}

template <typename TInterface, typename... Interfaces>
void FunctionBlockImpl<TInterface, Interfaces...>::onConnected(const InputPortPtr& /*port*/)
{
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::disconnected(IInputPort* port)
{
    return wrapHandler(this, &Self::onDisconnected, port);
}

template <typename TInterface, typename... Interfaces>
void FunctionBlockImpl<TInterface, Interfaces...>::onDisconnected(const InputPortPtr& /*port*/)
{
}

template <typename TInterface, typename... Interfaces>
InputPortConfigPtr FunctionBlockImpl<TInterface, Interfaces...>::createAndAddInputPort(const std::string& localId,
                                                                                       PacketReadyNotification notificationMethod,
                                                                                       BaseObjectPtr customData,
                                                                                       bool requestGapPackets)
{
    auto inputPort = InputPort(this->context, inputPorts, localId, requestGapPackets);
    inputPort.setListener(this->template borrowPtr<InputPortNotificationsPtr>());
    inputPort.setNotificationMethod(notificationMethod);
    inputPort.setCustomData(customData);

    addInputPort(inputPort);
    return inputPort;
}

template <typename TInterface, typename... Interfaces>
void FunctionBlockImpl<TInterface, Interfaces...>::addInputPort(const InputPortPtr& inputPort)
{
    if (inputPort.getParent() != inputPorts)
        throw InvalidParameterException("Invalid parent of input port");

    try
    {
        this->inputPorts.addItem(inputPort);
    }
    catch (DuplicateItemException&)
    {
        throw DuplicateItemException("Input port with the same ID already exists");
    }
}

template <typename TInterface, typename... Interfaces>
void FunctionBlockImpl<TInterface, Interfaces...>::removeInputPort(const InputPortConfigPtr& inputPort)
{
    inputPorts.removeItem(inputPort);
}

template <typename TInterface, typename... Interfaces>
void FunctionBlockImpl<TInterface, Interfaces...>::removed()
{
    Super::removed();
    inputPorts.remove();
}

template <typename TInterface, typename... Interfaces>
void FunctionBlockImpl<TInterface, Interfaces...>::serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate)
{
    serializer.key("typeId");
    auto typeId = type.getId();
    serializer.writeString(typeId.getCharPtr(), typeId.getLength());

    Super::serializeCustomObjectValues(serializer, forUpdate);
    this->serializeFolder(serializer, inputPorts, "IP", forUpdate);
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::packetReceived(IInputPort* port)
{
    const auto portPtr = InputPortPtr::Borrow(port);
    return wrapHandler(this, &Self::onPacketReceived, portPtr);
}

template <typename TInterface, typename... Interfaces>
ErrCode INTERFACE_FUNC FunctionBlockImpl<TInterface, Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ConstCharPtr FunctionBlockImpl<TInterface, Interfaces...>::SerializeId()
{
    return "FunctionBlock";
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::Deserialize(ISerializedObject* serialized, IBaseObject* context, IBaseObject** obj)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

template <typename TInterface, typename... Interfaces>
ErrCode INTERFACE_FUNC FunctionBlockImpl<TInterface, Interfaces...>::queryInterface(const IntfID& id, void** intf)
{
    OPENDAQ_PARAM_NOT_NULL(intf);

    if (id == IInputPortNotifications::Id)
    {
        *intf = static_cast<IInputPortNotifications*>(this);
        this->addRef();

        return OPENDAQ_SUCCESS;
    }

    return Super::queryInterface(id, intf);
}

template <typename TInterface, typename... Interfaces>
ErrCode INTERFACE_FUNC FunctionBlockImpl<TInterface, Interfaces...>::borrowInterface(const IntfID& id, void** intf) const
{
    OPENDAQ_PARAM_NOT_NULL(intf);

    if (id == IInputPortNotifications::Id)
    {
        *intf = const_cast<IInputPortNotifications*>(static_cast<const IInputPortNotifications*>(this));

        return OPENDAQ_SUCCESS;
    }

    return Super::borrowInterface(id, intf);
}

template <typename TInterface, typename... Interfaces>
void FunctionBlockImpl<TInterface, Interfaces...>::onPacketReceived(const InputPortPtr& /*port*/)
{
}

template <typename TInterface, typename... Interfaces>
template <class Impl>
BaseObjectPtr FunctionBlockImpl<TInterface, Interfaces...>::DeserializeFunctionBlock(
    const SerializedObjectPtr& serialized,
    const BaseObjectPtr& context,
    const FunctionPtr& factoryCallback)
{
    return Super::DeserializeComponent(
               serialized,
               context,
               factoryCallback,
               [](const SerializedObjectPtr& serialized,
                  const ComponentDeserializeContextPtr& deserializeContext,
                  const StringPtr& className) -> BaseObjectPtr
               {
                   const auto typeId = serialized.readString("typeId");

                   const auto fbType = FunctionBlockType(typeId, typeId, "", nullptr);

                   return createWithImplementation<IFunctionBlock, Impl>(
                       fbType, deserializeContext.getContext(), deserializeContext.getParent(), deserializeContext.getLocalId(), className);
               });
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::Deserialize(ISerializedObject* serialized,
    IBaseObject* context,
    IFunction* factoryCallback,
    IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = DeserializeFunctionBlock<FunctionBlock>(serialized, context, factoryCallback).detach();
        });
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(FunctionBlock)

END_NAMESPACE_OPENDAQ
