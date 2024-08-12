#include <opendaq/property_wrapper_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

PropertyWrapperImpl::PropertyWrapperImpl(const PropertyPtr& property, std::optional<std::unordered_set<size_t>> allowedSelectionValues)
    : property(property)
    , allowedSelectionValues(allowedSelectionValues)
{
}

ErrCode PropertyWrapperImpl::getValueType(CoreType* type)
{
    return property->getValueType(type);
}

ErrCode PropertyWrapperImpl::getKeyType(CoreType* type)
{
    return property->getKeyType(type);
}

ErrCode PropertyWrapperImpl::getItemType(CoreType* type)
{
    return property->getItemType(type);
}

ErrCode PropertyWrapperImpl::getName(IString** name)
{
    return property->getName(name);
}

ErrCode PropertyWrapperImpl::getDescription(IString** description)
{
    return property->getDescription(description);
}

ErrCode PropertyWrapperImpl::getUnit(IUnit** unit)
{
    return property->getUnit(unit);
}

ErrCode PropertyWrapperImpl::getMinValue(INumber** min)
{
    return property->getMinValue(min);
}

ErrCode PropertyWrapperImpl::getMaxValue(INumber** max)
{
    return property->getMaxValue(max);
}

ErrCode PropertyWrapperImpl::getDefaultValue(IBaseObject** value)
{
    return property->getDefaultValue(value);
}

ErrCode PropertyWrapperImpl::getSuggestedValues(IList** values)
{
    return property->getSuggestedValues(values);
}

ErrCode PropertyWrapperImpl::getVisible(Bool* visible)
{
    return property->getVisible(visible);
}

ErrCode PropertyWrapperImpl::getReadOnly(Bool* readOnly)
{
    return property->getReadOnly(readOnly);
}

ErrCode PropertyWrapperImpl::getSelectionValues(IBaseObject** values)
{
    OPENDAQ_PARAM_NOT_NULL(values);

    if (!allowedSelectionValues.has_value())
        return property->getSelectionValues(values);

    BaseObjectPtr wrappedSelectionValues;
    const auto err = property->getSelectionValues(&wrappedSelectionValues);
    if (OPENDAQ_FAILED(err))
        return err;

    if (!wrappedSelectionValues.assigned())
    {
        *values = nullptr;
        return OPENDAQ_SUCCESS;
    }

    auto overridenSelectionValues = Dict<IInteger, IString>();
    if (wrappedSelectionValues.supportsInterface<IDict>())
    {
        const DictPtr<IInteger, IBaseObject> wrappedSelectionDict = wrappedSelectionValues;

        for (const auto& w : wrappedSelectionDict)
        {
            if (allowedSelectionValues.value().find(w.first) != allowedSelectionValues.value().end())
                overridenSelectionValues.set(w.first, w.second);
        }

        *values = overridenSelectionValues.detach();
        return OPENDAQ_SUCCESS;
    }

    if (wrappedSelectionValues.supportsInterface<IList>())
    {
        const ListPtr<IBaseObject> wrappedSelectionList = wrappedSelectionValues;

        for (size_t i = 0; i < wrappedSelectionList.getCount(); i++)
        {
            if (allowedSelectionValues.value().find(i) != allowedSelectionValues.value().end())
                overridenSelectionValues.set(i, wrappedSelectionList[i]);
        }

        *values = overridenSelectionValues.detach();
        return OPENDAQ_SUCCESS;
    }

    return makeErrorInfo(OPENDAQ_ERR_INVALIDPROPERTY, "Property selection list is of wrong type");
}

ErrCode PropertyWrapperImpl::getReferencedProperty(IProperty** property)
{
    return this->property->getReferencedProperty(property);
}

ErrCode PropertyWrapperImpl::getIsReferenced(Bool* isReferenced)
{
    return property->getIsReferenced(isReferenced);
}

ErrCode PropertyWrapperImpl::getValidator(IValidator** validator)
{
    return property->getValidator(validator);
}

ErrCode PropertyWrapperImpl::getCoercer(ICoercer** coercer)
{
    return property->getCoercer(coercer);
}

ErrCode PropertyWrapperImpl::getCallableInfo(ICallableInfo** callable)
{
    return property->getCallableInfo(callable);
}

ErrCode PropertyWrapperImpl::getStructType(IStructType** structType)
{
    return property->getStructType(structType);
}

ErrCode PropertyWrapperImpl::getOnPropertyValueWrite(IEvent** event)
{
    return property->getOnPropertyValueWrite(event);
}

ErrCode PropertyWrapperImpl::getOnPropertyValueRead(IEvent** event)
{
    return property->getOnPropertyValueRead(event);
}

ErrCode PropertyWrapperImpl::getValue(IBaseObject** value)
{
    return property->getValue(value);
}

ErrCode PropertyWrapperImpl::setValue(IBaseObject* value)
{
    return property->setValue(value);
}

END_NAMESPACE_OPENDAQ
