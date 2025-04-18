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
    {
        return OPENDAQ_SUCCESS;
    }

    auto ownable = BaseObjectPtr::Borrow(value).asPtrOrNull<IOwnable>();
    if (ownable.assigned())
    {
        return ownable->setOwner(nullptr);
    }
    return OPENDAQ_SUCCESS;
}

ErrCode OwningListImpl::setOwner(IBaseObject* value) const
{
    if (value == nullptr)
    {
        return OPENDAQ_SUCCESS;
    }

    ErrCode err = OPENDAQ_SUCCESS;

    auto ownable = BaseObjectPtr::Borrow(value).asPtrOrNull<IOwnable>();
    if (ownable.assigned())
    {
        GenericPropertyObjectPtr lock;
        err = owner->getRefAs(IPropertyObject::Id, reinterpret_cast<void**>(&lock));

        if (OPENDAQ_SUCCEEDED(err))
        {
            return ownable->setOwner(lock);
        }
    }
    return err;
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
        if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_FROZEN)
        {
            return DAQ_MAKE_ERROR_INFO(err);
        }
    }

    err = ListImpl::clear();
    return err;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, OwningList, IList, IPropertyObject*, owner, IString*, ref)

END_NAMESPACE_OPENDAQ
