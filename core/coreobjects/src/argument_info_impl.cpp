#include <coreobjects/argument_info_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/argument_info_factory.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr argumentInfoStructType = ArgumentInfoStructType();
}

ArgumentInfoImpl::ArgumentInfoImpl(const StringPtr& name, CoreType type, CoreType keyType, CoreType itemType)
    : GenericStructImpl(detail::argumentInfoStructType,
                        Dict<IString, IBaseObject>({{"Name", name}, {"Type", static_cast<Int>(type)},
                                                    {"KeyType", static_cast<Int>(keyType)}, {"ItemType", static_cast<Int>(itemType)}}))
{
    this->name = fields.get("Name");
    this->argType = fields.get("Type");
    this->keyType = fields.get("KeyType");
    this->itemType = fields.get("ItemType");
}

ErrCode ArgumentInfoImpl::getName(IString** argName)
{
    OPENDAQ_PARAM_NOT_NULL(argName);

    *argName = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ArgumentInfoImpl::getType(CoreType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = argType;
    return OPENDAQ_SUCCESS;
}

ErrCode ArgumentInfoImpl::getItemType(CoreType* itemType)
{
    OPENDAQ_PARAM_NOT_NULL(itemType);

    *itemType = this->itemType;
    return OPENDAQ_SUCCESS;
}

ErrCode ArgumentInfoImpl::getKeyType(CoreType* keyType)
{
    OPENDAQ_PARAM_NOT_NULL(keyType);

    *keyType = this->keyType;
    return OPENDAQ_SUCCESS;
}


ErrCode ArgumentInfoImpl::equals(IBaseObject* other, Bool* equal) const
{
    return daqTry([this, &other, &equal]() {
        if (equal == nullptr)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

        *equal = false;
        if (!other)
            return OPENDAQ_SUCCESS;

        ArgumentInfoPtr argInfo = BaseObjectPtr::Borrow(other).asPtrOrNull<IArgumentInfo>();
        if (argInfo == nullptr)
            return OPENDAQ_SUCCESS;

        if (name != argInfo.getName())
            return OPENDAQ_SUCCESS;

        if (argType != argInfo.getType())
            return OPENDAQ_SUCCESS;
        
        if (keyType != argInfo.getKeyType())
            return OPENDAQ_SUCCESS;

        if (itemType != argInfo.getItemType())
            return OPENDAQ_SUCCESS;

        *equal = true;
        return OPENDAQ_SUCCESS;
    });
}

ErrCode ArgumentInfoImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    {
        if (name.assigned())
        {
            serializer->key("name");
            serializer->writeString(name.getCharPtr(), name.getLength());
        }

        serializer->key("type");
        serializer->writeInt(argType);

        if (keyType != ctUndefined)
        {
            serializer->key("keyType");
            serializer->writeInt(keyType);
        }

        if (itemType != ctUndefined)
        {
            serializer->key("itemType");
            serializer->writeInt(itemType);
        }
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode ArgumentInfoImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr ArgumentInfoImpl::SerializeId()
{
    return "ArgumentInfo";
}

ErrCode ArgumentInfoImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    const SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);

    return daqTry([&serializedObj, &obj]
    {
        StringPtr name;
        if (serializedObj.hasKey("name"))
            name = serializedObj.readString("name");

        const auto argType = static_cast<CoreType>(serializedObj.readInt("type"));

        CoreType itemType = ctUndefined;
        if (serializedObj.hasKey("itemType"))
            itemType = static_cast<CoreType>(serializedObj.readInt("itemType"));

        CoreType keyType = ctUndefined;
        if (serializedObj.hasKey("keyType"))
            keyType = static_cast<CoreType>(serializedObj.readInt("keyType"));

        *obj = createWithImplementation<IArgumentInfo, ArgumentInfoImpl>(name, argType, keyType, itemType).detach();
    });
}

extern "C"
ErrCode PUBLIC_EXPORT createArgumentInfo(IArgumentInfo** objTmp,
                                         IString* name,
                                         CoreType type)
{
    return daq::createObject<IArgumentInfo, ArgumentInfoImpl>(objTmp, name, type, ctUndefined, ctUndefined);
}

extern "C"
ErrCode PUBLIC_EXPORT createListArgumentInfo(IArgumentInfo** objTmp,
                                         IString* name,
                                         CoreType itemType)
{
    return daq::createObject<IArgumentInfo, ArgumentInfoImpl>(objTmp, name, ctList, ctUndefined, itemType);
}

extern "C"
ErrCode PUBLIC_EXPORT createDictArgumentInfo(IArgumentInfo** objTmp,
                                         IString* name,
                                         CoreType keyType,
                                         CoreType itemType)
{
    return daq::createObject<IArgumentInfo, ArgumentInfoImpl>(objTmp, name, ctList, keyType, itemType);
}

END_NAMESPACE_OPENDAQ
