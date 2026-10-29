#include <opendaq/server_type_impl.h>
#include <opendaq/server_type_factory.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr serverTypeStructType = ServerTypeStructType();
}

ServerTypeImpl::ServerTypeImpl(const StringPtr& id,
                               const StringPtr& name,
                               const StringPtr& description,
                               const PropertyObjectPtr& defaultConfig,
                               const DictPtr<IString, IInteger>& transports)
    : Super(detail::serverTypeStructType, id, name, description, defaultConfig, Dict<IString, IAuthenticationMethod>(), transports)
{
}

ServerTypeImpl::ServerTypeImpl(const ComponentTypeBuilderPtr& builder)
    : ServerTypeImpl(builder.getId(),
                     builder.getName(),
                     builder.getDescription(),
                     builder.getDefaultConfig(),
                     builder.getAvailableTransports())
{
}

ErrCode ServerTypeImpl::createAuthenticationConfig(IAuthenticationConfig** /*config*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED, "Server types do not support authentication");
}

ErrCode ServerTypeImpl::getSupportedAuthenticationMethods(IDict** /*authenticationMethods*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED, "Server types do not support authentication");
}

ErrCode INTERFACE_FUNC ServerTypeImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    const auto serializerPtr = SerializerPtr::Borrow(serializer);

    const ErrCode errCode = daqTry([this, &serializerPtr]
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

            if (transports.getCount() > 0)
            {
                serializerPtr.key("availableTransports");
                transports.serialize(serializerPtr);
            }

            if (defaultConfig.assigned())
            {
                serializerPtr.key("defaultConfig");
                defaultConfig.serialize(serializerPtr);
            }

            if (moduleInfo.assigned())
            {
                serializerPtr.key("moduleInfo");
                moduleInfo.serialize(serializerPtr);
            }
        }

        serializerPtr.endObject();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
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

        DictPtr<IString, IInteger> transports;
        if (serializedObj.hasKey("availableTransports"))
            transports = serializedObj.readObject("availableTransports", contextPtr, factoryCallbackPtr);

        PropertyObjectPtr defaultConfig;
        if (serializedObj.hasKey("defaultConfig"))
            defaultConfig = serializedObj.readObject("defaultConfig", contextPtr, factoryCallbackPtr);

        auto serverType = createWithImplementation<IServerType, ServerTypeImpl>(id, name, description, defaultConfig, transports);

        ModuleInfoPtr moduleInfo;
        if (serializedObj.hasKey("moduleInfo"))
        {
            moduleInfo = serializedObj.readObject("moduleInfo", contextPtr, factoryCallbackPtr);
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
