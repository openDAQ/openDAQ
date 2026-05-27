#include <coretypes/listobject_impl.h>
#include <iterator>
#include <coretypes/errors.h>
#include <coretypes/impl.h>
#include <coretypes/ctutils.h>
#include <coretypes/iterator_base_impl.h>
#include <coretypes/cycle_detector.h>
#include <coretypes/list_ptr.h>
#include <coretypes/baseobject_factory.h>
#include <coretypes/validation.h>
#include <coretypes/serialization.h>
#include "coretypes/stringobject_factory.h"

BEGIN_NAMESPACE_OPENDAQ
class ListIteratorImpl : public IteratorBaseImpl<std::vector<IBaseObject*>, IListElementType>
{
public:
    ListIteratorImpl(ListImpl* list, std::vector<IBaseObject*>::iterator it);

    ErrCode INTERFACE_FUNC getElementInterfaceId(IntfID* id) override;
private:
    const IntfID& valueId;
};

ListImpl::ListImpl(IntfID id)
    : iid(id)
{
    list.reserve(16);
}

ListImpl::ListImpl()
    : ListImpl(IUnknown::Id)
{
}

ErrCode ListImpl::getElementInterfaceId(IntfID* id)
{
    if (id == nullptr)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Interface id used as an out-parameter must not be null");
    }

    *id = iid;
    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::toString(CharPtr* str)
{
    std::ostringstream stream;
    stream << "[";
    if (daqCycleDetectEnter(getThisAsBaseObject()) == 0)
    {
        stream << " ... ";
    }
    else
    {
        auto it = list.begin();
        if (it != list.end())
        {
            stream << " " << objectToString(*it);
            for (auto itemIt = ++it; itemIt != list.end(); ++itemIt)
                stream << ", " << objectToString(*itemIt);
            stream << " ";
        }
        daqCycleDetectLeave(getThisAsBaseObject());
    }
    stream << "]";
    return daqDuplicateCharPtr(stream.str().c_str(), str);
}

ErrCode ListImpl::clone(IBaseObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    ListImpl* lst = new(std::nothrow) ListImpl(iid);
    if (lst == nullptr)
    {
        *cloned = nullptr;
        return OPENDAQ_SUCCESS;
    }

    auto size = this->list.size();

    lst->list.reserve(size);
    for (SizeT i = 0; i < size; i++)
    {
        BaseObjectPtr valPtr = BaseObjectPtr::Borrow(this->list[i]);
        if (const auto cloneable = valPtr.asPtrOrNull<ICloneable>(); cloneable.assigned())
        {
            BaseObjectPtr clonedVal;
            const ErrCode err = cloneable->clone(&clonedVal);
            OPENDAQ_RETURN_IF_FAILED(err);

            lst->pushBack(clonedVal);
        }
        else
        {
            lst->pushBack(this->list[i]);
        }
    }

    ErrCode errCode = lst->queryInterface(IBaseObject::Id, reinterpret_cast<void**>(cloned));
    if (errCode == OPENDAQ_ERR_NOINTERFACE)
        return DAQ_MAKE_ERROR_INFO(errCode);
    return errCode;
}

ErrCode INTERFACE_FUNC ListImpl::equals(IBaseObject* other, Bool* equal) const
{
    if (equal == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Equal output parameter must not be null");

    if (!other)
    {
        *equal = false;
        return OPENDAQ_SUCCESS;
    }

    auto otherList = BaseObjectPtr::Borrow(other).asPtrOrNull<IList>();
    if (otherList == nullptr)
    {
        *equal = false;
        return OPENDAQ_SUCCESS;
    }

    SizeT size = this->list.size(); 
    if (size != otherList.getCount())
    {
        *equal = false;
        return OPENDAQ_SUCCESS;
    }

    *equal = true;

    for (SizeT i = 0; i < size; i++)
    {
        const auto item = list.at(i);
        const auto otherItem = otherList.getItemAt(i);

        Bool eq{};
        if (!(item == otherItem || (OPENDAQ_SUCCEEDED(item->equals(otherItem, &eq)) && eq)))
        {
            *equal = false;
            return OPENDAQ_SUCCESS;
        }
    }

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::getItemAt(SizeT index, IBaseObject** item)
{
    OPENDAQ_PARAM_NOT_NULL(item);
    
    if (index >= list.size())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OUTOFRANGE);

    IBaseObject* obj = list[index];
    if (obj)
        obj->addRef();

    *item = obj;

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::getCount(SizeT* size)
{
    *size = list.size();
    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::setItemAt(SizeT index, IBaseObject* obj)
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    if (index >= list.size())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OUTOFRANGE);

    IBaseObject* oldObj = list[index];
    if (oldObj != nullptr)
        oldObj->releaseRef();

    list[index] = obj;

    if (obj)
        obj->addRef();

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::pushBack(IBaseObject* obj)
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    list.push_back(obj);
    if (obj)
        obj->addRef();

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::pushFront(IBaseObject* obj)
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    list.insert(list.begin(), obj);
    if (obj)
        obj->addRef();

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::moveBack(IBaseObject* obj)
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    list.push_back(obj);
    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::moveFront(IBaseObject* obj)
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    list.insert(list.begin(), obj);
    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::popBack(IBaseObject** obj)
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    OPENDAQ_PARAM_NOT_NULL(obj);
    
    if (!list.empty())
    {
        *obj = list.back();
        list.pop_back();
        return OPENDAQ_SUCCESS;
    }

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);
}

ErrCode ListImpl::popFront(IBaseObject** obj)
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    OPENDAQ_PARAM_NOT_NULL(obj);

    if (!list.empty())
    {
        *obj = list.front();
        list.erase(list.begin());
        return OPENDAQ_SUCCESS;
    }

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);
}

ErrCode ListImpl::insertAt(SizeT index, IBaseObject* obj)
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    if (index >= list.size())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OUTOFRANGE);

    list.insert(list.begin() + index, obj);
    if (obj)
        obj->addRef();

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::removeAt(SizeT index, IBaseObject** obj)
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    if (index >= list.size())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OUTOFRANGE);

    *obj = list[index];
    list.erase(list.begin() + index);

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::deleteAtInternal(SizeT index, IBaseObject** removed, bool& deleted)
{
    deleted = false;

    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    if (index >= list.size())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OUTOFRANGE);

    IBaseObject* obj = list[index];
    if (obj)
        deleted = obj->releaseRef() == 0;

    if (removed != nullptr)
    {
        *removed = obj;
    }

    list.erase(list.begin() + index);

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::deleteAt(SizeT index)
{
    bool deleted;
    return deleteAtInternal(index, nullptr, deleted);
}

ErrCode ListImpl::clear()
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    releaseRefOnChildren();
    list.clear();
    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::createStartIterator(IIterator** iterator)
{
    OPENDAQ_PARAM_NOT_NULL(iterator);

    *iterator = new(std::nothrow) ListIteratorImpl(this, list.begin());
    if (*iterator == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOMEMORY);

    (*iterator)->addRef();

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::createEndIterator(IIterator** iterator)
{
    OPENDAQ_PARAM_NOT_NULL(iterator);

    *iterator = new(std::nothrow) ListIteratorImpl(this, list.end());
    if (*iterator == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOMEMORY);

    (*iterator)->addRef();

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::getCapacity(SizeT* capacity)
{
    OPENDAQ_PARAM_NOT_NULL(capacity);

    *capacity = list.capacity();
    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::reserve(SizeT capacity)
{
    if (frozen)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);

    list.reserve(capacity);
    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::insertRangeAtInternal(SizeT index, IList* other, bool moveRefs)
{
    if (other == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "List to insert must not be null");

    if (other == this->borrowInterface())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Cannot insert a list into itself");

    if (frozen)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);

    if (index > list.size())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OUTOFRANGE, "Index {} is out of range {}", index, list.size());

    SizeT otherCount = 0;
    ErrCode err = other->getCount(&otherCount);
    OPENDAQ_RETURN_IF_FAILED(err);

    if (otherCount == 0)
        return OPENDAQ_SUCCESS;

    auto* otherImpl = static_cast<ListImpl*>(other);
    const auto insertPos = std::next(list.begin(), index);
    list.insert(insertPos, otherImpl->list.begin(), otherImpl->list.end());

    if (moveRefs)
    {
        otherImpl->list.clear();
    }
    else
    {
        for (SizeT i = 0; i < otherCount; ++i)
        {
            if (IBaseObject* obj = list[index + i])
                obj->addRef();
        }
    }

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::pushBackRange(IList* other)
{
    SizeT size = list.size();
    return insertRangeAtInternal(size, other, false);
}

ErrCode ListImpl::moveBackRange(IList* other)
{
    SizeT size = list.size();
    return insertRangeAtInternal(size, other, true);
}

ErrCode ListImpl::insertRangeAt(SizeT index, IList* other)
{
    return insertRangeAtInternal(index, other, false);
}

ErrCode ListImpl::moveRangeAt(SizeT index, IList* other)
{
    return insertRangeAtInternal(index, other, true);
}

ErrCode ListImpl::removeRange(SizeT index, SizeT count)
{
    if (frozen)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);

    if (count == 0)
        return OPENDAQ_SUCCESS;

    if (index >= list.size() || count > list.size() - index)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OUTOFRANGE);

    const auto beginIt = std::next(list.begin(), index);
    const auto endIt = std::next(beginIt, count);

    for (auto it = beginIt; it != endIt; ++it)
    {
        if (*it)
            (*it)->releaseRef();
    }

    list.erase(beginIt, endIt);
    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::getRange(SizeT index, SizeT count, IList** range)
{
    OPENDAQ_PARAM_NOT_NULL(range);

    if (count > 0 && (index >= list.size() || count > list.size() - index))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_OUTOFRANGE);

    ListPtr<IBaseObject> rangeList = createWithImplementation<IList, ListImpl>(iid);
    auto* rangeListImpl = static_cast<ListImpl*>(rangeList.getObject());

    const auto first = std::next(list.begin(), index);
    const auto last = std::next(first, count);
    rangeListImpl->list.insert(rangeListImpl->list.end(), first, last);

    for (IBaseObject* obj : rangeListImpl->list)
    {
        if (obj)
            obj->addRef();
    }

    *range = rangeList.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::getCoreType(CoreType* coreType)
{
    OPENDAQ_PARAM_NOT_NULL(coreType);
    
    *coreType = ctList;
    return OPENDAQ_SUCCESS;
}

void ListImpl::releaseRefOnChildren()
{
    for (auto item : list)
    {
        if (item)
            item->releaseRef();
    }
}

void ListImpl::internalDispose(bool)
{
    releaseRefOnChildren();
}

#if defined(coretypes_EXPORTS)
    OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, List)

    OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
        LIBRARY_FACTORY, List, IList, createListWithElementType,
        IntfID, id
    )
#endif

ErrCode ListImpl::freeze()
{
    if (frozen)
        return OPENDAQ_IGNORED;

    frozen = true;

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::isFrozen(Bool* isFrozen) const
{
    *isFrozen = frozen;

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);
    Int version;
    ErrCode err = serializer->getVersion(&version);
    OPENDAQ_RETURN_IF_FAILED(err);

    if (version > 1)
    {
        serializer->startTaggedObject(this);

        if (!(iid == IUnknown::Id))
        {
            serializer->key("itemIntfID");
            
            char iidString[39];
            daqInterfaceIdToString(iid, iidString);
            serializer->writeString(iidString, 38);
        }

        serializer->key("values");
    }

    serializer->startList();

    for (const auto& element : list)
    {
        if (!element)
        {
            serializer->writeNull();
            continue;
        }

        ISerializable* serializableElement;
        ErrCode errCode = element->borrowInterface(ISerializable::Id, reinterpret_cast<void**>(&serializableElement));
        if (OPENDAQ_FAILED(errCode))
        {
            if (errCode == OPENDAQ_ERR_NOINTERFACE)
                return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SERIALIZABLE);
            return DAQ_EXTEND_ERROR_INFO(errCode);
        }

        errCode = serializableElement->serialize(serializer);
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    serializer->endList();

    if (version > 1)
    {
        serializer->endObject();
    }

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr ListImpl::SerializeId()
{
    return "List";
}

ErrCode INTERFACE_FUNC deserializeList(ISerializedObject* ser, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    Bool hasKey = false;
    IntfID id = IUnknown::Id;
    ser->hasKey(String("itemIntfID"), &hasKey);
    if (hasKey)
    {
        StringPtr str;
        ser->readString(String("itemIntfID"), &str);
        daqStringToInterfaceId(str.getCharPtr(), id);
    }

    SerializedListPtr list = nullptr;
    ser->readSerializedList(String("values"), &list);

    ListPtr<IBaseObject> listObj = createWithImplementation<IList, ListImpl>(id);
    for (SizeT i = 0; i < list.getCount(); i++)
    {
        listObj.pushBack(list.readObject(context, factoryCallback));
    }

    *obj = listObj.detach();
    return OPENDAQ_SUCCESS;
}

ListIteratorImpl::ListIteratorImpl(ListImpl* list, std::vector<IBaseObject*>::iterator it)
    : IteratorBaseImpl<std::vector<IBaseObject*>, IListElementType>(asOrNull<IBaseObject>(list, true), std::move(it), list->list.end())
    , valueId(list->iid)
{
}

ErrCode ListIteratorImpl::getElementInterfaceId(IntfID* id)
{
    if (id == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Id output parameter must not be null.");

    *id = valueId;
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
