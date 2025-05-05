#include <coreobjects/property_filter_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

VisiblePropertyFilterImpl::VisiblePropertyFilterImpl() = default;

ErrCode VisiblePropertyFilterImpl::acceptsProperty(IProperty* property, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(property);
    
    return property->getVisible(accepts);
}

ReadOnlyPropertyFilterImpl::ReadOnlyPropertyFilterImpl() = default;

ErrCode ReadOnlyPropertyFilterImpl::acceptsProperty(IProperty* property, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(property);

    return property->getReadOnly(accepts);
}

TypePropertyFilterImpl::TypePropertyFilterImpl(const CoreType& type)
    : type(type)
{
}

ErrCode TypePropertyFilterImpl::acceptsProperty(IProperty* property, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(property);

    const auto& propertyPtr = PropertyPtr::Borrow(property);
    *accepts = propertyPtr.getValueType() == type;

    return OPENDAQ_SUCCESS;
}

NamePropertyFilterImpl::NamePropertyFilterImpl(const StringPtr& name)
    : name(name)
{
}

ErrCode INTERFACE_FUNC NamePropertyFilterImpl::acceptsProperty(IProperty* property, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(property);

    const auto& propertyPtr = PropertyPtr::Borrow(property);
    *accepts = propertyPtr.getName() == name ? True : False;

    return OPENDAQ_SUCCESS;
}

AnyPropertyFilterImpl::AnyPropertyFilterImpl() = default;

ErrCode AnyPropertyFilterImpl::acceptsProperty(IProperty* /*property*/, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);

    *accepts = true;
    return OPENDAQ_SUCCESS;
}

OrPropertyFilterImpl::OrPropertyFilterImpl(const PropertyFilterPtr& left, const PropertyFilterPtr& right)
    : left(left)
    , right(right)
{
}

ErrCode OrPropertyFilterImpl::acceptsProperty(IProperty* property, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(property);

    *accepts = left.acceptsProperty(property) || right.acceptsProperty(property);
    return OPENDAQ_SUCCESS;
}

AndPropertyFilterImpl::AndPropertyFilterImpl(const PropertyFilterPtr& left, const PropertyFilterPtr& right)
    : left(left)
    , right(right)
{
}

ErrCode AndPropertyFilterImpl::acceptsProperty(IProperty* property, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(property);
    
    *accepts = left.acceptsProperty(property) && right.acceptsProperty(property);
    return OPENDAQ_SUCCESS;
}

NotPropertyFilterImpl::NotPropertyFilterImpl(const PropertyFilterPtr& filter)
    : filter(filter)
{
}

ErrCode NotPropertyFilterImpl::acceptsProperty(IProperty* property, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(property);
    
    *accepts = !filter.acceptsProperty(property);
    return OPENDAQ_SUCCESS;
}

CustomPropertyFilterImpl::CustomPropertyFilterImpl(const FunctionPtr& acceptsFunction)
    : acceptsFunc(acceptsFunction)
{
}

ErrCode CustomPropertyFilterImpl::acceptsProperty(IProperty* property, Bool* accepts)
{
    OPENDAQ_PARAM_NOT_NULL(accepts);
    OPENDAQ_PARAM_NOT_NULL(property);

    return daqTry(
        [&]()
        {
            *accepts = acceptsFunc(property);
            return OPENDAQ_SUCCESS;
        });
}

//RecursivePropertyFilterImpl::RecursivePropertyFilterImpl(const PropertyFilterPtr& filter)
//    : filter(filter)
//{
//}

//ErrCode RecursivePropertyFilterImpl::acceptsProperty(IProperty* property, Bool* accepts)
//{
//    OPENDAQ_PARAM_NOT_NULL(accepts);
//    OPENDAQ_PARAM_NOT_NULL(property);
    
//    *accepts = filter.acceptsProperty(property);
//    return OPENDAQ_SUCCESS;
//}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createVisiblePropertyFilter(IPropertyFilter** objTmp)
{
    return createObject<IPropertyFilter, VisiblePropertyFilterImpl>(objTmp);
}

extern "C"
    ErrCode PUBLIC_EXPORT createReadOnlyPropertyFilter(IPropertyFilter** objTmp)
{
    return createObject<IPropertyFilter, ReadOnlyPropertyFilterImpl>(objTmp);
}

extern "C"
ErrCode PUBLIC_EXPORT createTypePropertyFilter(IPropertyFilter** objTmp, const CoreType& type)
{
    return createObject<IPropertyFilter, TypePropertyFilterImpl, CoreType>(objTmp, type);
}

extern "C" ErrCode PUBLIC_EXPORT createNamePropertyFilter(IPropertyFilter** objTmp, IString* name)
{
    return createObject<IPropertyFilter, NamePropertyFilterImpl>(objTmp, name);
}

extern "C" ErrCode PUBLIC_EXPORT createAnyPropertyFilter(IPropertyFilter** objTmp)
{
    return createObject<IPropertyFilter, AnyPropertyFilterImpl>(objTmp);
}

extern "C"
ErrCode PUBLIC_EXPORT createAndPropertyFilter(IPropertyFilter** objTmp, IPropertyFilter* left, IPropertyFilter* right)
{
    return createObject<IPropertyFilter, AndPropertyFilterImpl, IPropertyFilter*, IPropertyFilter*>(objTmp, left, right);
}

extern "C"
ErrCode PUBLIC_EXPORT createOrPropertyFilter(IPropertyFilter** objTmp, IPropertyFilter* left, IPropertyFilter* right)
{
    return createObject<IPropertyFilter, OrPropertyFilterImpl, IPropertyFilter*, IPropertyFilter*>(objTmp, left, right);
}

extern "C"
ErrCode PUBLIC_EXPORT createNotPropertyFilter(IPropertyFilter** objTmp, IPropertyFilter* filter)
{
    return createObject<IPropertyFilter, NotPropertyFilterImpl, IPropertyFilter*>(objTmp, filter);
}

extern "C"
ErrCode PUBLIC_EXPORT createCustomPropertyFilter(IPropertyFilter** objTmp, IFunction* acceptsFunction)
{
    return createObject<IPropertyFilter, CustomPropertyFilterImpl, IFunction*>(objTmp, acceptsFunction);
}

//extern "C"
//ErrCode PUBLIC_EXPORT createRecursivePropertyFilter(IPropertyFilter** objTmp, IPropertyFilter* filter)
//{
//    return createObject<IPropertyFilter, RecursivePropertyFilterImpl, IPropertyFilter*>(objTmp, filter);
//}

#endif

END_NAMESPACE_OPENDAQ
