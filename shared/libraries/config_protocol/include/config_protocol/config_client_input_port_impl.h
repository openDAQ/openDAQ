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
#include <config_protocol/config_client_component_impl.h>
#include <opendaq/input_port_impl.h>
#include <config_protocol/config_client_connection_impl.h>
#include <config_protocol/config_client_input_port.h>
#include <opendaq/errors.h>

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

    ErrCode INTERFACE_FUNC acceptsSignal(ISignal* signal, Bool* accepts) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args) override;
    void onRemoteUpdate(const SerializedObjectPtr& serialized) override;

    ConnectionPtr createConnection(const SignalPtr& signal) override;
    SignalPtr getConnectedSignal();

    bool isSignalFromTheSameComponentTree(const SignalPtr& signal);

    void removed() override;
};

inline ConfigClientInputPortImpl::ConfigClientInputPortImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                            const std::string& remoteGlobalId,
                                                            const ContextPtr& ctx,
                                                            const ComponentPtr& parent,
                                                            const StringPtr& localId,
                                                            const StringPtr&)
    : Super(configProtocolClientComm, remoteGlobalId, ctx, parent, localId, false)
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
            if (!isSignalFromTheSameComponentTree(signalPtr))
                return OPENDAQ_ERR_SIGNAL_NOT_ACCEPTED;
            {
                std::scoped_lock lock(this->sync);

                const auto connectedSignal = getConnectedSignal();
                if (connectedSignal == signalPtr)
                    return OPENDAQ_IGNORED;
                if (connectedSignal.assigned() && !clientComm->isComponentNested(connectedSignal.getGlobalId()))
                    clientComm->disconnectExternalSignalFromServerInputPort(connectedSignal, remoteGlobalId);
            }

            const auto configObject = signalPtr.asPtrOrNull<IConfigClientObject>(true);
            if (configObject.assigned() && clientComm->isComponentNested(signalPtr.getGlobalId()))
            {
                StringPtr signalRemoteGlobalId;
                checkErrorInfo(configObject->getRemoteGlobalId(&signalRemoteGlobalId));
                clientComm->connectSignal(remoteGlobalId, signalRemoteGlobalId);
            }
            else
            {
                if (clientComm->getProtocolVersion() >= 2)
                    clientComm->connectExternalSignalToServerInputPort(signalPtr, remoteGlobalId);
                else
                    return makeErrorInfo(
                        OPENDAQ_ERR_SIGNAL_NOT_ACCEPTED,
                        "Client-to-device streaming operations are not supported by the protocol version currently in use"
                    );
            }

            return Super::connect(signal);
        });
}

inline ErrCode ConfigClientInputPortImpl::disconnect()
{
    return daqTry(
        [this]
        {
            assert(this->deserializationComplete);

            clientComm->disconnectSignal(remoteGlobalId);
            return Super::disconnect();
        });
}

inline ErrCode ConfigClientInputPortImpl::assignSignal(ISignal* signal)
{
    const auto connectedSignal = getConnectedSignal();
    const auto signalPtr = SignalPtr::Borrow(signal);

    if (connectedSignal != signalPtr && connectedSignal.assigned() && !clientComm->isComponentNested(connectedSignal.getGlobalId()))
        clientComm->disconnectExternalSignalFromServerInputPort(connectedSignal, remoteGlobalId);

    if (signal == nullptr)
        return Super::disconnect();
    return Super::connect(signal);
}

inline ErrCode INTERFACE_FUNC ConfigClientInputPortImpl::acceptsSignal(ISignal* signal, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(signal);
    OPENDAQ_PARAM_NOT_NULL(accepts);

    return daqTry(
        [this, &signal, &accepts]
        {
            const auto signalPtr = SignalPtr::Borrow(signal);
            if (!isSignalFromTheSameComponentTree(signalPtr))
                return makeErrorInfo(OPENDAQ_ERR_NATIVE_CLIENT_CALL_NOT_AVAILABLE, "Signal is not from the same component tree");

            const auto configObject = signalPtr.asPtrOrNull<IConfigClientObject>(true);
            if (configObject.assigned() && clientComm->isComponentNested(signalPtr.getGlobalId()))
            {
                StringPtr signalRemoteGlobalId;
                checkErrorInfo(configObject->getRemoteGlobalId(&signalRemoteGlobalId));
                BooleanPtr acceptsPtr = clientComm->acceptsSignal(remoteGlobalId, signalRemoteGlobalId);
                *accepts = acceptsPtr.getValue(False);
                return OPENDAQ_SUCCESS;
            }
            *accepts = False;
            return OPENDAQ_SUCCESS;
        });
}


inline SignalPtr ConfigClientInputPortImpl::getConnectedSignal()
{
    const auto conn = this->getConnectionNoLock();
    if (conn.assigned())
        return conn.getSignal();

    return nullptr;
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

inline bool ConfigClientInputPortImpl::isSignalFromTheSameComponentTree(const SignalPtr& signal)
{
    auto portGlobalId = this->globalId.toStdString();
    auto signalGlobalId = signal.getGlobalId().toStdString();

    size_t portTreeRootIdEnd = portGlobalId.find('/', 1);
    size_t signalTreeRootIdEnd = signalGlobalId.find('/', 1);

    if (portTreeRootIdEnd == std::string::npos || signalTreeRootIdEnd == std::string::npos || portTreeRootIdEnd != signalTreeRootIdEnd)
        return false;

    std::string portTreeRootId = portGlobalId.substr(1, portTreeRootIdEnd - 1);
    std::string signalTreeRootId = signalGlobalId.substr(1, signalTreeRootIdEnd - 1);

    return portTreeRootId == signalTreeRootId;
}

inline void ConfigClientInputPortImpl::removed()
{
    const auto connectedSignal = getConnectedSignal();

    if (connectedSignal.assigned() && !clientComm->isComponentNested(connectedSignal.getGlobalId()))
        clientComm->disconnectExternalSignalFromServerInputPort(connectedSignal, remoteGlobalId);
    Super::removed();
}

}
