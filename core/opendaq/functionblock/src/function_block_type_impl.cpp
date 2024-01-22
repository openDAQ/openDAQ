#include <opendaq/function_block_type_impl.h>
#include <opendaq/function_block_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr functionBlockTypeStructType = FunctionBlockTypeStructType();
}

FunctionBlockTypeImpl::FunctionBlockTypeImpl(const StringPtr& id,
                                             const StringPtr& name,
                                             const StringPtr& description,
                                             const FunctionPtr& createDefaultConfigCallback)
    : Super(detail::functionBlockTypeStructType, id, name, description, createDefaultConfigCallback)
{
}

ErrCode FunctionBlockTypeImpl::serialize(ISerializer* serializer)
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

                if (createDefaultConfigCallback.assigned())
                {
                    const auto defaultConfig = createDefaultConfigCallback.call();
                    if (defaultConfig.assigned())
                    {
                        serializerPtr.key("defaultConfig");
                        defaultConfig.serialize(serializerPtr);
                    }
                }
            }

            serializerPtr.endObject();
        });
}


ErrCode FunctionBlockTypeImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr FunctionBlockTypeImpl::SerializeId()
{
    return "FunctionBlockType";
}

ErrCode FunctionBlockTypeImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
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

            FunctionPtr createDefaultConfig;
            if (serializedObj.hasKey("defaultConfig"))
            {
                PropertyObjectPtr defaultConfig = serializedObj.readObject("defaultConfig", contextPtr, factoryCallbackPtr);
                createDefaultConfig = Function([defaultConfig]
                {
                    return defaultConfig;
                });
            }

            *obj = createWithImplementation<IFunctionBlockType, FunctionBlockTypeImpl>(id, name, description, createDefaultConfig).detach();
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    FunctionBlockType,
    IString*,
    id,
    IString*,
    name,
    IString*,
    description,
    IFunction*,
    createDefaultConfigCallback
    )

END_NAMESPACE_OPENDAQ
