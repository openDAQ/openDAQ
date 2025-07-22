#include <coreobjects/owning_list_impl.h>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

OwningListImpl::OwningListImpl(IPropertyObject* owner, StringPtr ref)
    : ref(std::move(ref))
    , owner(owner)
{
}

ErrCode OwningListImpl::removeOwner(IBaseObject* value) const
{
    if (value == nullptr)
        return OPENDAQ_SUCCESS;

    auto ownable = BaseObjectPtr::Borrow(value).asPtrOrNull<IOwnable>();
    if (!ownable.assigned())
        return OPENDAQ_SUCCESS;

    return ownable->setOwner(nullptr);
}

ErrCode OwningListImpl::setOwner(IBaseObject* value) const
{
    if (value == nullptr)
        return OPENDAQ_SUCCESS;

    auto ownable = BaseObjectPtr::Borrow(value).asPtrOrNull<IOwnable>(true);
    if (!ownable.assigned())
        return OPENDAQ_SUCCESS;

    GenericPropertyObjectPtr lock;
    const ErrCode errCode = owner->getRefAs(IPropertyObject::Id, reinterpret_cast<void**>(&lock));
    OPENDAQ_RETURN_IF_FAILED(errCode);

    return ownable->setOwner(lock);
}

ErrCode OwningListImpl::setItemAt(SizeT index, IBaseObject* obj)
{
    ErrCode err = ListImpl::setItemAt(index, obj);
    OPENDAQ_RETURN_IF_FAILED(err);

    return setOwner(obj);
}

ErrCode OwningListImpl::pushBack(IBaseObject* obj)
{
    ErrCode err = ListImpl::pushBack(obj);
    OPENDAQ_RETURN_IF_FAILED(err);

    return setOwner(obj);
}

ErrCode OwningListImpl::pushFront(IBaseObject* obj)
{
    ErrCode err = ListImpl::pushFront(obj);
    OPENDAQ_RETURN_IF_FAILED(err);

    return setOwner(obj);
}

ErrCode OwningListImpl::moveBack(IBaseObject* obj)
{
    ErrCode err = ListImpl::moveBack(obj);
    OPENDAQ_RETURN_IF_FAILED(err);

    return setOwner(obj);
}

ErrCode OwningListImpl::moveFront(IBaseObject* obj)
{
    ErrCode err = ListImpl::moveFront(obj);
    OPENDAQ_RETURN_IF_FAILED(err);

    return setOwner(obj);
}

ErrCode OwningListImpl::insertAt(SizeT index, IBaseObject* obj)
{
    ErrCode err = ListImpl::insertAt(index, obj);
    OPENDAQ_RETURN_IF_FAILED(err);

    return setOwner(obj);
}

ErrCode OwningListImpl::popBack(IBaseObject** obj)
{
    ErrCode err = ListImpl::popBack(obj);
    OPENDAQ_RETURN_IF_FAILED(err);

    return removeOwner(*obj);
}

ErrCode OwningListImpl::popFront(IBaseObject** obj)
{
    ErrCode err = ListImpl::popFront(obj);
    OPENDAQ_RETURN_IF_FAILED(err);

    return removeOwner(*obj);
}

ErrCode OwningListImpl::removeAt(SizeT index, IBaseObject** obj)
{
    ErrCode err = ListImpl::removeAt(index, obj);
    OPENDAQ_RETURN_IF_FAILED(err);

    if (*obj != nullptr)
    {
        removeOwner(*obj);
    }

    return err;
}

ErrCode OwningListImpl::deleteAt(SizeT index)
{
    bool deleted;
    IBaseObject* value;

    ErrCode err = deleteAtInternal(index, &value, deleted);
    OPENDAQ_RETURN_IF_FAILED(err);

    if (!deleted)
    {
        err = removeOwner(value);
    }

    return err;
}

ErrCode OwningListImpl::clear()
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    ErrCode err;
    for (IBaseObject* value : list)
    {
        err = removeOwner(value);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(err, OPENDAQ_ERR_FROZEN);
    }

    err = ListImpl::clear();
    return err;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, OwningList, IList, IPropertyObject*, owner, IString*, ref)

END_NAMESPACE_OPENDAQ
