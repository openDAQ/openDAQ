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
#include <opendaq/server_impl.h>

namespace daq::config_protocol
{

class ConfigClientServerImpl : public ConfigClientComponentBaseImpl<ServerImpl<IServer, IConfigClientObject>>
{
public:
    using Super = ConfigClientComponentBaseImpl<ServerImpl<IServer, IConfigClientObject>>;

    ConfigClientServerImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                           const std::string& remoteGlobalId,
                           const StringPtr& id,
                           const DevicePtr& parentDevice,
                           const ContextPtr& ctx);

    // IServer
    ErrCode INTERFACE_FUNC enableDiscovery() override;
    ErrCode INTERFACE_FUNC stop() override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void removed() override;
};

inline ConfigClientServerImpl::ConfigClientServerImpl(
    const ConfigProtocolClientCommPtr& configProtocolClientComm,
    const std::string& remoteGlobalId,
    const StringPtr& id,
    const DevicePtr& parentDevice,
    const ContextPtr& ctx)
    : Super(configProtocolClientComm, remoteGlobalId, id, nullptr, parentDevice, ctx)
{
}

inline ErrCode ConfigClientServerImpl::enableDiscovery()
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

inline ErrCode ConfigClientServerImpl::stop()
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

inline ErrCode ConfigClientServerImpl::Deserialize(ISerializedObject* serialized,
                                                   IBaseObject* context,
                                                   IFunction* factoryCallback,
                                                   IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    return daqTry(
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

                           const auto id = serialized.readString("id");
                           DevicePtr parentDevice;

                           if (const auto parentFolder = deserializeContext.getParent(); parentFolder.assigned())
                           {
                               if (parentFolder.getLocalId() == "Srv" &&
                                   parentFolder.getParent().assigned() &&
                                   parentFolder.getParent().supportsInterface<IDevice>())
                                   parentDevice = parentFolder.getParent().asPtr<IDevice>();
                               else
                                   throw GeneralErrorException("The server-component can be placed only under device's servers folder");
                           }

                           return createWithImplementation<IServer, ConfigClientServerImpl>(
                               configDeserializeContext->getClientComm(),
                               configDeserializeContext->getRemoteGlobalId(),
                               id,
                               parentDevice,
                               deserializeContext.getContext());
                       })
                       .detach();
        });
}

inline void ConfigClientServerImpl::removed()
{
    Super::Super::removed();
}

} // namespace daq::config_protocol
