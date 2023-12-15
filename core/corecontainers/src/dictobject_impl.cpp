#include <coretypes/dictobject_impl.h>
#include <coretypes/serialization.h>
#include <coretypes/impl.h>
#include <coretypes/dict_ptr.h>
#include <coretypes/dictobject_iterable_impl.h>

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
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Interface id used as an out-parameter must not be null");
    }

    *id = keyId;
    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::getValueInterfaceId(IntfID* id)
{
    if (id == nullptr)
    {
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Interface id used as an out-parameter must not be null");
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
    if (key == nullptr || value == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    auto item = hashTable.find(key);
    if (item == hashTable.end())
        return OPENDAQ_ERR_NOTFOUND;

    *value = item->second;
    if (*value != nullptr)
        (*value)->addRef();

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::set(IBaseObject* key, IBaseObject* value)
{
    if (frozen)
    {
        return OPENDAQ_ERR_FROZEN;
    }

    if (key == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

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
        return OPENDAQ_ERR_FROZEN;
    }

    if (key == nullptr || value == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    auto item = hashTable.find(key);
    if (item == hashTable.end())
        return OPENDAQ_ERR_NOTFOUND;

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
        return OPENDAQ_ERR_FROZEN;
    }

    if (key == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    auto item = hashTable.find(key);
    if (item == hashTable.end())
        return OPENDAQ_ERR_NOTFOUND;

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
        return OPENDAQ_ERR_FROZEN;
    }

    releaseRefOnChildren();
    hashTable.clear();

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::getCount(SizeT* count)
{
    if (count == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

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
    if (list == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    auto errCode = createList(list);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

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
    if (iterable == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    // have to pass with std::cref so it doesn't create a dangling copy without perfect-forwarding
    return createObject<IIterable, DictKeyIterableImpl>(iterable, this, std::cref(keyId));
}

ErrCode DictImpl::getValues(IIterable** iterable)
{
    if (iterable == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    // have to pass with std::cref so it doesn't create a dangling copy without perfect-forwarding
    return createObject<IIterable, DictValueIterableImpl>(iterable, this, std::cref(valueId));
}

ErrCode DictImpl::createStartIterator(IIterator** iterator)
{
    if (iterator == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (iterator == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *iterator = new (std::nothrow) DictIterator<decltype(hashTable)>(
        borrowInterface<IBaseObject>(),
        hashTable.begin(),
        hashTable.end(),
        keyId,
        valueId
    );

    if (*iterator == nullptr)
        return OPENDAQ_ERR_NOMEMORY;

    (*iterator)->addRef();

    return OPENDAQ_SUCCESS;
}

ErrCode DictImpl::createEndIterator(IIterator** iterator)
{
    if (iterator == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (iterator == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *iterator = new(std::nothrow) DictIterator<decltype(hashTable)>(
        borrowInterface<IBaseObject>(),
        hashTable.end(),
        hashTable.end(),
        keyId,
        valueId
    );

    if (*iterator == nullptr)
        return OPENDAQ_ERR_NOMEMORY;

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
        return  OPENDAQ_IGNORED;

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

ErrCode INTERFACE_FUNC DictImpl::equals(IBaseObject* other, Bool* equal) const
{
    if (equal == nullptr)
    {
        return daq::makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equal output parameter must not be null.", nullptr);
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
            return OPENDAQ_ERR_NOT_SERIALIZABLE;
        }

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
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
                return OPENDAQ_ERR_NOT_SERIALIZABLE;
            }

            if (OPENDAQ_FAILED(errCode))
            {
                return errCode;
            }

            errCode = serializableValue->serialize(serializer);
            if (OPENDAQ_FAILED(errCode))
            {
                return errCode;
            }
        }

        serializer->endObject();
    }

    serializer->endList();

    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC deserializeDict(ISerializedObject* ser, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    SerializedListPtr list = nullptr;
    ser->readSerializedList(String("values"), &list);

    SizeT length;
    list->getCount(&length);

    DictObjectPtr<IDict, IBaseObject, IBaseObject> dict = Dict_Create();

    for (SizeT i = 0; i < length; i++)
    {
        SerializedObjectPtr keyValue;
        ErrCode errCode = list->readSerializedObject(&keyValue);

        if (OPENDAQ_FAILED(errCode))
            return errCode;

        BaseObjectPtr key;
        errCode = keyValue->readObject(String("key"), context, factoryCallback, &key);

        if (OPENDAQ_FAILED(errCode))
            return errCode;

        BaseObjectPtr value;
        errCode = keyValue->readObject(String("value"), context, factoryCallback, &value);

        if (OPENDAQ_FAILED(errCode))
            return errCode;

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
