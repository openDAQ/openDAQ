#include <opendaq/device_type_impl.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/authentication_method_factory.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr deviceTypeStructType = DeviceTypeStructType();
}

DeviceTypeImpl::DeviceTypeImpl(const StringPtr& id,
                               const StringPtr& name,
                               const StringPtr& description,
                               const PropertyObjectPtr& defaultConfig,
                               const StringPtr& prefix,
                               const DictPtr<IString, IAuthenticationMethod>& supportedAuthenticationMethods,
                               DeviceKind deviceKind)
    : Super(detail::deviceTypeStructType, id, name, description, prefix, defaultConfig, supportedAuthenticationMethods)
    , deviceKind(deviceKind)
{
}

DeviceTypeImpl::DeviceTypeImpl(const ComponentTypeBuilderPtr& builder)
    : DeviceTypeImpl(builder.getId(),
                     builder.getName(),
                     builder.getDescription(),
                     builder.getDefaultConfig(),
                     builder.getConnectionStringPrefix(),
                     builder.getSupportedAuthenticationMethods(),
                     builder.getDeviceKind())
{
}

ErrCode DeviceTypeImpl::getConnectionStringPrefix(IString** prefix)
{
    OPENDAQ_PARAM_NOT_NULL(prefix);

    *prefix = this->prefix.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceTypeImpl::getDeviceKind(DeviceKind* kind)
{
    OPENDAQ_PARAM_NOT_NULL(kind);

    *kind = this->deviceKind;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC DeviceTypeImpl::serialize(ISerializer* serializer)
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

            if (moduleInfo.assigned())
            {
                serializerPtr.key("moduleInfo");
                moduleInfo.serialize(serializerPtr);
            }

            serializerPtr.key("deviceKind");
            serializerPtr.writeInt(static_cast<Int>(deviceKind));
        }

        serializerPtr.endObject();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode INTERFACE_FUNC DeviceTypeImpl::getSerializeId(ConstCharPtr* serializedId) const
{
    OPENDAQ_PARAM_NOT_NULL(serializedId);

    *serializedId = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr DeviceTypeImpl::SerializeId()
{
    return "DeviceType";
}

ErrCode DeviceTypeImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
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

        PropertyObjectPtr defaultConfig;
        if (serializedObj.hasKey("defaultConfig"))
            defaultConfig = serializedObj.readObject("defaultConfig", contextPtr, factoryCallbackPtr);

        DictPtr<IString, IAuthenticationMethod> supportedAuthenticationMethods = Dict<IString, IAuthenticationMethod>();
        if (serializedObj.hasKey("supportedAuthenticationMethods"))
            supportedAuthenticationMethods = serializedObj.readObject("supportedAuthenticationMethods", contextPtr, factoryCallbackPtr).asPtr<IDict>();

        DeviceKind deviceKind = DeviceKind::External;
        if (serializedObj.hasKey("deviceKind"))
            deviceKind = static_cast<DeviceKind>(serializedObj.readInt("deviceKind"));

        auto deviceType = createWithImplementation<IDeviceType, DeviceTypeImpl>(
            id, name, description, defaultConfig, prefix, supportedAuthenticationMethods, deviceKind);

        ModuleInfoPtr moduleInfo;
        if (serializedObj.hasKey("moduleInfo"))
        {
            moduleInfo = serializedObj.readObject("moduleInfo", contextPtr, factoryCallbackPtr);
            deviceType.asPtr<IComponentTypePrivate>()->setModuleInfo(moduleInfo);
        }

        *obj = deviceType.detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(DeviceTypeImpl)

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    DeviceType,
    IString*,
    id,
    IString*,
    name,
    IString*,
    description,
    IPropertyObject*,
    defaultConfig,
    IString*,
    prefix,
    IDict*,
    supportedAuthenticationMethods
    )

END_NAMESPACE_OPENDAQ
