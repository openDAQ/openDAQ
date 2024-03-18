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
#include <config_protocol/config_client_component_impl.h>
#include <opendaq/input_port_impl.h>
#include <config_protocol/config_client_connection_impl.h>
#include <config_protocol/config_client_input_port.h>

namespace daq::config_protocol
{

class ConfigClientInputPortImpl : public ConfigClientComponentBaseImpl<GenericInputPortImpl<IConfigClientObject, IConfigClientInputPort>>
{
public:
    using Super = ConfigClientComponentBaseImpl<GenericInputPortImpl<IConfigClientObject, IConfigClientInputPort>>;

    ConfigClientInputPortImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                              const std::string& remoteGlobalId,
                              const ContextPtr& ctx,
                              const ComponentPtr& parent,
                              const StringPtr& localId,
                              const StringPtr& className = nullptr);

    ErrCode INTERFACE_FUNC connect(ISignal* signal) override;
    ErrCode INTERFACE_FUNC disconnect() override;

    ErrCode INTERFACE_FUNC assignSignal(ISignal* signal) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;
    void onRemoteUpdate(const SerializedObjectPtr& serialized) override;

    ConnectionPtr createConnection(const SignalPtr& signal) override;
    bool isConnected(const SignalPtr& signal);
};

inline ConfigClientInputPortImpl::ConfigClientInputPortImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                            const std::string& remoteGlobalId,
                                                            const ContextPtr& ctx,
                                                            const ComponentPtr& parent,
                                                            const StringPtr& localId,
                                                            const StringPtr& className)
    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId, className)
{
}

inline ErrCode ConfigClientInputPortImpl::connect(ISignal* signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    return daqTry(
        [this, &signal]
        {
            if (!this->deserializationComplete)
                return Super::connect(signal);

            const auto signalPtr = SignalPtr::Borrow(signal);
            {
                std::scoped_lock lock(this->sync);

                if (isConnected(signalPtr))
                    return OPENDAQ_IGNORED;
            }

            const auto configObject = signalPtr.asPtrOrNull<IConfigClientObject>(true);
            if (!configObject.assigned())
                throw InvalidParameterException("Not a remote signal");

             // TODO check that signal actually belongs to this device

            StringPtr signalRemoteGlobalId;
            checkErrorInfo(configObject->getRemoteGlobalId(&signalRemoteGlobalId));

            auto params = ParamsDict({{"SignalId", signalRemoteGlobalId}});

            clientComm->sendComponentCommand(remoteGlobalId, "ConnectSignal", params, nullptr);
            return Super::connect(signal);
        });
}

inline ErrCode ConfigClientInputPortImpl::disconnect()
{
    return daqTry(
        [this]
        {
            assert(this->deserializationComplete);

            clientComm->sendComponentCommand(remoteGlobalId, "DisconnectSignal", nullptr);
            return Super::disconnect();
        });
}

inline ErrCode ConfigClientInputPortImpl::assignSignal(ISignal* signal)
{
    if (signal == nullptr)
        return Super::disconnect();
    return Super::connect(signal);
}

inline bool ConfigClientInputPortImpl::isConnected(const SignalPtr& signal)
{
    const auto conn = this->getConnectionNoLock();
    if (conn.assigned() && conn.getSignal() == signal)
        return true;

    return false;
}

inline ErrCode ConfigClientInputPortImpl::Deserialize(ISerializedObject* serialized,
                                                      IBaseObject* context,
                                                      IFunction* factoryCallback,
                                                      IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = DeserializeConfigComponent<IInputPortConfig, ConfigClientInputPortImpl>(serialized, context, factoryCallback).detach();
        });
}

inline void ConfigClientInputPortImpl::handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args)
{
    switch (static_cast<CoreEventId>(args.getEventId()))
    {
        case CoreEventId::SignalConnected:
            {
                const SignalPtr signal = args.getParameters().get("Signal");
                checkErrorInfo(this->assignSignal(signal));
            }
            break;
        case CoreEventId::SignalDisconnected:
            checkErrorInfo(this->assignSignal(nullptr));
            break;
        case CoreEventId::ComponentUpdateEnd:
        case CoreEventId::AttributeChanged:
        case CoreEventId::TagsChanged:
        case CoreEventId::PropertyValueChanged:
        case CoreEventId::PropertyObjectUpdateEnd:
        case CoreEventId::PropertyAdded:
        case CoreEventId::PropertyRemoved:
        case CoreEventId::DataDescriptorChanged:
        case CoreEventId::ComponentAdded:
        case CoreEventId::ComponentRemoved:
        case CoreEventId::StatusChanged:
        case CoreEventId::TypeAdded:
        case CoreEventId::TypeRemoved:
        case CoreEventId::DeviceDomainChanged:
        default:
            break;
    }

    Super::handleRemoteCoreObjectInternal(sender, args);
}

inline void ConfigClientInputPortImpl::onRemoteUpdate(const SerializedObjectPtr& serialized)
{
    ConfigClientComponentBaseImpl::onRemoteUpdate(serialized);
    if (serialized.hasKey("signalId"))
    {
        serializedSignalId = serialized.readString("signalId");
    }
    else
        serializedSignalId.release();
}

inline ConnectionPtr ConfigClientInputPortImpl::createConnection(const SignalPtr& signal)
{
    const auto connection = createWithImplementation<IConnection, ConfigClientConnectionImpl>(this->template thisPtr<InputPortPtr>(), signal, this->context);
    return connection;
}

}
