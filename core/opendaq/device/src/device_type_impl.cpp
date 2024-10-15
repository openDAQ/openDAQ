#include <opendaq/device_type_impl.h>
#include <opendaq/device_type_factory.h>
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
                               const StringPtr& prefix)
    : Super(detail::deviceTypeStructType, id, name, description, prefix, defaultConfig)
{
}

DeviceTypeImpl::DeviceTypeImpl(const ComponentTypeBuilderPtr& builder)
    : DeviceTypeImpl(builder.getId(), builder.getName(), builder.getDescription(), builder.getDefaultConfig(), builder.getConnectionStringPrefix())
{
}

ErrCode DeviceTypeImpl::getConnectionStringPrefix(IString** prefix)
{
    OPENDAQ_PARAM_NOT_NULL(prefix);

    *prefix = this->prefix.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC DeviceTypeImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    const auto serializerPtr = SerializerPtr::Borrow(serializer);

    return daqTry(
        [this, &serializerPtr]
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

                if (description.assigned())
                {
                    serializerPtr.key("prefix");
                    serializerPtr.writeString(prefix);
                }

                if (defaultConfig.assigned())
                {
                    serializerPtr.key("defaultConfig");
                    defaultConfig.serialize(serializerPtr);
                }
            }

            serializerPtr.endObject();
        });
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

    return daqTry(
        [&serializedObj, &contextPtr, &factoryCallbackPtr, &obj]
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

            *obj = createWithImplementation<IDeviceType, DeviceTypeImpl>(id, name, description, defaultConfig, prefix).detach();
        });
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
    prefix
    )

END_NAMESPACE_OPENDAQ
