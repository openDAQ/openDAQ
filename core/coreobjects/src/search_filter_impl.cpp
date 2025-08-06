#include <coreobjects/search_filter_impl.h>
#include <coretypes/validation.h>
#include <coreobjects/property_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

static ErrCode validateType(IBaseObject* obj)
{
    auto objPtr = BaseObjectPtr::Borrow(obj);
    if (objPtr.supportsInterface<IProperty>())
        return OPENDAQ_SUCCESS;

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE, "Search filter input mismatch acceptable object type - IProperty expected");
}

VisiblePropertyFilterImpl::VisiblePropertyFilterImpl() = default;

ErrCode VisiblePropertyFilterImpl::acceptsObject(IBaseObject* obj, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    const ErrCode errCode = daqTry([&]
    {
        const auto& propertyPtr = PropertyPtr::Borrow(obj);
        *accepts = propertyPtr.getVisible();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode VisiblePropertyFilterImpl::visitChildren(IBaseObject* obj, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    const ErrCode errCode = daqTry([&]
    {
        const auto& propertyPtr = PropertyPtr::Borrow(obj);
        *visit = propertyPtr.getVisible();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ReadOnlyPropertyFilterImpl::ReadOnlyPropertyFilterImpl() = default;

ErrCode ReadOnlyPropertyFilterImpl::acceptsObject(IBaseObject* obj, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    const ErrCode errCode = daqTry([&]
    {
        const auto& propertyPtr = PropertyPtr::Borrow(obj);
        *accepts = propertyPtr.getReadOnly();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode ReadOnlyPropertyFilterImpl::visitChildren(IBaseObject* obj, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    *visit = true;
    return OPENDAQ_SUCCESS;
}

NamePropertyFilterImpl::NamePropertyFilterImpl(const StringPtr& regex)
    : regex(regex.toStdString())
{
}

ErrCode NamePropertyFilterImpl::acceptsObject(IBaseObject* obj, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    const ErrCode errCode = daqTry([&]
    {
        const auto& propertyPtr = PropertyPtr::Borrow(obj);
        *accepts = std::regex_search(propertyPtr.getName().toStdString(), regex);

        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode NamePropertyFilterImpl::visitChildren(IBaseObject* obj, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    *visit = true;
    return OPENDAQ_SUCCESS;
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createVisiblePropertyFilter(ISearchFilter** objTmp)
{
    return createObject<ISearchFilter, VisiblePropertyFilterImpl>(objTmp);
}

extern "C"
    ErrCode PUBLIC_EXPORT createReadOnlyPropertyFilter(ISearchFilter** objTmp)
{
    return createObject<ISearchFilter, ReadOnlyPropertyFilterImpl>(objTmp);
}

extern "C" ErrCode PUBLIC_EXPORT createNamePropertyFilter(ISearchFilter** objTmp, IString* regex)
{
    return createObject<ISearchFilter, NamePropertyFilterImpl>(objTmp, regex);
}

#endif

END_NAMESPACE_OPENDAQ
