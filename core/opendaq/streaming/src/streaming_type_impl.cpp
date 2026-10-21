#include <opendaq/streaming_type_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

StreamingTypeImpl::StreamingTypeImpl(const StringPtr& id,
                                            const StringPtr& name,
                                            const StringPtr& description,
                                            const StringPtr& prefix,
                                            const PropertyObjectPtr& defaultConfig,
                                            const DictPtr<IString, IAuthenticationMethod>& supportedAuthenticationMethods,
                                            const DeviceTypePtr& linkedDeviceType,
                                            const DictPtr<IString, IInteger>& transports)
    : Super(StreamingTypeStructType(), id, name, description, prefix, defaultConfig, supportedAuthenticationMethods, transports)
    , linkedDeviceType(linkedDeviceType)
{
}

StreamingTypeImpl::StreamingTypeImpl(const ComponentTypeBuilderPtr& builder)
    : StreamingTypeImpl(builder.getId(),
                        builder.getName(),
                        builder.getDescription(),
                        builder.getConnectionStringPrefix(),
                        builder.getDefaultConfig(),
                        builder.getSupportedAuthenticationMethods(),
                        builder.getLinkedDeviceType(),
                        builder.getAvailableTransports())
{
}

ErrCode StreamingTypeImpl::getConnectionStringPrefix(IString** prefix)
{
    OPENDAQ_PARAM_NOT_NULL(prefix);

    *prefix = this->prefix.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode StreamingTypeImpl::getLinkedDeviceType(IDeviceType** deviceType)
{
    OPENDAQ_PARAM_NOT_NULL(deviceType);

    *deviceType = this->linkedDeviceType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode StreamingTypeImpl::createDefaultConfig(IPropertyObject** defaultConfig)
{
    OPENDAQ_PARAM_NOT_NULL(defaultConfig);

    if (linkedDeviceType.assigned())
        return linkedDeviceType->createDefaultConfig(defaultConfig);

    return Super::createDefaultConfig(defaultConfig);
}

ErrCode StreamingTypeImpl::createAuthenticationConfig(IAuthenticationConfig** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    if (linkedDeviceType.assigned())
        return linkedDeviceType->createAuthenticationConfig(config);

    return Super::createAuthenticationConfig(config);
}

ErrCode StreamingTypeImpl::getSupportedAuthenticationMethods(IDict** authenticationMethods)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethods);

    if (linkedDeviceType.assigned())
        return linkedDeviceType.asPtr<IComponentTypePrivate>()->getSupportedAuthenticationMethods(authenticationMethods);

    return Super::getSupportedAuthenticationMethods(authenticationMethods);
}

ErrCode INTERFACE_FUNC StreamingTypeImpl::serialize(ISerializer* serializer)
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

            if (prefix.assigned())
            {
                serializerPtr.key("prefix");
                serializerPtr.writeString(prefix);
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

            if (supportedAuthenticationMethods.getCount() > 0)
            {
                serializerPtr.key("supportedAuthenticationMethods");
                supportedAuthenticationMethods.serialize(serializerPtr);
            }

            if (linkedDeviceType.assigned())
            {
                serializerPtr.key("linkedDeviceType");
                linkedDeviceType.serialize(serializerPtr);
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

ErrCode INTERFACE_FUNC StreamingTypeImpl::getSerializeId(ConstCharPtr* serializedId) const
{
    OPENDAQ_PARAM_NOT_NULL(serializedId);

    *serializedId = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr StreamingTypeImpl::SerializeId()
{
    return "StreamingType";
}

ErrCode StreamingTypeImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
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

        StringPtr prefix;
        if (serializedObj.hasKey("prefix"))
            prefix = serializedObj.readString("prefix");

        DictPtr<IString, IInteger> transports;
        if (serializedObj.hasKey("availableTransports"))
            transports = serializedObj.readObject("availableTransports", contextPtr, factoryCallbackPtr);

        PropertyObjectPtr defaultConfig;
        if (serializedObj.hasKey("defaultConfig"))
            defaultConfig = serializedObj.readObject("defaultConfig", contextPtr, factoryCallbackPtr);

        DictPtr<IString, IAuthenticationMethod> supportedAuthenticationMethods = Dict<IString, IAuthenticationMethod>();
        if (serializedObj.hasKey("supportedAuthenticationMethods"))
            supportedAuthenticationMethods = serializedObj.readObject("supportedAuthenticationMethods", contextPtr, factoryCallbackPtr).asPtr<IDict>();

        DeviceTypePtr linkedDeviceType;
        if (serializedObj.hasKey("linkedDeviceType"))
            linkedDeviceType = serializedObj.readObject("linkedDeviceType", contextPtr, factoryCallbackPtr).asPtr<IDeviceType>();

        auto streamingType = createWithImplementation<IStreamingType, StreamingTypeImpl>(
            id, name, description, prefix, defaultConfig, supportedAuthenticationMethods, linkedDeviceType, transports);

        ModuleInfoPtr moduleInfo;
        if (serializedObj.hasKey("moduleInfo"))
        {
            moduleInfo = serializedObj.readObject("moduleInfo", contextPtr, factoryCallbackPtr);
            streamingType.asPtr<IComponentTypePrivate>()->setModuleInfo(moduleInfo);
        }

        *obj = streamingType.detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(StreamingTypeImpl)

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    StreamingType,
    IString*,
    id,
    IString*,
    name,
    IString*,
    description,
    IString*,
    prefix,
    IPropertyObject*,
    defaultConfig,
    IDict*,
    supportedAuthenticationMethods,
    IDeviceType*,
    linkedDeviceType
    )

END_NAMESPACE_OPENDAQ
