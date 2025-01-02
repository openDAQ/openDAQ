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
#include <opendaq/context_ptr.h>
#include <opendaq/device_ptr.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/server.h>
#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/validation.h>
#include <coreobjects/property_factory.h>
#include <opendaq/discovery_server_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/signal_container_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TInterface = IServer, typename... Interfaces>
class ServerImpl;

using Server = ServerImpl<>;

template <typename TInterface, typename... Interfaces>
class ServerImpl : public SignalContainerImpl<TInterface, Interfaces...>
{
public:
    using Self = ServerImpl<TInterface, Interfaces...>;
    using Super = SignalContainerImpl<TInterface, Interfaces...>;

    explicit ServerImpl(const StringPtr& id,
                        const PropertyObjectPtr& serverConfig,
                        const DevicePtr& rootDevice,
                        const ContextPtr& context,
                        const ComponentPtr& parent = nullptr)
        : Super(context, parent.assigned() ? parent : (rootDevice.assigned() ? rootDevice.getItem("Srv") : nullptr), id)
        , id(id)
        , config(serverConfig)
        , rootDeviceRef(rootDevice)
        , context(context)
    {
    }

    ErrCode INTERFACE_FUNC getId(IString** serverId) override
    {
        if (serverId == nullptr)
            return OPENDAQ_ERR_INVALIDPARAMETER;
        *serverId = id.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC enableDiscovery() override
    {
        if (const DevicePtr rootDevice = rootDeviceRef.assigned() ? rootDeviceRef.getRef() : nullptr; rootDevice.assigned() && context.assigned())
        {
            DeviceInfoPtr rootDeviceInfo = rootDevice.getInfo();
            for (const auto& [_, service] : context.getDiscoveryServers())
            {
                service.template asPtr<IDiscoveryServer>().registerService(id, getDiscoveryConfig(), rootDeviceInfo);
            }
        }
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC stop() override
    {
        if (context != nullptr)
        {
            for (const auto& [_, service] : context.getDiscoveryServers())
            {
                service.template asPtr<IDiscoveryServer>().unregisterService(id);
            }
        }
        return wrapHandler(this, &Self::onStopServer);
    }

    ErrCode INTERFACE_FUNC getSignals(IList** signals, ISearchFilter* searchFilter = nullptr) override
    {
        OPENDAQ_PARAM_NOT_NULL(signals);

        if (this->isComponentRemoved)
            return OPENDAQ_ERR_COMPONENT_REMOVED;

        if (!searchFilter)
            return this->signals->getItems(signals);

        const auto searchFilterPtr = SearchFilterPtr::Borrow(searchFilter);
        return this->signals->getItems(signals, searchFilter);
    }

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override
    {
        OPENDAQ_PARAM_NOT_NULL(id);

        *id = SerializeId();

        return OPENDAQ_SUCCESS;
    }

    static ConstCharPtr SerializeId()
    {
        return "Server";
    }

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
    {
        OPENDAQ_PARAM_NOT_NULL(obj);

        return daqTry([&obj, &serialized, &context, &factoryCallback]()
            {
                *obj = Super::DeserializeComponent(
                    serialized,
                    context,
                    factoryCallback,
                    [](const SerializedObjectPtr& serialized,
                       const ComponentDeserializeContextPtr& deserializeContext,
                       const StringPtr& /*className*/) -> BaseObjectPtr
                    {
                        const auto id = serialized.readString("id");
                        DevicePtr parentDevice;

                        const auto parentFolder = deserializeContext.getParent();
                        if (parentFolder.assigned())
                        {
                            if (parentFolder.getLocalId() == "Srv" &&
                                parentFolder.getParent().assigned() &&
                                parentFolder.getParent().supportsInterface<IDevice>())
                                parentDevice = parentFolder.getParent().asPtr<IDevice>();
                            else
                                throw GeneralErrorException("The server-component can be placed only under device's servers folder");
                        }

                        return createWithImplementation<IServer, Server>(
                            id,
                            nullptr,
                            parentDevice,
                            deserializeContext.getContext(),
                            parentFolder);
                    }).detach();
            });
    }

protected:

    virtual PropertyObjectPtr getDiscoveryConfig()
    {
        return PropertyObject();
    }

    virtual void onStopServer()
    {
    }

    void removed() override
    {
        checkErrorInfo(stop());
        Super::removed();
    }

    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override
    {
        serializer.key("id");
        serializer.writeString(id);

        Super::serializeCustomObjectValues(serializer, forUpdate);
    }

    StringPtr id;
    PropertyObjectPtr config;
    WeakRefPtr<IDevice> rootDeviceRef;
    ContextPtr context;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(Server)

END_NAMESPACE_OPENDAQ
