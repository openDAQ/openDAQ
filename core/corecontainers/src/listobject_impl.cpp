#include <coretypes/listobject_impl.h>
#include <coretypes/errors.h>
#include <coretypes/impl.h>
#include <coretypes/ctutils.h>
#include <coretypes/iterator_base_impl.h>
#include <coretypes/cycle_detector.h>
#include <coretypes/list_ptr.h>
#include <coretypes/baseobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class ListIteratorImpl : public IteratorBaseImpl<std::vector<IBaseObject*>, IListElementType>
{
public:
    ListIteratorImpl(ListImpl* list, std::vector<IBaseObject*>::iterator it);

    ErrCode INTERFACE_FUNC getElementInterfaceId(IntfID* id) override;
private:
    const IntfID& valueId;
};

ListImpl::ListImpl()
    : iid(IUnknown::Id)
{
    list.reserve(16);
}

ListImpl::ListImpl(IntfID id)
    : iid(id)
{
    list.reserve(16);
}

ErrCode ListImpl::getElementInterfaceId(IntfID* id)
{
    if (id == nullptr)
    {
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Interface id used as an out-parameter must not be null");
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
    if (cloned == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

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
            if (OPENDAQ_FAILED(err))
                return err;

            lst->pushBack(clonedVal);
        }
        else
        {
            lst->pushBack(this->list[i]);
        }
    }

    return lst->queryInterface(IBaseObject::Id, reinterpret_cast<void**>(cloned));
}

ErrCode INTERFACE_FUNC ListImpl::equals(IBaseObject* other, Bool* equal) const
{
    if (equal == nullptr)
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equal output parameter must not be null");

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
        if (!(item == otherItem || OPENDAQ_SUCCEEDED(item->equals(otherItem, &eq)) && eq))
        {
            *equal = false;
            return OPENDAQ_SUCCESS;
        }
    }

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::getItemAt(SizeT index, IBaseObject** item)
{
    if (item == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (index >= list.size())
        return OPENDAQ_ERR_OUTOFRANGE;

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
        return OPENDAQ_ERR_FROZEN;
    }

    if (index >= list.size())
        return OPENDAQ_ERR_OUTOFRANGE;

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
        return OPENDAQ_ERR_FROZEN;
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
        return OPENDAQ_ERR_FROZEN;
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
        return OPENDAQ_ERR_FROZEN;
    }

    list.push_back(obj);
    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::moveFront(IBaseObject* obj)
{
    if (frozen)
    {
        return OPENDAQ_ERR_FROZEN;
    }

    list.insert(list.begin(), obj);
    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::popBack(IBaseObject** obj)
{
    if (frozen)
    {
        return OPENDAQ_ERR_FROZEN;
    }

    if (obj == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (!list.empty())
    {
        *obj = list.back();
        list.pop_back();
        return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_ERR_NOTFOUND;
}

ErrCode ListImpl::popFront(IBaseObject** obj)
{
    if (frozen)
    {
        return OPENDAQ_ERR_FROZEN;
    }

    if (obj == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (!list.empty())
    {
        *obj = list.front();
        list.erase(list.begin());
        return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_ERR_NOTFOUND;
}

ErrCode ListImpl::insertAt(SizeT index, IBaseObject* obj)
{
    if (frozen)
    {
        return OPENDAQ_ERR_FROZEN;
    }

    if (index >= list.size())
        return OPENDAQ_ERR_OUTOFRANGE;

    list.insert(list.begin() + index, obj);
    if (obj)
        obj->addRef();

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::removeAt(SizeT index, IBaseObject** obj)
{
    if (frozen)
    {
        return OPENDAQ_ERR_FROZEN;
    }

    if (index >= list.size())
        return OPENDAQ_ERR_OUTOFRANGE;

    *obj = list[index];
    list.erase(list.begin() + index);

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::deleteAtInternal(SizeT index, IBaseObject** removed, bool& deleted)
{
    deleted = false;

    if (frozen)
    {
        return OPENDAQ_ERR_FROZEN;
    }

    if (index >= list.size())
        return OPENDAQ_ERR_OUTOFRANGE;

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
        return OPENDAQ_ERR_FROZEN;
    }

    releaseRefOnChildren();
    list.clear();
    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::createStartIterator(IIterator** iterator)
{
    if (iterator == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *iterator = new(std::nothrow) ListIteratorImpl(this, list.begin());
    if (*iterator == nullptr)
        return OPENDAQ_ERR_NOMEMORY;

    (*iterator)->addRef();

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::createEndIterator(IIterator** iterator)
{
    if (iterator == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *iterator = new(std::nothrow) ListIteratorImpl(this, list.end());
    if (*iterator == nullptr)
        return OPENDAQ_ERR_NOMEMORY;

    (*iterator)->addRef();

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::getCoreType(CoreType* coreType)
{
    if (coreType == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

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
        return  OPENDAQ_IGNORED;

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

        if (errCode == OPENDAQ_ERR_NOINTERFACE)
        {
            return OPENDAQ_ERR_NOT_SERIALIZABLE;
        }

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        errCode = serializableElement->serialize(serializer);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }

    serializer->endList();

    return OPENDAQ_SUCCESS;
}

ErrCode ListImpl::getSerializeId(ConstCharPtr* /*id*/) const
{
    // Handled directly by the serializer and deserializer
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ListIteratorImpl::ListIteratorImpl(ListImpl* list, std::vector<IBaseObject*>::iterator it)
    : IteratorBaseImpl<std::vector<IBaseObject*>, IListElementType>(asOrNull<IBaseObject>(list, true), std::move(it), list->list.end())
    , valueId(list->iid)
{
}

ErrCode ListIteratorImpl::getElementInterfaceId(IntfID* id)
{
    if (id == nullptr)
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Id output parameter must not be null.");

    *id = valueId;
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
