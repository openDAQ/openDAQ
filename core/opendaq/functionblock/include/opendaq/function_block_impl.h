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
#include <opendaq/folder_factory.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/input_port_notifications.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/signal_container_impl.h>

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
                      ComponentStandardProps propsMode = ComponentStandardProps::Add);

    ErrCode INTERFACE_FUNC getFunctionBlockType(IFunctionBlockType** type) override;

    ErrCode INTERFACE_FUNC getInputPorts(IList** ports) override;
    ErrCode INTERFACE_FUNC getSignals(IList** signals) override;
    ErrCode INTERFACE_FUNC getSignalsRecursive(IList** signals) override;
    ErrCode INTERFACE_FUNC getStatusSignal(ISignal** statusSignal) override;
    ErrCode INTERFACE_FUNC getFunctionBlocks(IList** functionBlocks) override;

    // IInputPortNotifications
    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept) override;
    ErrCode INTERFACE_FUNC connected(IInputPort* port) override;
    ErrCode INTERFACE_FUNC disconnected(IInputPort* port) override;
    ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IBaseObject** obj);

    virtual SignalPtr onGetStatusSignal();

    virtual bool onAcceptsSignal(const InputPortPtr& port, const SignalPtr& signal);
    virtual void onConnected(const InputPortPtr& port);
    virtual void onDisconnected(const InputPortPtr& port);
    virtual void onPacketReceived(const InputPortPtr& port);

protected:
    FunctionBlockTypePtr type;
    LoggerComponentPtr loggerComponent;
    FolderConfigPtr inputPorts;

    InputPortConfigPtr createAndAddInputPort(const std::string& localId,
                                             PacketReadyNotification notificationMethod,
                                             BaseObjectPtr customData = nullptr);
    void addInputPort(const InputPortPtr& inputPort);
    void removeInputPort(const InputPortConfigPtr& inputPort);

    void removed() override;

    void serializeCustomObjectValues(const SerializerPtr& serializer) override;
    void updateInputPort(const std::string& localId, const SerializedObjectPtr& obj);
    void deserializeFunctionBlock(const std::string& fbId, const SerializedObjectPtr& serializedFunctionBlock) override;

    void updateObject(const SerializedObjectPtr& obj) override;
};

template <typename TInterface, typename... Interfaces>
FunctionBlockImpl<TInterface, Interfaces...>::FunctionBlockImpl(const FunctionBlockTypePtr& type,
                                                                const ContextPtr& context,
                                                                const ComponentPtr& parent,
                                                                const StringPtr& localId,
                                                                const StringPtr& className,
                                                                const ComponentStandardProps propsMode)
    : Super(context, parent, localId, className, propsMode)
    , type(type)
    , loggerComponent(this->context.getLogger().assigned() ? this->context.getLogger().getOrAddComponent(this->globalId)
                                                           : throw ArgumentNullException("Logger must not be null"))
{
    this->defaultComponents.insert("IP");
    inputPorts = this->template addFolder<IInputPort>("IP", nullptr, ComponentStandardProps::Skip);
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
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::getSignals(IList** signals)
{
    if (signals == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return this->signals->getItems(signals);
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::getSignalsRecursive(IList** signals)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    return daqTry(
        [this, &signals]
        {
            auto signalList = List<ISignal>();
            auto devSignals = this->signals.getItems();
            for (const auto& sig : devSignals)
                signalList.pushBack(sig.template asPtr<ISignal>());

            auto fbs = this->functionBlocks.getItems();
            for (const auto& fb : fbs)
            {
                auto fbSigs = fb.template asPtr<IFunctionBlock>(true).getSignalsRecursive();
                for (const auto& fbSig : fbSigs)
                    signalList.pushBack(fbSig);
            }

            *signals = signalList.detach();
            return OPENDAQ_SUCCESS;
        });
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::getInputPorts(IList** ports)
{
    if (ports == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return inputPorts->getItems(ports);
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
    if (obj.hasKey("ip"))
    {
        const auto ipFolder = obj.readSerializedObject("ip");
        this->updateFolder(ipFolder,
                     "Folder",                    
                     "InputPort",
                     [this](const std::string& localId, const SerializedObjectPtr& obj)
                     { updateInputPort(localId, obj); });
    }

    return Super::updateObject(obj);
}

template <class Intf, class... Intfs>
void FunctionBlockImpl<Intf, Intfs...>::updateInputPort(const std::string& localId,
                                                             const SerializedObjectPtr& obj)
{
    InputPortPtr inputPort;
    if (!inputPorts.hasItem(localId))
    {
        LOG_W("Input port {} not found", localId);
        for (const auto& ip : inputPorts.getItems())
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
void FunctionBlockImpl<TInterface, Interfaces...>::deserializeFunctionBlock(const std::string& fbId,
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
SignalPtr FunctionBlockImpl<TInterface, Interfaces...>::onGetStatusSignal()
{
    return nullptr;
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::getFunctionBlocks(IList** functionBlocks)
{
    if (functionBlocks == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return this->functionBlocks->getItems(functionBlocks);
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
                                                                                       BaseObjectPtr customData)
{
    auto inputPort = InputPort(this->context, inputPorts, localId);
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
void FunctionBlockImpl<TInterface, Interfaces...>::serializeCustomObjectValues(const SerializerPtr& serializer)
{
    serializer.key("typeId");
    auto typeId = type.getId();
    serializer.writeString(typeId.getCharPtr(), typeId.getLength());

    Super::serializeCustomObjectValues(serializer);

    if (!inputPorts.isEmpty())
    {
        serializer.key("ip");
        inputPorts.serialize(serializer);
    }
}

template <typename TInterface, typename... Interfaces>
ErrCode FunctionBlockImpl<TInterface, Interfaces...>::packetReceived(IInputPort* port)
{
    return wrapHandler(this, &Self::onPacketReceived, port);
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
void FunctionBlockImpl<TInterface, Interfaces...>::onPacketReceived(const InputPortPtr& /*port*/)
{
}

END_NAMESPACE_OPENDAQ
