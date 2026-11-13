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
#include <opendaq/context_ptr.h>
#include <opendaq/device_ptr.h>
#include <opendaq/module_manager_ptr.h>
#include <opendaq/server.h>
#include <opendaq/server_type_factory.h>
#include <opendaq/component_type_private.h>
#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/validation.h>
#include <coreobjects/property_factory.h>
#include <opendaq/discovery_server_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/signal_container_impl.h>
#include <opendaq/streaming_ptr.h>

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

    // The server's ID and local ID are the type's ID.
    explicit ServerImpl(const ServerTypePtr& serverType,
                        const PropertyObjectPtr& serverConfig,
                        const DevicePtr& rootDevice,
                        const ContextPtr& context,
                        const ComponentPtr& parent = nullptr)
        : Super(context, parent.assigned() ? parent : (rootDevice.assigned() ? rootDevice.getItem("Srv") : nullptr), serverType.getId())
        , id(serverType.getId())
        , serverType(serverType)
        , config(serverConfig)
        , rootDeviceRef(rootDevice)
        , context(context)
    {
    }

    // getServerType returns a type with this ID and ServerKind::Unspecified.
    explicit ServerImpl(const StringPtr& id,
                        const PropertyObjectPtr& serverConfig,
                        const DevicePtr& rootDevice,
                        const ContextPtr& context,
                        const ComponentPtr& parent = nullptr)
        : ServerImpl(CreateServerType(id, ServerKind::Unspecified), serverConfig, rootDevice, context, parent)
    {
    }

    // Keeps Server("Id", ...) unambiguous: every smart pointer, ServerTypePtr included, converts from a string literal.
    explicit ServerImpl(ConstCharPtr id,
                        const PropertyObjectPtr& serverConfig,
                        const DevicePtr& rootDevice,
                        const ContextPtr& context,
                        const ComponentPtr& parent = nullptr)
        : ServerImpl(StringPtr(id), serverConfig, rootDevice, context, parent)
    {
    }

    ErrCode INTERFACE_FUNC getId(IString** serverId) override
    {
        if (serverId == nullptr)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER);

        *serverId = id.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC enableDiscovery() override
    {
        if (const DevicePtr rootDevice = rootDeviceRef.assigned() ? rootDeviceRef.getRef() : nullptr; rootDevice.assigned() && context.assigned())
        {
            DeviceInfoPtr rootDeviceInfo = rootDevice.getInfo();
            for (const auto& [_, discoveryServer] : context.getDiscoveryServers())
            {
                for (const auto& discoveryConfig : getDiscoveryConfigs())
                    discoveryServer.template asPtr<IDiscoveryServer>().registerService(id, discoveryConfig, rootDeviceInfo);
            }
        }
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC disableDiscovery() override
    {
        disableDiscoveryInternal();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC stop() override
    {
        disableDiscoveryInternal();
        const ErrCode errCode = wrapHandler(this, &Self::onStopServer);
        OPENDAQ_RETURN_IF_FAILED(errCode, "Failed to stop server");
        return errCode;
    }

    ErrCode INTERFACE_FUNC getSignals(IList** signals, ISearchFilter* searchFilter = nullptr) override
    {
        OPENDAQ_PARAM_NOT_NULL(signals);

        if (this->isComponentRemoved)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_COMPONENT_REMOVED);

        if (!searchFilter)
            return this->signals->getItems(signals);

        const auto searchFilterPtr = SearchFilterPtr::Borrow(searchFilter);
        return this->signals->getItems(signals, searchFilter);
    }

    ErrCode INTERFACE_FUNC getStreaming(IStreaming** streaming) override
    {
        OPENDAQ_PARAM_NOT_NULL(streaming);

        StreamingPtr streamingPtr;
        const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetStreaming, streamingPtr);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        *streaming = streamingPtr.detach();

        return errCode;
    }

    ErrCode INTERFACE_FUNC getServerType(IServerType** type) override
    {
        OPENDAQ_PARAM_NOT_NULL(type);

        *type = serverType.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter = nullptr) override
    {
        OPENDAQ_PARAM_NOT_NULL(functionBlocks);

        if (this->isComponentRemoved)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_COMPONENT_REMOVED);

        return this->getFunctionBlocksInternal(functionBlocks, searchFilter);
    }

    ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) override
    {
        return this->getAvailableFunctionBlockTypesInternal(functionBlockTypes);
    }

    ErrCode INTERFACE_FUNC addFunctionBlock(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config = nullptr) override
    {
        return this->addFunctionBlockInternal(functionBlock, typeId, config);
    }

    ErrCode INTERFACE_FUNC removeFunctionBlock(IFunctionBlock* functionBlock) override
    {
        return this->removeFunctionBlockInternal(functionBlock);
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

        const ErrCode errCode = daqTry([&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::DeserializeComponent(
                serialized,
                context,
                factoryCallback,
                [](const SerializedObjectPtr& serialized,
                    const ComponentDeserializeContextPtr& deserializeContext,
                    const StringPtr& /*className*/) -> BaseObjectPtr
                {
                    const auto serverType = DeserializeServerType(serialized);
                    DevicePtr parentDevice;

                    const auto parentFolder = deserializeContext.getParent();
                    if (parentFolder.assigned())
                    {
                        if (parentFolder.getLocalId() == "Srv" &&
                            parentFolder.getParent().assigned() &&
                            parentFolder.getParent().supportsInterface<IDevice>())
                            parentDevice = parentFolder.getParent().asPtr<IDevice>();
                        else
                            DAQ_THROW_EXCEPTION(GeneralErrorException, "The server-component can be placed only under device's servers folder");
                    }

                    return createWithImplementation<IServer, Server>(
                        serverType,
                        nullptr,
                        parentDevice,
                        deserializeContext.getContext(),
                        parentFolder);
                }).detach();
        });
        OPENDAQ_RETURN_IF_FAILED(errCode);
        return errCode;
    }

    // Builds a placeholder type from an ID and kind, for servers created or received without a full type.
    static ServerTypePtr CreateServerType(const StringPtr& id, ServerKind kind)
    {
        return ServerTypeBuilder().setId(id).setName(id).setDescription("").setServerKind(kind).build().asPtr<IServerType>();
    }

    // The type the server carries, or a placeholder with only the ID from peers that write just the ID.
    static ServerTypePtr DeserializeServerType(const SerializedObjectPtr& serialized)
    {
        if (serialized.hasKey("componentType"))
            return serialized.readObject("componentType");
        return CreateServerType(serialized.readString("id"), ServerKind::Unspecified);
    }

protected:

    virtual PropertyObjectPtr getDiscoveryConfig()
    {
        return PropertyObject();
    }

    virtual ListPtr<IPropertyObject> getDiscoveryConfigs()
    {
        return List<IPropertyObject>(getDiscoveryConfig());
    }

    virtual void onStopServer()
    {
    }

    virtual StreamingPtr onGetStreaming()
    {
        return nullptr;
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

        // The type without its default config; clients take the default config from the type catalog.
        if (const auto typePrivate = serverType.template asPtrOrNull<IComponentTypePrivate>(true); !forUpdate && typePrivate.assigned())
        {
            serializer.key("componentType");
            checkErrorInfo(typePrivate->serializeWithoutDefaultConfig(serializer));
        }

        Super::serializeCustomObjectValues(serializer, forUpdate);
    }

    void disableDiscoveryInternal()
    {
        if (context.assigned())
        {
            for (const auto& [_, discoveryServer] : context.getDiscoveryServers())
                discoveryServer.template asPtr<IDiscoveryServer>(true).unregisterService(id);
        }
    }

    StringPtr id;
    ServerTypePtr serverType;
    PropertyObjectPtr config;
    WeakRefPtr<IDevice> rootDeviceRef;
    ContextPtr context;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(Server)

END_NAMESPACE_OPENDAQ
