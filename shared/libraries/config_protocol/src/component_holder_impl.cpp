#include <config_protocol/component_holder_impl.h>
#include <coretypes/validation.h>
#include <opendaq/component_deserialize_context_ptr.h>

namespace daq::config_protocol
{
ComponentHolderImpl::ComponentHolderImpl(const StringPtr& localId, const ComponentPtr& component)
    : localId(localId)
    , component(component)
{
    if (!localId.assigned())
        throw ArgumentNullException("Id must be assigned");

    if (!component.assigned())
        throw ArgumentNullException("Component must be assigned");
}

ComponentHolderImpl::ComponentHolderImpl(const ComponentPtr& component)
    : ComponentHolderImpl(component.assigned() ? component.getLocalId() : nullptr, component)
{
}

ErrCode ComponentHolderImpl::getLocalId(IString** localId)
{
    OPENDAQ_PARAM_NOT_NULL(localId);

    *localId = this->localId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentHolderImpl::getComponent(IComponent** component)
{
    OPENDAQ_PARAM_NOT_NULL(localId);

    *component = this->component.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentHolderImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    const auto serializerPtr = SerializerPtr::Borrow(serializer);

    return daqTry(
        [this, &serializerPtr]
        {
            serializerPtr.startTaggedObject(borrowPtr<SerializablePtr>());
            {
                serializerPtr.key(localId);
                component.serialize(serializerPtr);
            }

            serializerPtr.endObject();
        });
}

ErrCode ComponentHolderImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr ComponentHolderImpl::SerializeId()
{
    return "ComponentHolder";
}

ErrCode ComponentHolderImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(context);
    OPENDAQ_PARAM_NOT_NULL(obj);

    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    if (!contextPtr.supportsInterface<IComponentDeserializeContext>())
        throw InvalidParameterException("Invalid context");

    const auto deserializeContextPtr = contextPtr.asPtr<IComponentDeserializeContext>(true);

    return daqTry(
        [&serializedObj, &deserializeContextPtr, &factoryCallbackPtr, &obj]
        {
            const auto keys = serializedObj.getKeys();
            if (keys.getCount() != 2)
                throw InvalidValueException("Invalid structure of component holder");

            if (keys[0] != "__type")
                throw InvalidValueException("Invalid structure of component holder");

            const auto rootKey = keys[1];

            const auto newDeserializeContextPtr = deserializeContextPtr.clone(deserializeContextPtr.getParent(), rootKey);

            const ComponentPtr comp = serializedObj.readObject(rootKey, newDeserializeContextPtr, factoryCallbackPtr);

            *obj = createWithImplementation<IComponentHolder, ComponentHolderImpl>(rootKey, comp).detach();
        });
}

}
