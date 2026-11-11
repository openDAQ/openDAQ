#include <opendaq/server_type_impl.h>
#include <opendaq/server_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr serverTypeStructType = ServerTypeStructType();

    // Kinds from newer peers that this version does not know read as Unspecified.
    static ServerKind toServerKind(Int value)
    {
        if (value > static_cast<Int>(ServerKind::Unspecified) && value <= static_cast<Int>(ServerKind::Streaming))
            return static_cast<ServerKind>(value);
        return ServerKind::Unspecified;
    }
}

ServerTypeImpl::ServerTypeImpl(const StringPtr& id,
                               const StringPtr& name,
                               const StringPtr& description,
                               const PropertyObjectPtr& defaultConfig,
                               ServerKind serverKind)
    : Super(detail::serverTypeStructType, id, name, description, defaultConfig)
    , serverKind(serverKind)
{
}

ServerTypeImpl::ServerTypeImpl(const ComponentTypeBuilderPtr& builder)
    : ServerTypeImpl(builder.getId(), builder.getName(), builder.getDescription(), builder.getDefaultConfig(), builder.getServerKind())
{
}

ErrCode ServerTypeImpl::getServerKind(ServerKind* kind)
{
    OPENDAQ_PARAM_NOT_NULL(kind);

    *kind = this->serverKind;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ServerTypeImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    const auto serializerPtr = SerializerPtr::Borrow(serializer);
    const ErrCode errCode = daqTry([this, &serializerPtr] { serializeType(serializerPtr, true); });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

void ServerTypeImpl::serializeType(const SerializerPtr& serializerPtr, bool withDefaultConfig)
{
    serializerPtr.startTaggedObject(borrowPtr<SerializablePtr>());
    {
        serializerPtr.key("id");
        serializerPtr.writeString(id);

        if (name.assigned())
        {
            serializerPtr.key("name");
            serializerPtr.writeString(name);
        }

        if (description.assigned())
        {
            serializerPtr.key("description");
            serializerPtr.writeString(description);
        }

        if (withDefaultConfig && defaultConfig.assigned())
        {
            serializerPtr.key("defaultConfig");
            defaultConfig.serialize(serializerPtr);
        }

        if (moduleInfo.assigned())
        {
            serializerPtr.key("moduleInfo");
            moduleInfo.serialize(serializerPtr);
        }

        serializerPtr.key("serverKind");
        serializerPtr.writeInt(static_cast<Int>(serverKind));
    }

    serializerPtr.endObject();
}

ErrCode INTERFACE_FUNC ServerTypeImpl::getSerializeId(ConstCharPtr* serializedId) const
{
    OPENDAQ_PARAM_NOT_NULL(serializedId);

    *serializedId = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr ServerTypeImpl::SerializeId()
{
    return "ServerType";
}

ErrCode ServerTypeImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    const ErrCode errCode = daqTry([&serializedObj, &contextPtr, &factoryCallbackPtr, &obj]
    {
        const auto id = serializedObj.readString("id");

        StringPtr name;
        if (serializedObj.hasKey("name"))
            name = serializedObj.readString("name");

        StringPtr description;
        if (serializedObj.hasKey("description"))
            description = serializedObj.readString("description");

        PropertyObjectPtr defaultConfig;
        if (serializedObj.hasKey("defaultConfig"))
            defaultConfig = serializedObj.readObject("defaultConfig", contextPtr, factoryCallbackPtr);

        const auto serverKind =
            serializedObj.hasKey("serverKind") ? detail::toServerKind(serializedObj.readInt("serverKind")) : ServerKind::Unspecified;

        auto serverType = createWithImplementation<IServerType, ServerTypeImpl>(id, name, description, defaultConfig, serverKind);

        if (serializedObj.hasKey("moduleInfo"))
        {
            const ModuleInfoPtr moduleInfo = serializedObj.readObject("moduleInfo", contextPtr, factoryCallbackPtr);
            serverType.asPtr<IComponentTypePrivate>()->setModuleInfo(moduleInfo);
        }

        *obj = serverType.detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(ServerTypeImpl)

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    ServerType,
    IString*,
    id,
    IString*,
    name,
    IString*,
    description,
    IPropertyObject*,
    defaultConfig
    )

END_NAMESPACE_OPENDAQ
