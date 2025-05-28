#include <opendaq/search_filter_impl.h>
#include <coretypes/validation.h>
#include <opendaq/component_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

static ErrCode validateType(IBaseObject* obj)
{
    auto objPtr = BaseObjectPtr::Borrow(obj);
    if (objPtr.supportsInterface<IComponent>())
        return OPENDAQ_SUCCESS;

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE, "Search filter input mismatch acceptable object type - IComponent expected");
}

VisibleSearchFilterImpl::VisibleSearchFilterImpl() = default;

ErrCode VisibleSearchFilterImpl::acceptsObject(IBaseObject* obj, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    return daqTry(
        [&]{
            const auto& componentPtr = ComponentPtr::Borrow(obj);
            *accepts = componentPtr.getVisible();
        });
}

ErrCode VisibleSearchFilterImpl::visitChildren(IBaseObject* obj, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    return daqTry(
        [&]{
            const auto& componentPtr = ComponentPtr::Borrow(obj);
            *visit = componentPtr.getVisible();
        });
}

RequiredTagsSearchFilterImpl::RequiredTagsSearchFilterImpl(const ListPtr<IString>& requiredTags)
{
    for (const auto& tag : requiredTags)
        this->requiredTags.insert(tag);
}

ErrCode RequiredTagsSearchFilterImpl::acceptsObject(IBaseObject* obj, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    return daqTry(
        [&]{
            const auto& componentPtr = ComponentPtr::Borrow(obj);
            TagsPtr tags = componentPtr.getTags();

            *accepts = true;
            for (const auto& requiredTag : requiredTags)
            {
                if (!tags.contains(requiredTag))
                {
                    *accepts = false;
                    break;
                }
            }
            return OPENDAQ_SUCCESS;
        });
}

ErrCode RequiredTagsSearchFilterImpl::visitChildren(IBaseObject* obj, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    *visit = true;
    return OPENDAQ_SUCCESS;
}

ExcludedTagsSearchFilterImpl::ExcludedTagsSearchFilterImpl(const ListPtr<IString>& excludedTags)
{
    for (const auto& tag : excludedTags)
        this->excludedTags.insert(tag);
}

ErrCode ExcludedTagsSearchFilterImpl::acceptsObject(IBaseObject* obj, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    return daqTry(
        [&]{
            const auto& componentPtr = ComponentPtr::Borrow(obj);
            TagsPtr tags = componentPtr.getTags();

            *accepts = true;
            for (const auto& tag : tags.getList())
            {
                if (excludedTags.count(tag))
                {
                    *accepts = false;
                    break;
                }
            }
            return OPENDAQ_SUCCESS;
        });
}

ErrCode ExcludedTagsSearchFilterImpl::visitChildren(IBaseObject* obj, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    *visit = true;
    return OPENDAQ_SUCCESS;
}

InterfaceIdSearchFilterImpl::InterfaceIdSearchFilterImpl(const IntfID& id)
    : intfId(id)
{
}

ErrCode InterfaceIdSearchFilterImpl::acceptsObject(IBaseObject* obj, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    return daqTry(
        [&]{
            const auto& componentPtr = ComponentPtr::Borrow(obj);
            *accepts = true;
            if (!componentPtr.supportsInterface(intfId))
                *accepts = false;

            return OPENDAQ_SUCCESS;
        });
}

ErrCode InterfaceIdSearchFilterImpl::visitChildren(IBaseObject* obj, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    *visit = true;
    return OPENDAQ_SUCCESS;
}

LocalIdSearchFilterImpl::LocalIdSearchFilterImpl(const StringPtr& localId)
    : localId(localId)
{
}

ErrCode LocalIdSearchFilterImpl::acceptsObject(IBaseObject* obj, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    return daqTry(
        [&]{
            const auto& componentPtr = ComponentPtr::Borrow(obj);
            *accepts = componentPtr.getLocalId() == localId ? True : False;

            return OPENDAQ_SUCCESS;
        });
}

ErrCode LocalIdSearchFilterImpl::visitChildren(IBaseObject* obj, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_RETURN_IF_FAILED(validateType(obj));

    *visit = true;
    return OPENDAQ_SUCCESS;
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createVisibleSearchFilter(ISearchFilter** objTmp)
{
    return createObject<ISearchFilter, VisibleSearchFilterImpl>(objTmp);
}

extern "C"
ErrCode PUBLIC_EXPORT createRequiredTagsSearchFilter(ISearchFilter** objTmp, IList* requiredTags)
{
    return createObject<ISearchFilter, RequiredTagsSearchFilterImpl, IList*>(objTmp, requiredTags);
}

extern "C"
ErrCode PUBLIC_EXPORT createExcludedTagsSearchFilter(ISearchFilter** objTmp, IList* excludedTags)
{
    return createObject<ISearchFilter, ExcludedTagsSearchFilterImpl, IList*>(objTmp, excludedTags);
}

extern "C"
ErrCode PUBLIC_EXPORT createInterfaceIdSearchFilter(ISearchFilter** objTmp, const IntfID& intfId)
{
    return createObject<ISearchFilter, InterfaceIdSearchFilterImpl, IntfID>(objTmp, intfId);
}

extern "C" ErrCode PUBLIC_EXPORT createLocalIdSearchFilter(ISearchFilter** objTmp, IString* localId)
{
    return createObject<ISearchFilter, LocalIdSearchFilterImpl>(objTmp, localId);
}

#endif

END_NAMESPACE_OPENDAQ
