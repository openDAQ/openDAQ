#include <coreobjects/owning_dict_impl.h>
#include <coreobjects/ownable_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

OwningDictImpl::OwningDictImpl(PropertyObjectPtr owner, StringPtr ref)
    : ref(std::move(ref))
    , owner(std::move(owner))
{
}

ErrCode OwningDictImpl::removeOwner(IBaseObject* value) const
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

ErrCode OwningDictImpl::set(IBaseObject* key, IBaseObject* value)
{
    ErrCode err = DictImpl::set(key, value);
    OPENDAQ_RETURN_IF_FAILED(err);

    auto ownable = BaseObjectPtr::Borrow(value).asPtrOrNull<IOwnable>();
    if (ownable.assigned())
    {
        err = ownable->setOwner(owner);
    }

    return err;
}

ErrCode OwningDictImpl::remove(IBaseObject* key, IBaseObject** value)
{
    ErrCode err = DictImpl::remove(key, value);
    OPENDAQ_RETURN_IF_FAILED(err);

    if (*value != nullptr)
    {
        removeOwner(*value);
    }

    return err;
}

ErrCode OwningDictImpl::deleteItem(IBaseObject* key)
{
    bool deleted;
    IBaseObject* value;

    ErrCode err = deleteItemInternal(key, &value, deleted);
    OPENDAQ_RETURN_IF_FAILED(err);

    if (!deleted)
    {
        err = removeOwner(value);
    }

    return err;
}

ErrCode OwningDictImpl::clear()
{
    if (frozen)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_FROZEN);
    }

    ErrCode err;
    for (auto [key, value] : hashTable)
    {
        err = removeOwner(value);
        if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_FROZEN)
        {
            return DAQ_MAKE_ERROR_INFO(err);
        }
    }

    err = DictImpl::clear();
    return err;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, OwningDict, IDict, IPropertyObject*, owner, IString*, ref)

END_NAMESPACE_OPENDAQ
