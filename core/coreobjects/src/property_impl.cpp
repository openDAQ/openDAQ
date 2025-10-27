#include <coreobjects/property_impl.h>

BEGIN_NAMESPACE_OPENDAQ

namespace details
{
    static CoreType intfIdToCoreType(IntfID intfID)
    {
        static const std::unordered_map<IntfID, CoreType> intfIdToCoreTypeMap = {
            {IBoolean::Id, ctBool},
            {IInteger::Id, ctInt},
            {IFloat::Id, ctFloat},
            {IString::Id, ctString},
            {IList::Id, ctList},
            {IDict::Id, ctDict},
            {IRatio::Id, ctRatio},
            {IProcedure::Id, ctProc},
            {IFunction::Id, ctFunc},
            {IBinaryData::Id, ctBinaryData},
            {IComplexNumber::Id, ctComplexNumber},
            {IPropertyObject::Id, ctObject}
        };

        if (auto it = intfIdToCoreTypeMap.find(intfID); it != intfIdToCoreTypeMap.end())
            return it->second;
        return ctUndefined;
    }
}

// Constructors

PropertyImpl::PropertyImpl()
    : owner(nullptr)
    , valueType(ctUndefined)
    , visible(true)
    , readOnly(false)
{
    propPtr = this->borrowPtr<PropertyPtr>();
}

PropertyImpl::PropertyImpl(const StringPtr& name)
    : PropertyImpl()
{
    this->name = name;
}

PropertyImpl::PropertyImpl(IPropertyBuilder* propertyBuilder)
{
    const auto propertyBuilderPtr = PropertyBuilderPtr::Borrow(propertyBuilder);
    this->valueType = propertyBuilderPtr.getValueType();
    this->name = propertyBuilderPtr.getName();
    this->description = propertyBuilderPtr.getDescription();
    this->unit = propertyBuilderPtr.getUnit();
    this->minValue = propertyBuilderPtr.getMinValue();
    this->maxValue = propertyBuilderPtr.getMaxValue();
    this->defaultValue = propertyBuilderPtr.getDefaultValue();
    this->visible = propertyBuilderPtr.getVisible();
    this->readOnly = propertyBuilderPtr.getReadOnly();
    this->selectionValues = propertyBuilderPtr.getSelectionValues();
    this->suggestedValues = propertyBuilderPtr.getSuggestedValues();
    this->refProp = propertyBuilderPtr.getReferencedProperty();
    this->coercer = propertyBuilderPtr.getCoercer();
    this->validator = propertyBuilderPtr.getValidator();
    this->callableInfo = propertyBuilderPtr.getCallableInfo();
    this->onValueWrite = (IEvent*) propertyBuilderPtr.getOnPropertyValueWrite();
    this->onValueRead = (IEvent*) propertyBuilderPtr.getOnPropertyValueRead();
    this->onSuggestedValuesRead = (IEvent*) propertyBuilderPtr.getOnSuggestedValuesRead();
    this->onSelectionValuesRead = (IEvent*) propertyBuilderPtr.getOnSelectionValuesRead();

    propPtr = this->borrowPtr<PropertyPtr>();
    owner = nullptr;

    checkErrorInfo(validateDuringConstruction());
}

PropertyImpl::PropertyImpl(const StringPtr& name, const BaseObjectPtr& defaultValue, const BooleanPtr& visible)
    : PropertyImpl(name)
{
    this->defaultValue = defaultValue;
    this->visible = visible;
}

// BoolProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, IBoolean* defaultValue, const BooleanPtr& visible)
    : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
{
    this->valueType = ctBool;

    checkErrorInfo(validateDuringConstruction());
}

// IntProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, IInteger* defaultValue, const BooleanPtr& visible)
    : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
{
    this->valueType = ctInt;

    checkErrorInfo(validateDuringConstruction());
}

// FloatProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, IFloat* defaultValue, const BooleanPtr& visible)
    : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
{
    this->valueType = ctFloat;

    checkErrorInfo(validateDuringConstruction());
}

// StringProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, IString* defaultValue, const BooleanPtr& visible)
    : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
{
    this->valueType = ctString;

    checkErrorInfo(validateDuringConstruction());
}

// ListProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, IList* defaultValue, const BooleanPtr& visible)
    : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
{
    this->valueType = ctList;

    checkErrorInfo(validateDuringConstruction());
}

// DictProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, IDict* defaultValue, const BooleanPtr& visible)
    : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
{
    this->valueType = ctDict;

    checkErrorInfo(validateDuringConstruction());
}

// RatioProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, IRatio* defaultValue, const BooleanPtr& visible)
    : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
{
    this->valueType = ctRatio;

    checkErrorInfo(validateDuringConstruction());
}

// ObjectProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, IPropertyObject* defaultValue)
    : PropertyImpl(name, BaseObjectPtr(defaultValue), true)
{
    this->valueType = ctObject;

    if (defaultValue == nullptr)
        this->defaultValue = PropertyObject().detach();

    checkErrorInfo(validateDuringConstruction());
}

// FunctionProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, ICallableInfo* callableInfo, const BooleanPtr& visible)
    : PropertyImpl(name)
{
    this->visible = visible;
    this->callableInfo = callableInfo;

    CoreType returnType;
    callableInfo->getReturnType(&returnType);
    if (returnType == ctUndefined)
    {
        this->valueType = ctProc;
    }
    else
    {
        this->valueType = ctFunc;
    }

    checkErrorInfo(validateDuringConstruction());
}

// ReferenceProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, IEvalValue* referencedProperty)
    : PropertyImpl(name)
{
    this->refProp = referencedProperty;

    checkErrorInfo(validateDuringConstruction());
}

// SelectionProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, IList* selectionValues, IInteger* defaultValue, const BooleanPtr& visible)
    : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
{
    this->valueType = ctInt;
    this->selectionValues = BaseObjectPtr(selectionValues);

    checkErrorInfo(validateDuringConstruction());
}

// SparseSelectionProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, IDict* selectionValues, IInteger* defaultValue, const BooleanPtr& visible)
    : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
{
    this->valueType = ctInt;
    this->selectionValues = BaseObjectPtr(selectionValues);

    checkErrorInfo(validateDuringConstruction());
}

// StructProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, IStruct* defaultValue, const BooleanPtr& visible)
    : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
{
    this->valueType = ctStruct;
    this->selectionValues = BaseObjectPtr(selectionValues);

   checkErrorInfo(validateDuringConstruction());
}

// EnumerationProperty()
PropertyImpl::PropertyImpl(const StringPtr& name, IEnumeration* defaultValue, const BooleanPtr& visible)
    : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
{
    this->valueType = ctEnumeration;
    this->selectionValues = BaseObjectPtr(selectionValues);

    checkErrorInfo(validateDuringConstruction());
}

ErrCode PropertyImpl::getValueType(CoreType* type)
{
    return getValueTypeInternal(type, true);
}

ErrCode PropertyImpl::getValueTypeNoLock(CoreType* type)
{
    return getValueTypeInternal(type, false);
}

ErrCode PropertyImpl::getValueTypeInternal(CoreType* type, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    const ErrCode errCode = daqTry([&]()
    {
        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
            *type = lock ? prop.getValueType() : prop.asPtr<IPropertyInternal>().getValueTypeNoLock();
        else
            *type = this->valueType;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode, "Failed to get value type of property");
    return errCode;
}

ErrCode PropertyImpl::getKeyType(CoreType* type)
{
    return getKeyTypeInternal(type, true);    
}

ErrCode PropertyImpl::getKeyTypeNoLock(CoreType* type)
{
    return getKeyTypeInternal(type, false);    
}

ErrCode PropertyImpl::getKeyTypeInternal(CoreType* type, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = ctUndefined;
    BaseObjectPtr defVal;
    auto err = this->getDefaultValueInternal(&defVal, lock);
    OPENDAQ_RETURN_IF_FAILED(err);

    if (!defVal.assigned())
        return OPENDAQ_SUCCESS;

    const auto value = defVal.asPtrOrNull<IDict>();
    if (!value.assigned())
        return OPENDAQ_SUCCESS;

    IntfID intfID;
    err = value.asPtr<IDictElementType>()->getKeyInterfaceId(&intfID);
    OPENDAQ_RETURN_IF_FAILED(err);

    auto coreType = details::intfIdToCoreType(intfID);

    // TODO: Workaround if item type of dict/list is undefined
    if (coreType == ctUndefined && value.getCount() > 0)
        coreType = value.getKeyList()[0].getCoreType();

    *type = coreType;
    return OPENDAQ_SUCCESS;
}
    
ErrCode PropertyImpl::getItemType(CoreType* type)
{
    return getItemTypeInternal(type, true);    
}

ErrCode PropertyImpl::getItemTypeNoLock(CoreType* type)
{
    return getItemTypeInternal(type, false);    
}

ErrCode PropertyImpl::getItemTypeInternal(CoreType* type, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    const ErrCode errCode = daqTry([&]()
    {
        IntfID intfID = IUnknown::Id;
        *type = ctUndefined;

        BaseObjectPtr defVal;
        auto err = this->getDefaultValueInternal(&defVal, lock);
        OPENDAQ_RETURN_IF_FAILED(err);

        BaseObjectPtr selVal;
        err = getSelectionValuesInternal(&selVal, lock);
        OPENDAQ_RETURN_IF_FAILED(err);

        BaseObjectPtr value = defVal;
        value = selVal.assigned() ? selVal : value;
        if (!value.assigned())
            return err;

        if (const auto dictElementType = value.asPtrOrNull<IDictElementType>(true); dictElementType.assigned())
        {
            err = dictElementType->getValueInterfaceId(&intfID);
            OPENDAQ_RETURN_IF_FAILED(err);
        }
        else if (const auto listElementType = value.asPtrOrNull<IListElementType>(true); listElementType.assigned())
        {
            err = listElementType->getElementInterfaceId(&intfID);
            OPENDAQ_RETURN_IF_FAILED(err);
        }
        auto coreType = details::intfIdToCoreType(intfID);

        // TODO: Workaround if item type of dict/list is undefined
        if (coreType == ctUndefined)
        {
            if (const auto asList = value.asPtrOrNull<IList>(); asList.assigned() && asList.getCount() > 0)
            {
                coreType = asList[0].getCoreType();
                err = OPENDAQ_SUCCESS;
            }
            else if (const auto asDict = value.asPtrOrNull<IDict>();asDict.assigned() && asDict.getCount() > 0)
            {
                coreType = asDict.getValueList()[0].getCoreType();
                err = OPENDAQ_SUCCESS;
            }
        }

        *type = coreType;
        return err;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode, "Failed to get item type of property");
    return errCode;
}

ErrCode PropertyImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    
    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::getDescription(IString** description)
{
    return getDescriptionInternal(description, true);
}

ErrCode PropertyImpl::getDescriptionNoLock(IString** description)
{
    return getDescriptionInternal(description, false);
}

ErrCode PropertyImpl::getDescriptionInternal(IString** description, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    const ErrCode errCode = daqTry([&]()
    {
        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
            *description = lock ? prop.getDescription().detach() : prop.asPtr<IPropertyInternal>().getDescriptionNoLock().detach();
        else
            *description = bindAndGet<StringPtr>(this->description, lock).detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getUnit(IUnit** unit)
{
    return getUnitInternal(unit, true);
}

ErrCode PropertyImpl::getUnitNoLock(IUnit** unit)
{
    return getUnitInternal(unit, false);
}

ErrCode PropertyImpl::getUnitInternal(IUnit** unit, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(unit);

    const ErrCode errCode = daqTry([&]()
    {
        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
            *unit = lock ? prop.getUnit().detach() : prop.asPtr<IPropertyInternal>().getUnitNoLock().detach();
        else
            *unit = bindAndGet<UnitPtr>(this->unit, lock).detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getMinValue(INumber** min)
{
    return getMinValueInternal(min, true);
}

ErrCode PropertyImpl::getMinValueNoLock(INumber** min)
{
    return getMinValueInternal(min, false);
}

ErrCode PropertyImpl::getMinValueInternal(INumber** min, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(min);

    const ErrCode errCode = daqTry([&]()
    {
        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
            *min = lock ? prop.getMinValue().detach() : prop.asPtr<IPropertyInternal>().getMinValueNoLock().detach();
        else
            *min = bindAndGet<NumberPtr>(this->minValue, lock).detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getMaxValue(INumber** max)
{
    return getMaxValueInternal(max, true);
}

ErrCode PropertyImpl::getMaxValueNoLock(INumber** max)
{
    return getMaxValueInternal(max, false);
}

ErrCode PropertyImpl::getMaxValueInternal(INumber** max, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(max);

    const ErrCode errCode = daqTry([&]()
    {
        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
            *max = lock ? prop.getMaxValue().detach() : prop.asPtr<IPropertyInternal>().getMaxValueNoLock().detach();
        else
            *max = bindAndGet<NumberPtr>(this->maxValue, lock).detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getDefaultValue(IBaseObject** value)
{
    return getDefaultValueInternal(value, true);
}

ErrCode PropertyImpl::getDefaultValueNoLock(IBaseObject** value)
{
    return getDefaultValueInternal(value, false);
}

ErrCode PropertyImpl::getDefaultValueInternal(IBaseObject** value, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    const ErrCode errCode = daqTry([&]()
    {
        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
            *value = lock ? prop.getDefaultValue().detach() : prop.asPtr<IPropertyInternal>().getDefaultValueNoLock().detach();
        else
            *value = bindAndGet<BaseObjectPtr>(this->defaultValue, lock).detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}
    
ErrCode PropertyImpl::getSuggestedValues(IList** values)
{
    return getSuggestedValuesInternal(values, true);
}

ErrCode PropertyImpl::getSuggestedValuesNoLock(IList** values)
{
    return getSuggestedValuesInternal(values, false);
}

ErrCode PropertyImpl::getSuggestedValuesInternal(IList** values, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(values);

    ErrCode err = daqTry([&]()
        {
            if (onSuggestedValuesRead.hasListeners())
            {
                // TODO: Should this lock !? If yes, what mutex !?
                auto args = PropertyMetadataReadArgs(propPtr);
                args.setValue(this->suggestedValues);
                onSuggestedValuesRead(propPtr, args);

                ListPtr<IBaseObject> selectionValuesPtr = args.getValue();
                *values = selectionValuesPtr.detach();
            }
            else if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
                *values = lock ? prop.getSuggestedValues().detach() : prop.asPtr<IPropertyInternal>().getSuggestedValuesNoLock().detach();
            else
                *values = bindAndGet<ListPtr<IBaseObject>>(this->suggestedValues, lock).detach();
                
            return OPENDAQ_SUCCESS;
        });
        OPENDAQ_RETURN_IF_FAILED(err);
    return err;
}
        
ErrCode PropertyImpl::getVisible(Bool* visible)
{
    return getVisibleInternal(visible, true);
}

ErrCode PropertyImpl::getVisibleNoLock(Bool* visible)
{
    return getVisibleInternal(visible, false);
}

ErrCode PropertyImpl::getVisibleInternal(Bool* visible, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(visible);

    const ErrCode errCode = daqTry([&]()
    {
        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
            *visible = lock ? prop.getVisible() : prop.asPtr<IPropertyInternal>().getVisibleNoLock();
        else
            *visible = bindAndGet<BooleanPtr>(this->visible, lock);
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getReadOnly(Bool* readOnly)
{
    return getReadOnlyInternal(readOnly, true);
}

ErrCode PropertyImpl::getReadOnlyNoLock(Bool* readOnly)
{
    return getReadOnlyInternal(readOnly, false);
}

ErrCode PropertyImpl::getReadOnlyInternal(Bool* readOnly, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(readOnly);

    const ErrCode errCode = daqTry([&]()
    {
        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
            *readOnly = lock ? prop.getReadOnly() : prop.asPtr<IPropertyInternal>().getReadOnlyNoLock();
        else
            *readOnly = bindAndGet<BooleanPtr>(this->readOnly, lock);
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getSelectionValues(IBaseObject** values)
{
    return getSelectionValuesInternal(values, true);
}

ErrCode PropertyImpl::getSelectionValuesNoLock(IBaseObject** values)
{
    return getSelectionValuesInternal(values, false);
}

ErrCode PropertyImpl::getSelectionValuesInternal(IBaseObject** values, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(values);

    ErrCode errCode = daqTry([&]()
        {
            if (onSelectionValuesRead.hasListeners())
            {
                // TODO: Should this lock !? If yes, what mutex !?
                auto args = PropertyMetadataReadArgs(propPtr);
                args.setValue(this->selectionValues);
                onSelectionValuesRead(propPtr, args);

                *values = args.getValue().detach();
            }
            else if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
                *values = lock ? prop.getSelectionValues().detach() : prop.asPtr<IPropertyInternal>().getSelectionValuesNoLock().detach();
            else
                *values = bindAndGet<BaseObjectPtr>(this->selectionValues, lock).detach();
                
            return OPENDAQ_SUCCESS;
        });

    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getReferencedProperty(IProperty** property)
{
    return getReferencedPropertyInternal(property, true);
}

ErrCode PropertyImpl::getReferencedPropertyNoLock(IProperty** property)
{
    return getReferencedPropertyInternal(property, false);
}

ErrCode PropertyImpl::getReferencedPropertyInternal(IProperty** property, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(property);

    const ErrCode errCode = daqTry([&]()
    {
        *property = bindAndGet<PropertyPtr>(this->refProp, lock).detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getIsReferenced(Bool* isReferenced)
{
    return getIsReferencedInternal(isReferenced, true);
}

ErrCode PropertyImpl::getIsReferencedNoLock(Bool* isReferenced)
{
    return getIsReferencedInternal(isReferenced, false);
}

ErrCode PropertyImpl::getIsReferencedInternal(Bool* isReferenced, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(isReferenced);

    const ErrCode errCode = daqTry([&]()
    {
        *isReferenced = false;
        if (const auto ownerPtr = getOwner(); ownerPtr.assigned())
        {
            const auto ownerInternal = ownerPtr.asPtr<IPropertyObjectInternal>();
            *isReferenced = lock ? ownerInternal.checkForReferences(propPtr) : ownerInternal.checkForReferencesNoLock(propPtr);
        }
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}
    
ErrCode PropertyImpl::getValidator(IValidator** validator)
{
    return getValidatorInternal(validator, true);
}

ErrCode PropertyImpl::getValidatorNoLock(IValidator** validator)
{
    return getValidatorInternal(validator, false);
}

ErrCode PropertyImpl::getValidatorInternal(IValidator** validator, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(validator);

    const ErrCode errCode = daqTry([&]()
    {
        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
            *validator = lock ? prop.getValidator().detach() : prop.asPtr<IPropertyInternal>().getValidatorNoLock().detach();
        else
            *validator = this->validator.addRefAndReturn();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}
        
ErrCode PropertyImpl::getCoercer(ICoercer** coercer)
{
    return getCoercerInternal(coercer, true);
}

ErrCode PropertyImpl::getCoercerNoLock(ICoercer** coercer)
{
    return getCoercerInternal(coercer, false);
}

ErrCode PropertyImpl::getCoercerInternal(ICoercer** coercer, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(coercer);

    const ErrCode errCode = daqTry([&]()
    {
        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
            *coercer = lock ? prop.getCoercer().detach() : prop.asPtr<IPropertyInternal>().getCoercerNoLock().detach();
        else
            *coercer = this->coercer.addRefAndReturn();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}
        
ErrCode PropertyImpl::getCallableInfo(ICallableInfo** callableInfo)
{
    return getCallableInfoInternal(callableInfo, true);
}

ErrCode PropertyImpl::getCallableInfoNoLock(ICallableInfo** callableInfo)
{
    return getCallableInfoInternal(callableInfo, false);
}

ErrCode PropertyImpl::getCallableInfoInternal(ICallableInfo** callableInfo, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(callableInfo);

    const ErrCode errCode = daqTry([&]()
    {
        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
            *callableInfo = lock ? prop.getCallableInfo().detach() : prop.asPtr<IPropertyInternal>().getCallableInfoNoLock().detach();
        else
            *callableInfo = this->callableInfo.addRefAndReturn();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getStructType(IStructType** structType)
{
    return getStructTypeInternal(structType, true);
}

ErrCode PropertyImpl::getStructTypeNoLock(IStructType** structType)
{
    return getStructTypeInternal(structType, false);
}

ErrCode PropertyImpl::getStructTypeInternal(IStructType** structType, bool lock)
{
    OPENDAQ_PARAM_NOT_NULL(structType);

    const ErrCode errCode = daqTry([&]()
    {
        BaseObjectPtr defaultStruct;
        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
        {
            defaultStruct = lock ? prop.getDefaultValue().detach() : prop.asPtr<IPropertyInternal>().getDefaultValueNoLock().detach();
        }
        else
        {
            const ErrCode errCode = this->getDefaultValueInternal(&defaultStruct, lock);
            OPENDAQ_RETURN_IF_FAILED(errCode);
        }
        *structType = defaultStruct.asPtr<IStruct>().getStructType().detach();
        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::overrideDefaultValue(IBaseObject* newDefaultValue)
{
    defaultValue = newDefaultValue;
    if (defaultValue.supportsInterface<IFreezable>())
        defaultValue.freeze();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::getClassOnPropertyValueWrite(IEvent** event)
{
    if (event == nullptr)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return the event via a null pointer.");
    }

    *event = onValueWrite.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::getOnPropertyValueWrite(IEvent** event)
{
    if (event == nullptr)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return the event via a null pointer.");
    }
    if (const auto ownerPtr = getOwner(); ownerPtr.assigned())
    {
        return ownerPtr->getOnPropertyValueWrite(this->name, event);
    }

    *event = onValueWrite.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::getClassOnPropertyValueRead(IEvent** event)
{
    if (event == nullptr)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return the event via a null pointer.");
    }

    *event = onValueRead.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::getOnPropertyValueRead(IEvent** event)
{
    if (event == nullptr)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return the event via a null pointer.");
    }

    if (const auto ownerPtr = getOwner(); ownerPtr.assigned())
    {
        return ownerPtr->getOnPropertyValueRead(this->name, event);
    }

    *event = onValueRead.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::getOnSelectionValuesRead(IEvent** event)
{
    if (event == nullptr)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return the event via a null pointer.");
    }

    *event = onSelectionValuesRead.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::getOnSuggestedValuesRead(IEvent** event)
{
    if (event == nullptr)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return the event via a null pointer.");
    }

    *event = onSuggestedValuesRead.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::getValue(IBaseObject** value)
{
    if (const auto ownerPtr = getOwner(); ownerPtr.assigned())
        return ownerPtr->getPropertyValue(this->name, value);
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NO_OWNER);
}

ErrCode PropertyImpl::setValue(IBaseObject* value)
{
    if (const auto ownerPtr = getOwner(); ownerPtr.assigned())
        return ownerPtr->setPropertyValue(this->name, value);
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NO_OWNER);
}

ErrCode PropertyImpl::setValueProtected(IBaseObject* newValue)
{
    if (const auto ownerPtr = getOwner(); ownerPtr.assigned())
        return ownerPtr.asPtr<IPropertyObjectProtected>()->setProtectedPropertyValue(this->name, newValue);
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NO_OWNER);
}

ErrCode PropertyImpl::getHasOnReadListeners(Bool* hasListeners)
{
    OPENDAQ_PARAM_NOT_NULL(hasListeners);

    auto ownerPtr = getOwner();
    if (ownerPtr.assigned())
    {
        EventPtr event;
        ErrCode err = ownerPtr->getOnPropertyValueRead(name, &event);
        if (OPENDAQ_FAILED(err))
        {
            *hasListeners = false;
            daqClearErrorInfo();
            return OPENDAQ_SUCCESS;
        }

        *hasListeners = event.hasListeners();
    }
    else
    {
        *hasListeners = onValueRead.hasListeners();
    }

    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::getHasOnGetSuggestedValuesListeners(Bool* hasListeners)
{
    OPENDAQ_PARAM_NOT_NULL(hasListeners);

    *hasListeners = onSuggestedValuesRead.hasListeners();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::getHasOnGetSelectionValuesListeners(Bool* hasListeners)
{
    OPENDAQ_PARAM_NOT_NULL(hasListeners);

    *hasListeners = onSelectionValuesRead.hasListeners();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::validate()
{
    if (!name.assigned() || name == "opendaq_unassigned")
    {
        name = "opendaq_unassigned";
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Property name is not assigned");
    }

    if (valueType == ctFunc || valueType == ctProc)
    {
        if (defaultValue.assigned())
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE,
                                        fmt::format(R"(Function/procedure property "{}" cannot have a default value)", name));
        }
    }
    else if (refProp.assigned())
    {
        if (defaultValue.assigned())
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE,
                                        fmt::format(R"(Reference property {} cannot have default values)", name));
        }
    }
    else if (!defaultValue.assigned())
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, fmt::format(R"(Property {} is missing its default value)", name));
    }

    if (defaultValue.assigned() && valueType != ctObject)
    {
        if (const auto freezable = defaultValue.asPtrOrNull<IFreezable>(); freezable.assigned())
        {
            const ErrCode err = freezable->freeze();
            OPENDAQ_RETURN_IF_FAILED(err);
        }
    }

    if (valueType == ctObject)
    {
        bool valid = !selectionValues.assigned() && !suggestedValues.assigned();
        valid = valid && !coercer.assigned() && !validator.assigned();
        valid = valid && !unit.assigned();

        if (!valid)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE,
                                    fmt::format(R"(Object-type property {} can only have its name, description, read-only, visible, and default value configured)", name));
    }

    if (minValue.assigned() || maxValue.assigned())
    {
        if (valueType != ctInt && valueType != ctFloat)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE,
                                        fmt::format(R"({}: Min/max can only be configured on Int, Float, and Ratio properties)", name));
    }

    if (callableInfo.assigned())
    {
        if (!(valueType == ctProc || valueType == ctFunc))
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE,
                                        fmt::format(R"({}: Callable info can be configured only on function- and procedure-type
                                        properties.)", name));
    }

    if (refProp.assigned())
    {
        bool valid = valueType == ctUndefined;
        valid = (valid && !description.assigned()) || description == "";
        valid = valid && !readOnly;
        valid = valid && !selectionValues.assigned() && !suggestedValues.assigned();
        valid = valid && !coercer.assigned() && !validator.assigned();

        if (!valid)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, fmt::format(R"(Reference property {} has invalid metadata.)", name));
    }

    if (selectionValues.assigned())
    {
        bool valid = valueType == ctInt;
        valid = valid && (selectionValues.supportsInterface<IList>() || selectionValues.supportsInterface<IDict>());
        if (!valid)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE,
                                    fmt::format(R"(Selection property {} must have the value type ctInt, and the selection values must be a list or dictionary)",
                                        name));
    }

    if (suggestedValues.assigned() && (valueType != ctInt && valueType != ctFloat && valueType != ctString))
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE,
                                    fmt::format(R"({}: Only numerical and string properties can have a list of suggested values)", name));
    }

    if (valueType == ctList || valueType == ctDict)
    {
        CoreType itemType = ctUndefined;
        CoreType keyType = ctUndefined;

        this->getItemType(&itemType);
        if (valueType == ctDict)
        {
            this->getKeyType(&keyType);
        }

        if (itemType == ctObject || keyType == ctObject)
            return DAQ_MAKE_ERROR_INFO(
                OPENDAQ_ERR_INVALIDSTATE,
                fmt::format(R"(Container type property {} cannot have keys/items that are object-types.)", name));

        if (itemType == ctFunc || keyType == ctFunc || itemType == ctProc || keyType == ctProc)
            return DAQ_MAKE_ERROR_INFO(
                OPENDAQ_ERR_INVALIDSTATE,
                fmt::format(R"(Container type property {} cannot have keys/items that are function-types.)", name));

        if (itemType == ctList || keyType == ctList || itemType == ctDict || keyType == ctDict)
            return DAQ_MAKE_ERROR_INFO(
                OPENDAQ_ERR_INVALIDSTATE,
                fmt::format(R"(Container type property {} cannot have keys/items that are container-types.)", name));
    }

    if (valueType == ctStruct)
    {
        bool valid = !selectionValues.assigned() && !suggestedValues.assigned();
        valid = valid && !coercer.assigned() && !validator.assigned();
        valid = valid && !maxValue.assigned() && !minValue.assigned();
        valid = valid && !unit.assigned() && !callableInfo.assigned();

        if (!valid)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, fmt::format(R"(Structure property {} has invalid metadata.)", name));
    }

    if (valueType == ctEnumeration)
    {
        bool valid = !selectionValues.assigned() && !suggestedValues.assigned();
        valid = valid && !coercer.assigned() && !validator.assigned();
        valid = valid && !maxValue.assigned() && !minValue.assigned();
        valid = valid && !unit.assigned() && !callableInfo.assigned();

        if (!valid)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, fmt::format(R"(Enumeration property {} has invalid metadata.)", name));
    }

    // TODO: Make callable info serializable
    // if ((valueType == ctProc || valueType == ctFunc) && !callableInfo.assigned())
    //{
    //    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, fmt::format(R"(Function- and procedure-type property {} must have
    //    Callable info configured)", name));
    //}

    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::toString(CharPtr* str)
{
    if (str == nullptr)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Parameter must not be null");
    }

    std::ostringstream stream;
    stream << "Property {" << name << "}";
    return daqDuplicateCharPtr(stream.str().c_str(), str);
}

//
// ISerializable
//

ErrCode PropertyImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    {
        SERIALIZE_PROP_PTR(name)
        SERIALIZE_PROP_PTR(description)

        serializer->key("valueType");
        serializer->writeInt(this->valueType);

        SERIALIZE_PROP_PTR(unit)
        SERIALIZE_PROP_PTR(minValue)
        SERIALIZE_PROP_PTR(maxValue)
        SERIALIZE_PROP_PTR(defaultValue)
        SERIALIZE_PROP_PTR(readOnly)
        SERIALIZE_PROP_PTR(visible)
        SERIALIZE_PROP_PTR(refProp)

        Bool hasSelectionValuesListeners = false;
        OPENDAQ_RETURN_IF_FAILED(getHasOnGetSelectionValuesListeners(&hasSelectionValuesListeners));

        if (hasSelectionValuesListeners)
        {
            serializer->key("HasSelectionValuesListeners");
            serializer->writeBool(true);
        }
        else
        {
            SERIALIZE_PROP_PTR(selectionValues)
        }

        SERIALIZE_PROP_PTR(coercer)
        SERIALIZE_PROP_PTR(validator)
            
        Bool hasSuggestedValuesListeners = false;
        OPENDAQ_RETURN_IF_FAILED(getHasOnGetSuggestedValuesListeners(&hasSuggestedValuesListeners));

        if (hasSuggestedValuesListeners)
        {
            serializer->key("HasSuggestedValuesListeners");
            serializer->writeBool(true);
        }
        else
        {
            SERIALIZE_PROP_PTR(suggestedValues)
        }

        SERIALIZE_PROP_PTR(callableInfo)
            
        Bool hasOnReadListeners = false;
        OPENDAQ_RETURN_IF_FAILED(getHasOnReadListeners(&hasOnReadListeners));

        if (hasOnReadListeners)
        {
            serializer->key("HasOnReadListeners");
            serializer->writeBool(true);
        }
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr PropertyImpl::SerializeId()
{
    return "Property";
}

ErrCode PropertyImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::ReadBuilderDeserializeValues(const PropertyBuilderPtr& builder, ISerializedObject* serializedObj, IBaseObject* context, IFunction* factoryCallback)
{
    auto propObj = builder;
    ErrCode errCode = deserializeMember<decltype(valueType)>(serializedObj, "valueType", builder, context, factoryCallback, &IPropertyBuilder::setValueType);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    DESERIALIZE_MEMBER(context, factoryCallback, description, setDescription)

    BaseObjectPtr unit;
    errCode = serializedObj->readObject(String("unit"), context, factoryCallback, &unit);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);
    if (errCode != OPENDAQ_ERR_NOTFOUND)
        builder->setUnit(unit.asPtr<IUnit>());

    DESERIALIZE_MEMBER(context, factoryCallback, defaultValue, setDefaultValue)

    BaseObjectPtr refProp;
    errCode = serializedObj->readObject(String("refProp"), context, factoryCallback, &refProp);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);
    if (errCode != OPENDAQ_ERR_NOTFOUND)
        propObj->setReferencedProperty(refProp.asPtr<IEvalValue>());

    DESERIALIZE_MEMBER(context, factoryCallback, selectionValues, setSelectionValues)

    BaseObjectPtr suggestedValues;
    errCode = serializedObj->readObject(String("suggestedValues"), context, factoryCallback, &suggestedValues);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);
    if (errCode != OPENDAQ_ERR_NOTFOUND)
        propObj->setSuggestedValues(suggestedValues.asPtr<IList>());

    BaseObjectPtr visible;
    errCode = serializedObj->readObject(String("visible"), context, factoryCallback, &visible);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);
    if (errCode != OPENDAQ_ERR_NOTFOUND)
        propObj->setVisible(visible.asPtr<IBoolean>());

    BaseObjectPtr readOnly;
    errCode = serializedObj->readObject(String("readOnly"), context, factoryCallback, &readOnly);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);
    if (errCode != OPENDAQ_ERR_NOTFOUND)
        propObj->setReadOnly(readOnly.asPtr<IBoolean>());

    BaseObjectPtr minValue;
    errCode = serializedObj->readObject(String("minValue"), context, factoryCallback, &minValue);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);
    if (errCode != OPENDAQ_ERR_NOTFOUND)
        propObj->setMinValue(minValue.asPtr<INumber>());

    BaseObjectPtr maxValue;
    errCode = serializedObj->readObject(String("maxValue"), context, factoryCallback, &maxValue);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);
    if (errCode != OPENDAQ_ERR_NOTFOUND)
        propObj->setMaxValue(maxValue.asPtr<INumber>());

    BaseObjectPtr coercer;
    errCode = serializedObj->readObject(String("coercer"), context, factoryCallback, &coercer);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);
    if (errCode != OPENDAQ_ERR_NOTFOUND)
        propObj->setCoercer(coercer.asPtr<ICoercer>());

    BaseObjectPtr validator;
    errCode = serializedObj->readObject(String("validator"), context, factoryCallback, &validator);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);
    if (errCode != OPENDAQ_ERR_NOTFOUND)
        propObj->setValidator(validator.asPtr<IValidator>());

    BaseObjectPtr callableInfo;
    errCode = serializedObj->readObject(String("callableInfo"), context, factoryCallback, &callableInfo);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);
    if (errCode != OPENDAQ_ERR_NOTFOUND)
        propObj->setCallableInfo(callableInfo.asPtr<ICallableInfo>());

    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::Deserialize(ISerializedObject* serializedObj, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    StringPtr name;
    ErrCode errCode = serializedObj->readString(String("name"), &name);
    OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);

    const auto propObj = PropertyBuilder(name);
    OPENDAQ_RETURN_IF_FAILED(ReadBuilderDeserializeValues(propObj, serializedObj, context, factoryCallback));

    *obj = propObj.build().detach();
    return OPENDAQ_SUCCESS;
}

//
// IPropertyInternal
//

ErrCode PropertyImpl::clone(IProperty** clonedProperty)
{
    OPENDAQ_PARAM_NOT_NULL(clonedProperty);

    PropertyBuilderPtr builder;
    OPENDAQ_RETURN_IF_FAILED(this->getBuilderFromThis(&builder));

    OPENDAQ_RETURN_IF_FAILED(builder->build(clonedProperty));
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::cloneWithOwner(IPropertyObject* owner, IProperty** clonedProperty)
{
    OPENDAQ_PARAM_NOT_NULL(clonedProperty);
    
    if (const auto ownerPtr = getOwner(); ownerPtr.assigned() && owner == ownerPtr)
    {
        this->addRef();
        *clonedProperty = this;
        return OPENDAQ_SUCCESS;
    }

    PropertyPtr prop;
    ErrCode err = clone(&prop);
    OPENDAQ_RETURN_IF_FAILED(err);

    if (auto ownableProp = prop.asPtrOrNull<IOwnable>(true); ownableProp.assigned())
        OPENDAQ_RETURN_IF_FAILED(ownableProp->setOwner(owner));
    else 
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOINTERFACE);

    *clonedProperty = prop.detach();
    return err;
}

ErrCode PropertyImpl::getDescriptionUnresolved(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    const ErrCode errCode = daqTry([&]()
    {
        StringPtr descriptionPtr = getUnresolved(this->description);
        *description = descriptionPtr.detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getUnitUnresolved(IBaseObject** unit)
{
    OPENDAQ_PARAM_NOT_NULL(unit);

    const ErrCode errCode = daqTry([&]()
    {
        BaseObjectPtr unitPtr = getUnresolved(this->unit);
        *unit = unitPtr.detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getMinValueUnresolved(INumber** min)
{
    OPENDAQ_PARAM_NOT_NULL(min);

    if (!this->minValue.assigned())
    {
        *min = nullptr;
        return OPENDAQ_SUCCESS;
    }

    const ErrCode errCode = daqTry([&]()
    {
        NumberPtr minPtr = getUnresolved(this->minValue);
        *min = minPtr.detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getMaxValueUnresolved(INumber** max)
{
    OPENDAQ_PARAM_NOT_NULL(max);

    const ErrCode errCode = daqTry([&]()
    {
        NumberPtr maxPtr = getUnresolved(this->maxValue);
        *max = maxPtr.detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getDefaultValueUnresolved(IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    const ErrCode errCode = daqTry([&]()
    {
        BaseObjectPtr defaultValuePtr = getUnresolved(this->defaultValue);
        *value = defaultValuePtr.detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getSuggestedValuesUnresolved(IList** values)
{
    OPENDAQ_PARAM_NOT_NULL(values);

    if (onSuggestedValuesRead.hasListeners())
        return getSuggestedValuesNoLock(values);

    ErrCode errCode = daqTry([&]()
    {
        ListPtr<IBaseObject> suggestedValuesPtr = getUnresolved(this->suggestedValues);
        *values = suggestedValuesPtr.detach();
    });

    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getVisibleUnresolved(IBoolean** visible)
{
    OPENDAQ_PARAM_NOT_NULL(visible);

    const ErrCode errCode = daqTry([&]()
    {
        BoolPtr visiblePtr = getUnresolved(this->visible);
        *visible = visiblePtr.detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getReadOnlyUnresolved(IBoolean** readOnly)
{
    OPENDAQ_PARAM_NOT_NULL(readOnly);

    const ErrCode errCode = daqTry([&]()
    {
        BoolPtr readOnlyPtr = getUnresolved(this->readOnly);
        *readOnly = readOnlyPtr.detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getSelectionValuesUnresolved(IBaseObject** values)
{
    OPENDAQ_PARAM_NOT_NULL(values);

    if (onSelectionValuesRead.hasListeners())
        return getSelectionValuesNoLock(values);

    ErrCode errCode = daqTry([&]()
    {
        BaseObjectPtr selectionValuesPtr = getUnresolved(this->selectionValues);
        *values = selectionValuesPtr.detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getReferencedPropertyUnresolved(IEvalValue** propertyEval)
{
    OPENDAQ_PARAM_NOT_NULL(propertyEval);

    const ErrCode errCode = daqTry([&]()
    {
        EvalValuePtr propertyEvalPtr = getUnresolved(this->refProp);
        *propertyEval = propertyEvalPtr.detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode PropertyImpl::getValueTypeUnresolved(CoreType* coreType)
{
    OPENDAQ_PARAM_NOT_NULL(coreType);

    *coreType = this->valueType;
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::getBuilderFromThis(IPropertyBuilder** builder)
{
    OPENDAQ_PARAM_NOT_NULL(builder);

    const ErrCode errCode = daqTry([&]
    {
        auto defaultValueObj = defaultValue;

        auto cloneableDefaultValue = defaultValue.asPtrOrNull<IPropertyObjectInternal>(true);
        if (cloneableDefaultValue.assigned())
            defaultValueObj = cloneableDefaultValue.clone();

        auto propBuilder = PropertyBuilder(name)
                            .setValueType(valueType)
                            .setDescription(description)
                            .setUnit(unit)
                            .setMinValue(minValue)
                            .setMaxValue(maxValue)
                            .setDefaultValue(defaultValueObj)
                            .setVisible(visible)
                            .setReadOnly(readOnly)
                            .setSelectionValues(selectionValues)
                            .setSuggestedValues(suggestedValues)
                            .setReferencedProperty(refProp)
                            .setCoercer(coercer)
                            .setValidator(validator)
                            .setCallableInfo(callableInfo)
                            .setOnPropertyValueRead(onValueRead)
                            .setOnPropertyValueWrite(onValueWrite)
                            .setOnSelectionValuesRead(onSelectionValuesRead)
                            .setOnSuggestedValuesRead(onSuggestedValuesRead);
        
        *builder = propBuilder.detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}


//
// IOwnable
//

ErrCode PropertyImpl::setOwner(IPropertyObject* owner)
{
    if (this->owner.assigned())
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ALREADYEXISTS, "Owner is already assigned.");
    }

    this->owner = owner;

    if (this->defaultValue.assigned())
    {
        if (const auto defaultValueObj = this->defaultValue.asPtrOrNull<IPropertyObject>(true); defaultValueObj.assigned())
        {
            PermissionManagerPtr parentManager;
            ErrCode err = owner->getPermissionManager(&parentManager);
            OPENDAQ_RETURN_IF_FAILED(err);

            defaultValueObj.getPermissionManager().asPtr<IPermissionManagerInternal>(true).setParent(parentManager);
        }
    }

    return OPENDAQ_SUCCESS;
}

ErrCode PropertyImpl::validateDuringConstruction()
{
    this->internalAddRefNoCheck();
    const ErrCode err = validate();
    this->internalReleaseRef();
    return err;
}

PropertyObjectPtr PropertyImpl::getOwner() const
{
    if (owner.assigned())
        return owner.getRef();
    return nullptr;
}

PropertyPtr PropertyImpl::bindAndGetRefProp(bool lock)
{
    PropertyPtr refPropPtr;
    checkErrorInfo(getReferencedPropertyInternal(&refPropPtr, lock));
    if (!refPropPtr.assigned())
        return nullptr;
        
    return refPropPtr;
}

template <typename TPtr>
TPtr PropertyImpl::bindAndGet(const BaseObjectPtr& metadata, bool lock) const
{       
    auto eval = metadata.asPtrOrNull<IEvalValue>();
    if (!eval.assigned())
        return metadata;

    if (const auto ownerPtr = getOwner(); ownerPtr.assigned())
        eval = eval.cloneWithOwner(ownerPtr);

    return lock ? eval.getResult() : eval.getResultNoLock();
}

BaseObjectPtr PropertyImpl::getUnresolved(const BaseObjectPtr& localMetadata) const
{
    if (const auto eval = localMetadata.asPtrOrNull<IEvalValue>(true); eval.assigned())
    {
        if (const auto ownerPtr = getOwner(); ownerPtr.assigned())
            return eval.cloneWithOwner(ownerPtr);
    }

    return localMetadata;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createProperty,
    IString*, name
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createBoolProperty,
    IString*, name,
    IBoolean*, defaultValue,
    IBoolean*, visible
)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createIntProperty,
    IString*, name,
    IInteger*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createFloatProperty,
    IString*, name,
    IFloat*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createStringProperty,
    IString*, name,
    IString*, defaultValue,
    IBoolean*, visible
)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createListProperty,
    IString*, name,
    IList*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createDictProperty,
    IString*, name,
    IDict*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createRatioProperty,
    IString*, name,
    IRatio*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createObjectProperty,
    IString*, name,
    IPropertyObject*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createFunctionProperty,
    IString*, name,
    ICallableInfo*, callableInfo,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createReferenceProperty,
    IString*, name,
    IEvalValue*, referencedPropertyEval
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createSelectionProperty,
    IString*, name,
    IList*, selectionValues,
    IInteger*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createSparseSelectionProperty,
    IString*, name,
    IDict*, selectionValues,
    IInteger*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createStructProperty,
    IString*, name,
    IStruct*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createEnumerationProperty,
    IString*, name,
    IEnumeration*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createPropertyFromBuilder,
    IPropertyBuilder*, builder
)

END_NAMESPACE_OPENDAQ
