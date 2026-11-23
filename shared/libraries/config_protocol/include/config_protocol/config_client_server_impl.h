/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/server_impl.h>

namespace daq::config_protocol
{

class ConfigClientServerImpl : public ConfigClientComponentBaseImpl<ServerImpl<IServer, IConfigClientObject>>
{
public:
    using Super = ConfigClientComponentBaseImpl<ServerImpl<IServer, IConfigClientObject>>;

    ConfigClientServerImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                           const std::string& remoteGlobalId,
                           const ServerTypePtr& serverType,
                           const DevicePtr& parentDevice,
                           const ContextPtr& ctx,
                           const ComponentPtr& parentFolder);

    // IServer
    ErrCode INTERFACE_FUNC enableDiscovery() override;
    ErrCode INTERFACE_FUNC disableDiscovery() override;
    ErrCode INTERFACE_FUNC stop() override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void removed() override;

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config) override;
    void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock) override;

private:
    // Oldest native configuration protocol version that handles function block commands on servers.
    static constexpr uint16_t FunctionBlockCommandsMinVersion = 26;
};

inline ConfigClientServerImpl::ConfigClientServerImpl(
    const ConfigProtocolClientCommPtr& configProtocolClientComm,
    const std::string& remoteGlobalId,
    const ServerTypePtr& serverType,
    const DevicePtr& parentDevice,
    const ContextPtr& ctx,
    const ComponentPtr& parentFolder)
    : Super(configProtocolClientComm, remoteGlobalId, serverType, nullptr, parentDevice, ctx, parentFolder)
{
}

inline ErrCode ConfigClientServerImpl::enableDiscovery()
{
    const ErrCode errCode = daqTry([this] { this->clientComm->enableDiscovery(this->remoteGlobalId); });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

inline ErrCode ConfigClientServerImpl::disableDiscovery()
{
    const ErrCode errCode = daqTry([this] { this->clientComm->disableDiscovery(this->remoteGlobalId); });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

inline ErrCode ConfigClientServerImpl::stop()
{
    return OPENDAQ_IGNORED;
}

inline ErrCode ConfigClientServerImpl::Deserialize(ISerializedObject* serialized,
                                                   IBaseObject* context,
                                                   IFunction* factoryCallback,
                                                   IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    const ErrCode errCode = daqTry(
    [&obj, &serialized, &context, &factoryCallback]()
    {
        *obj = Super::DeserializeComponent(
                    serialized,
                    context,
                    factoryCallback,
                    [](const SerializedObjectPtr& serialized,
                        const ComponentDeserializeContextPtr& deserializeContext,
                        const StringPtr& /*className*/)
                    {
                        const auto configDeserializeContext = deserializeContext.asPtr<IConfigProtocolDeserializeContext>();

                        const auto serverType = DeserializeServerType(serialized);
                        DevicePtr parentDevice;

                        const auto parentFolder = deserializeContext.getParent();
                        if (parentFolder.assigned())
                        {
                            if (parentFolder.getLocalId() == "Srv" &&
                                parentFolder.getParent().supportsInterface<IDevice>())
                                parentDevice = parentFolder.getParent().asPtr<IDevice>();
                            else
                                DAQ_THROW_EXCEPTION(GeneralErrorException, "The server-component can be placed only under device's servers folder");
                        }

                        return createWithImplementation<IServer, ConfigClientServerImpl>(
                            configDeserializeContext->getClientComm(),
                            configDeserializeContext->getRemoteGlobalId(),
                            serverType,
                            parentDevice,
                            deserializeContext.getContext(),
                            parentFolder);
                    })
                    .detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

inline void ConfigClientServerImpl::removed()
{
    Super::Super::removed();
}

inline DictPtr<IString, IFunctionBlockType> ConfigClientServerImpl::onGetAvailableFunctionBlockTypes()
{
    // A device older than the server function block commands has no types to offer.
    if (this->clientComm->getProtocolVersion() < FunctionBlockCommandsMinVersion)
        return Dict<IString, IFunctionBlockType>();

    return this->clientComm->getAvailableFunctionBlockTypes(this->remoteGlobalId, FunctionBlockCommandsMinVersion);
}

inline FunctionBlockPtr ConfigClientServerImpl::onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config)
{
    return this->addFunctionBlockRemote(typeId, config, FunctionBlockCommandsMinVersion);
}

inline void ConfigClientServerImpl::onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock)
{
    this->removeFunctionBlockRemote(functionBlock, FunctionBlockCommandsMinVersion);
}

} // namespace daq::config_protocol
