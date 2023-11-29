#include <opendaq/search_filter_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

VisibleSearchFilterImpl::VisibleSearchFilterImpl() = default;

ErrCode VisibleSearchFilterImpl::acceptsComponent(IComponent* component, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    
    return component->getVisible(accepts);
}

ErrCode VisibleSearchFilterImpl::visitChildren(IComponent* component, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);
    OPENDAQ_PARAM_NOT_NULL(component);

    component->getVisible(visit);
    return OPENDAQ_SUCCESS;
}

RequiredTagsSearchFilterImpl::RequiredTagsSearchFilterImpl(const ListPtr<IString>& requiredTags)
{
    for (const auto& tag : requiredTags)
        this->requiredTags.insert(tag);
}

ErrCode RequiredTagsSearchFilterImpl::acceptsComponent(IComponent* component, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(component);

    TagsConfigPtr tags;
    const ErrCode err = component->getTags(&tags);
    if (OPENDAQ_FAILED(err))
        return err;

    *accepts = true;
    for (const auto tag : tags.getList())
    {
        if (!requiredTags.count(tag))
        {
            *accepts = false;
            break;
        }
    }

    return OPENDAQ_SUCCESS;
}

ErrCode RequiredTagsSearchFilterImpl::visitChildren(IComponent* component, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);

    *visit = true;
    return OPENDAQ_SUCCESS;
}

ExcludedTagsSearchFilterImpl::ExcludedTagsSearchFilterImpl(const ListPtr<IString>& excludedTags)
{
    for (const auto& tag : excludedTags)
        this->excludedTags.insert(tag);
}

ErrCode ExcludedTagsSearchFilterImpl::acceptsComponent(IComponent* component, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(component);

    TagsConfigPtr tags;
    const ErrCode err = component->getTags(&tags);
    if (OPENDAQ_FAILED(err))
        return err;

    *accepts = true;
    for (const auto tag : tags.getList())
    {
        if (excludedTags.count(tag))
        {
            *accepts = false;
            break;
        }
    }

    return OPENDAQ_SUCCESS;
}

ErrCode ExcludedTagsSearchFilterImpl::visitChildren(IComponent* component, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);

    *visit = true;
    return OPENDAQ_SUCCESS;
}

SearchIdSearchFilterImpl::SearchIdSearchFilterImpl(const IntfID id)
    : searchId(id)
{
}

ErrCode SearchIdSearchFilterImpl::acceptsComponent(IComponent* component, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(component);

    const auto& componentPtr = ComponentPtr::Borrow(component);
    *accepts = true;
    if (!componentPtr.supportsInterface(searchId))
        *accepts = false;

    return OPENDAQ_SUCCESS;
}

ErrCode SearchIdSearchFilterImpl::visitChildren(IComponent* component, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);

    *visit = true;
    return OPENDAQ_SUCCESS;
}

AnySearchFilterImpl::AnySearchFilterImpl() = default;

ErrCode AnySearchFilterImpl::acceptsComponent(IComponent* /*component*/, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);

    *accepts = true;
    return OPENDAQ_SUCCESS;
}

ErrCode AnySearchFilterImpl::visitChildren(IComponent* component, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);

    *visit = true;
    return OPENDAQ_SUCCESS;
}

OrSearchFilterImpl::OrSearchFilterImpl(const SearchFilterPtr& left, const SearchFilterPtr& right)
    : left(left)
    , right(right)
{
}

ErrCode OrSearchFilterImpl::acceptsComponent(IComponent* component, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(component);

    *accepts = left.acceptsComponent(component) || right.acceptsComponent(component);
    return OPENDAQ_SUCCESS;
}

ErrCode OrSearchFilterImpl::visitChildren(IComponent* component, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);

    *visit = left.visitChildren(component) || right.visitChildren(component);
    return OPENDAQ_SUCCESS;
}

AndSearchFilterImpl::AndSearchFilterImpl(const SearchFilterPtr& left, const SearchFilterPtr& right)
    : left(left)
    , right(right)
{
}

ErrCode AndSearchFilterImpl::acceptsComponent(IComponent* component, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(component);
    
    *accepts = left.acceptsComponent(component) && right.acceptsComponent(component);
    return OPENDAQ_SUCCESS;
}

ErrCode AndSearchFilterImpl::visitChildren(IComponent* component, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);

    *visit = left.visitChildren(component) && right.visitChildren(component);
    return OPENDAQ_SUCCESS;
}

NotSearchFilterImpl::NotSearchFilterImpl(const SearchFilterPtr& filter)
    : filter(filter)
{
}

ErrCode NotSearchFilterImpl::acceptsComponent(IComponent* component, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(component);
    
    *accepts = !filter.acceptsComponent(component);
    return OPENDAQ_SUCCESS;
}

ErrCode NotSearchFilterImpl::visitChildren(IComponent* component, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);

    *visit = true;
    return OPENDAQ_SUCCESS;
}

CustomSearchFilterImpl::CustomSearchFilterImpl(const FunctionPtr& acceptsFunction, const FunctionPtr& visitFunction)
    : acceptsFunc(acceptsFunction)
    , visitFunc(visitFunction)
{
}

ErrCode CustomSearchFilterImpl::acceptsComponent(IComponent* component, Bool* accepts)
{

    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(component);

    return daqTry(
        [&]()
        {
            *accepts = acceptsFunc(component);
            return OPENDAQ_SUCCESS;
        });
}

ErrCode CustomSearchFilterImpl::visitChildren(IComponent* component, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);
    OPENDAQ_PARAM_NOT_NULL(component);

    return daqTry(
        [&]()
        {
            *visit = visitFunc(component);
            return OPENDAQ_SUCCESS;
        });
}

RecursiveSearchFilterImpl::RecursiveSearchFilterImpl(const SearchFilterPtr& filter)
    : filter(filter)
{
}

ErrCode RecursiveSearchFilterImpl::acceptsComponent(IComponent* component, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(component);
    
    *accepts = filter.acceptsComponent(component);
    return OPENDAQ_SUCCESS;
}

ErrCode RecursiveSearchFilterImpl::visitChildren(IComponent* component, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);

    *visit = filter.visitChildren(component);
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
ErrCode PUBLIC_EXPORT createSearchIdSearchFilter(ISearchFilter** objTmp, IntfID searchId)
{
    return createObject<ISearchFilter, SearchIdSearchFilterImpl, IntfID>(objTmp, searchId);
}

extern "C"
ErrCode PUBLIC_EXPORT createAnySearchFilter(ISearchFilter** objTmp)
{
    return createObject<ISearchFilter, AnySearchFilterImpl>(objTmp);
}

extern "C"
ErrCode PUBLIC_EXPORT createAndSearchFilter(ISearchFilter** objTmp, ISearchFilter* left, ISearchFilter* right)
{
    return createObject<ISearchFilter, AndSearchFilterImpl, ISearchFilter*, ISearchFilter*>(objTmp, left, right);
}

extern "C"
ErrCode PUBLIC_EXPORT createOrSearchFilter(ISearchFilter** objTmp, ISearchFilter* left, ISearchFilter* right)
{
    return createObject<ISearchFilter, OrSearchFilterImpl, ISearchFilter*, ISearchFilter*>(objTmp, left, right);
}

extern "C"
ErrCode PUBLIC_EXPORT createNotSearchFilter(ISearchFilter** objTmp, ISearchFilter* filter)
{
    return createObject<ISearchFilter, NotSearchFilterImpl, ISearchFilter*>(objTmp, filter);
}

extern "C"
ErrCode PUBLIC_EXPORT createCustomSearchFilter(ISearchFilter** objTmp, IFunction* acceptsFunction, IFunction* visitFunction)
{
    return createObject<ISearchFilter, CustomSearchFilterImpl, IFunction*, IFunction*>(objTmp, acceptsFunction, visitFunction);
}

extern "C"
ErrCode PUBLIC_EXPORT createRecursiveSearchFilter(ISearchFilter** objTmp, ISearchFilter* filter)
{
    return createObject<ISearchFilter, RecursiveSearchFilterImpl, ISearchFilter*>(objTmp, filter);
}

#endif

END_NAMESPACE_OPENDAQ
