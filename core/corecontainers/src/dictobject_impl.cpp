#include <coretypes/dictobject_impl.h>
#include <coretypes/serialization.h>
#include <coretypes/impl.h>
#include <coretypes/dict_ptr.h>
#include <coretypes/dictobject_iterable_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

DictImpl::DictImpl(IntfID keyId, IntfID valueId)
    : keyId(keyId)
    , valueId(valueId)
{
}

ErrCode DictImpl::getKeyInterfaceId(IntfID* id)
{
    if (id == nullptr)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Interface id used as an out-parameter must not be null");
    }

    *id = keyId;
    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::getValueInterfaceId(IntfID* id)
{
    if (id == nullptr)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Interface id used as an out-parameter must not be null");
    }

    *id = valueId;
    return OPENDAQ_SUCCESS;
}

void DictImpl::internalDispose(bool)
{
    releaseRefOnChildren();
}

ErrCode DictImpl::get(IBaseObject* key, IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(key);
    OPENDAQ_PARAM_NOT_NULL(value);

    auto item = hashTable.find(key);
    if (item == hashTable.end())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

    *value = item->second;
    if (*value != nullptr)
        (*value)->addRef();

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::set(IBaseObject* key, IBaseObject* value)
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    OPENDAQ_PARAM_NOT_NULL(key);

    auto oldItem = hashTable.find(key);
    if (oldItem == hashTable.end())
    {
        hashTable.insert(std::make_pair(key, value));
        key->addRef();
        if (value != nullptr)
            value->addRef();
    }
    else
    {
        if (oldItem->second)
            oldItem->second->releaseRef();

        oldItem.value() = value;
        if (value != nullptr)
            value->addRef();
    }

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::remove(IBaseObject* key, IBaseObject** value)
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    OPENDAQ_PARAM_NOT_NULL(key);
    OPENDAQ_PARAM_NOT_NULL(value);

    auto item = hashTable.find(key);
    if (item == hashTable.end())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

    IBaseObject* tmpKey = item->first;
    IBaseObject* tmpValue = item->second;

    hashTable.erase(item);

    tmpKey->releaseRef();
    *value = tmpValue;

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::deleteItemInternal(IBaseObject* key, IBaseObject** obj, bool& deleted)
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    OPENDAQ_PARAM_NOT_NULL(key);

    auto item = hashTable.find(key);
    if (item == hashTable.end())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);

    deleted = false;
    IBaseObject* tmpKey = item->first;
    IBaseObject* tmpValue = item->second;

    if (obj != nullptr)
    {
        *obj = tmpValue;
    }

    hashTable.erase(item);

    tmpKey->releaseRef();
    if (tmpValue != nullptr)
        deleted = tmpValue->releaseRef() == 0;

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::deleteItem(IBaseObject* key)
{
    bool deleted;
    return deleteItemInternal(key, nullptr, deleted);
}

ErrCode DictImpl::clear()
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    releaseRefOnChildren();
    hashTable.clear();

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::getCount(SizeT* count)
{
    OPENDAQ_PARAM_NOT_NULL(count);

    *count = hashTable.size();
    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::hasKey(IBaseObject* key, Bool* hasKey)
{
    *hasKey = hashTable.find(key) != hashTable.cend();

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::enumerate(const std::function<IBaseObject*(const BaseObjectPair&)>& chooseElement, IList** list)
{
    OPENDAQ_PARAM_NOT_NULL(list);
    
    auto errCode = createList(list);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    for (auto& item : hashTable)
    {
        const auto obj = chooseElement(item);
        (*list)->pushBack(obj);
    }

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::getKeyList(IList** keys)
{
    return enumerate([](const BaseObjectPair& item)
                     {
                         return item.first;
                     },
                     keys);
}

ErrCode DictImpl::getValueList(IList** values)
{
    return enumerate([](const BaseObjectPair& item)
                     {
                         return item.second;
                     },
                     values);
}

ErrCode DictImpl::getKeys(IIterable** iterable)
{
    OPENDAQ_PARAM_NOT_NULL(iterable);
    
    // have to pass with std::cref so it doesn't create a dangling copy without perfect-forwarding
    return createObject<IIterable, DictKeyIterableImpl>(iterable, this, std::cref(keyId));
}

ErrCode DictImpl::getValues(IIterable** iterable)
{
    OPENDAQ_PARAM_NOT_NULL(iterable);
    
    // have to pass with std::cref so it doesn't create a dangling copy without perfect-forwarding
    return createObject<IIterable, DictValueIterableImpl>(iterable, this, std::cref(valueId));
}

ErrCode DictImpl::createStartIterator(IIterator** iterator)
{
    OPENDAQ_PARAM_NOT_NULL(iterator);
    
    *iterator = new (std::nothrow) DictIterator<decltype(hashTable)>(
        borrowInterface<IBaseObject>(),
        hashTable.begin(),
        hashTable.end(),
        keyId,
        valueId
    );

    if (*iterator == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOMEMORY);

    (*iterator)->addRef();

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::createEndIterator(IIterator** iterator)
{
    OPENDAQ_PARAM_NOT_NULL(iterator);

    *iterator = new(std::nothrow) DictIterator<decltype(hashTable)>(
        borrowInterface<IBaseObject>(),
        hashTable.end(),
        hashTable.end(),
        keyId,
        valueId
    );

    if (*iterator == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOMEMORY);

    (*iterator)->addRef();

    return OPENDAQ_SUCCESS;
}

void DictImpl::releaseRefOnChildren()
{
    for (auto item : hashTable)
    {
        item.first->releaseRef();
        if (item.second)
            item.second->releaseRef();
    }
}

ErrCode DictImpl::getCoreType(CoreType* coreType)
{
    *coreType = ctDict;
    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::freeze()
{
    if (frozen)
        return OPENDAQ_IGNORED;

    frozen = true;

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::isFrozen(Bool* isFrozen) const
{
    *isFrozen = frozen;

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::clone(IBaseObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    DictImpl* lst = new(std::nothrow) DictImpl(keyId, valueId);
    if (lst == nullptr)
    {
        *cloned = nullptr;
        return OPENDAQ_SUCCESS;
    }

    auto size = this->hashTable.size();
    lst->hashTable.reserve(size);
    for (const auto& item : hashTable)
    {
        ObjectPtr<ICloneable> keyCloneable;
        IBaseObject* key = nullptr;

        ErrCode err = item.first->queryInterface(ICloneable::Id, reinterpret_cast<void**>(&keyCloneable));
        if (OPENDAQ_SUCCEEDED(err))
            keyCloneable->clone(&key);
        else
        {
            item.first->addRef();
            key = item.first;
        }
        
        ObjectPtr<ICloneable> valCloneable;
        IBaseObject* val = nullptr;

        if (item.second)
        {
            err = item.second->queryInterface(ICloneable::Id, reinterpret_cast<void**>(&valCloneable));
            if (OPENDAQ_SUCCEEDED(err))
                valCloneable->clone(&val);
            else
            {
                item.second->addRef();
                val = item.second;
            }
        }
        
        lst->hashTable.insert(std::make_pair(key, val));
    }

    return lst->queryInterface(IBaseObject::Id, reinterpret_cast<void**>(cloned));
}

ErrCode INTERFACE_FUNC DictImpl::equals(IBaseObject* other, Bool* equal) const
{
    if (equal == nullptr)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Equal output parameter must not be null.");
    }

    *equal = false;
    if (!other)
        return OPENDAQ_SUCCESS;

    DictObjectPtr<IDict, IBaseObject, IBaseObject> dict = BaseObjectPtr::Borrow(other).asPtrOrNull<IDict>();
    if (dict == nullptr)
        return OPENDAQ_SUCCESS;

    if (dict.getCount() != hashTable.size())
        return OPENDAQ_SUCCESS;

    try
    {
        for (const auto& [key, value] : dict)
        {
            if (!this->hashTable.contains(key))
                return OPENDAQ_SUCCESS;

            auto otherValue = this->hashTable.at(key);
            if (!value.equals(otherValue))
                return OPENDAQ_SUCCESS;
        }
    }
    catch (const DaqException&)
    {
        return OPENDAQ_SUCCESS;
    }

    *equal = true;
    return OPENDAQ_SUCCESS;
}

// static
ConstCharPtr DictImpl::SerializeId()
{
    return "Dict";
}

ErrCode DictImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    Int version;
    serializer->getVersion(&version);
    if (version > 1)
    {
        if (!(keyId == IUnknown::Id))
        {
            serializer->key("keyIntfID");
            
            char iidString[39];
            daqInterfaceIdToString(keyId, iidString);
            serializer->writeString(iidString, 38);
        }

        if (!(valueId == IUnknown::Id))
        {
            serializer->key("valueIntfID");
            
            char iidString[39];
            daqInterfaceIdToString(valueId, iidString);
            serializer->writeString(iidString, 38);
        }
    }

    serializer->key("values");
    serializer->startList();

    for (const auto& keyValue : hashTable)
    {
        serializer->startObject();
        serializer->key("key");

        ISerializable* serializableKey;
        ErrCode errCode = keyValue.first->borrowInterface(ISerializable::Id, reinterpret_cast<void**>(&serializableKey));

        if (errCode == OPENDAQ_ERR_NOINTERFACE)
        {
            return DAQ_EXTEND_ERROR_INFO(errCode, OPENDAQ_ERR_NOT_SERIALIZABLE);
        }

        OPENDAQ_RETURN_IF_FAILED(errCode);
        serializableKey->serialize(serializer);

        serializer->key("value");

        if (!keyValue.second)
        {
            serializer->writeNull();
        }
        else
        {
            ISerializable* serializableValue;
            errCode = keyValue.second->borrowInterface(ISerializable::Id, reinterpret_cast<void**>(&serializableValue));

            if (errCode == OPENDAQ_ERR_NOINTERFACE)
            {
                return DAQ_EXTEND_ERROR_INFO(errCode, OPENDAQ_ERR_NOT_SERIALIZABLE);
            }

            OPENDAQ_RETURN_IF_FAILED(errCode);

            errCode = serializableValue->serialize(serializer);
            OPENDAQ_RETURN_IF_FAILED(errCode);
        }

        serializer->endObject();
    }

    serializer->endList();

    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC deserializeDict(ISerializedObject* ser, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    Bool hasKey = false;
    IntfID keyId = IUnknown::Id;
    ser->hasKey(String("keyIntfID"), &hasKey);
    if (hasKey)
    {
        StringPtr str;
        ser->readString(String("keyIntfID"), &str);
        daqStringToInterfaceId(str.getCharPtr(), keyId);
    }
    
    hasKey = false;
    IntfID valueId = IUnknown::Id;
    ser->hasKey(String("valueIntfID"), &hasKey);
    if (hasKey)
    {
        StringPtr str;
        ser->readString(String("valueIntfID"), &str);
        daqStringToInterfaceId(str.getCharPtr(), valueId);
    }

    SerializedListPtr list = nullptr;
    ser->readSerializedList(String("values"), &list);

    SizeT length;
    list->getCount(&length);
    
    DictObjectPtr<IDict, IBaseObject, IBaseObject> dict = createWithImplementation<IDict, DictImpl>(keyId, valueId);

    for (SizeT i = 0; i < length; i++)
    {
        SerializedObjectPtr keyValue;
        ErrCode errCode = list->readSerializedObject(&keyValue);

        OPENDAQ_RETURN_IF_FAILED(errCode);

        BaseObjectPtr key;
        errCode = keyValue->readObject(String("key"), context, factoryCallback, &key);

        OPENDAQ_RETURN_IF_FAILED(errCode);

        BaseObjectPtr value;
        errCode = keyValue->readObject(String("value"), context, factoryCallback, &value);

        OPENDAQ_RETURN_IF_FAILED(errCode);

        dict->set(key, value);
    }

    *obj = dict.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

#if defined(coretypes_EXPORTS)
    OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, Dict)

    OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
        LIBRARY_FACTORY, Dict, IDict, createDictWithExpectedTypes,
        IntfID, keyType,
        IntfID, valueType
    )
#endif

END_NAMESPACE_OPENDAQ
