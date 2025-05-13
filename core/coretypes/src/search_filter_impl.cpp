#include <coretypes/search_filter_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

AnySearchFilterImpl::AnySearchFilterImpl() = default;

ErrCode AnySearchFilterImpl::acceptsObject(IBaseObject* /*obj*/, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);

    *accepts = true;
    return OPENDAQ_SUCCESS;
}

ErrCode AnySearchFilterImpl::visitChildren(IBaseObject* /*obj*/, Bool* visit)
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

ErrCode OrSearchFilterImpl::acceptsObject(IBaseObject* obj, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(obj);

    *accepts = left.acceptsObject(obj) || right.acceptsObject(obj);
    return OPENDAQ_SUCCESS;
}

ErrCode OrSearchFilterImpl::visitChildren(IBaseObject* obj, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);

    *visit = left.visitChildren(obj) || right.visitChildren(obj);
    return OPENDAQ_SUCCESS;
}

AndSearchFilterImpl::AndSearchFilterImpl(const SearchFilterPtr& left, const SearchFilterPtr& right)
    : left(left)
    , right(right)
{
}

ErrCode AndSearchFilterImpl::acceptsObject(IBaseObject* obj, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(obj);
    
    *accepts = left.acceptsObject(obj) && right.acceptsObject(obj);
    return OPENDAQ_SUCCESS;
}

ErrCode AndSearchFilterImpl::visitChildren(IBaseObject* obj, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);

    *visit = left.visitChildren(obj) && right.visitChildren(obj);
    return OPENDAQ_SUCCESS;
}

NotSearchFilterImpl::NotSearchFilterImpl(const SearchFilterPtr& filter)
    : filter(filter)
{
}

ErrCode NotSearchFilterImpl::acceptsObject(IBaseObject* obj, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(obj);
    
    *accepts = !filter.acceptsObject(obj);
    return OPENDAQ_SUCCESS;
}

ErrCode NotSearchFilterImpl::visitChildren(IBaseObject* /*obj*/, Bool* visit)
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

ErrCode CustomSearchFilterImpl::acceptsObject(IBaseObject* obj, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry(
        [&]()
        {
            auto objPtr = BaseObjectPtr::Borrow(obj);
            *accepts = acceptsFunc(objPtr);
            return OPENDAQ_SUCCESS;
        });
}

ErrCode CustomSearchFilterImpl::visitChildren(IBaseObject* obj, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);
    OPENDAQ_PARAM_NOT_NULL(obj);

    if (!visitFunc.assigned())
    {
        *visit = True;
        return OPENDAQ_SUCCESS;
    }

    return daqTry(
        [&]()
        {
            auto objPtr = BaseObjectPtr::Borrow(obj);
            *visit = visitFunc(objPtr);
            return OPENDAQ_SUCCESS;
        });
}

RecursiveSearchFilterImpl::RecursiveSearchFilterImpl(const SearchFilterPtr& filter)
    : filter(filter)
{
}

ErrCode RecursiveSearchFilterImpl::acceptsObject(IBaseObject* obj, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(obj);
    
    *accepts = filter.acceptsObject(obj);
    return OPENDAQ_SUCCESS;
}

ErrCode RecursiveSearchFilterImpl::visitChildren(IBaseObject* obj, Bool* visit)
{
    OPENDAQ_PARAM_NOT_NULL(visit);

    *visit = filter.visitChildren(obj);
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AnySearchFilter, ISearchFilter)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, AndSearchFilter, ISearchFilter, ISearchFilter*, left, ISearchFilter*, right)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, OrSearchFilter, ISearchFilter, ISearchFilter*, left, ISearchFilter*, right)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, NotSearchFilter, ISearchFilter, ISearchFilter*, filter)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, CustomSearchFilter, ISearchFilter, IFunction*, acceptsFunction, IFunction*, visitFunction)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, RecursiveSearchFilter, ISearchFilter, ISearchFilter*, filter)

END_NAMESPACE_OPENDAQ
