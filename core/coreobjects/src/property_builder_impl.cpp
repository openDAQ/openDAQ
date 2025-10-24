#include <coreobjects/property_builder_impl.h>
#include <coretypes/exceptions.h>
#include <coretypes/validation.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_internal_ptr.h>
#include <coretypes/event_ptr.h>


BEGIN_NAMESPACE_OPENDAQ

PropertyBuilderImpl::PropertyBuilderImpl()
    : valueType(ctUndefined)
    , visible(true)
    , readOnly(false)
{
    if (valueType == ctBinaryData)
    {
        DAQ_THROW_EXCEPTION(InvalidTypeException, "Properties cannot be BinaryData types");
    }
}

PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name)
    : PropertyBuilderImpl()
{
    this->name = name;
    this->visible = true;
}

PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, const BaseObjectPtr& defaultValue)
    : PropertyBuilderImpl(name)
{
    this->defaultValue = defaultValue;
}

// BoolProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, IBoolean* defaultValue)
    : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
{
    this->valueType = ctBool;
}

// IntProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, IInteger* defaultValue)
    : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
{
    this->valueType = ctInt;
}

// FloatProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, IFloat* defaultValue)
    : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
{
    this->valueType = ctFloat;
}

// StringProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, IString* defaultValue)
    : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
{
    this->valueType = ctString;
}

// ListProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, IList* defaultValue)
    : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
{
    this->valueType = ctList;
}

// DictProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, IDict* defaultValue)
    : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
{
    this->valueType = ctDict;
}

// RatioProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, IRatio* defaultValue)
    : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
{
    this->valueType = ctRatio;
}

// ObjectProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, IPropertyObject* defaultValue)
    : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
{
    this->valueType = ctObject;
    this->readOnly = true;
    if (defaultValue == nullptr)
        this->defaultValue = PropertyObject().detach();
}

// FunctionProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, ICallableInfo* callableInfo)
    : PropertyBuilderImpl(name)
{
    this->visible = true;
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
}

// ReferenceProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, IEvalValue* referencedProperty)
    : PropertyBuilderImpl(name)
{
    this->refProp = referencedProperty;
}

// SelectionProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, IList* selectionValues, IInteger* defaultValue)
    : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
{
    this->valueType = ctInt;
    this->selectionValues = BaseObjectPtr(selectionValues);
}

// SparseSelectionProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, IDict* selectionValues, IInteger* defaultValue)
    : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
{
    this->valueType = ctInt;
    this->selectionValues = BaseObjectPtr(selectionValues);
}

// StructureProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, IStruct* defaultValue)
    : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
{
    this->valueType = ctStruct;
}

// EnumerationProperty()
PropertyBuilderImpl::PropertyBuilderImpl(const StringPtr& name, IEnumeration* defaultValue)
    : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
{
    this->valueType = ctEnumeration;
}

PropertyBuilderImpl::PropertyBuilderImpl(IProperty* property)
{
    if (property == nullptr)
        DAQ_THROW_EXCEPTION(ArgumentNullException);

    const auto propertyPtr = PropertyPtr::Borrow(property);
    const auto propertyInternal = propertyPtr.asPtr<IPropertyInternal>();

    checkErrorInfo(property->getName(&this->name));
    checkErrorInfo(property->getValueType(&this->valueType));
    checkErrorInfo(property->getDescription(&this->description));
    checkErrorInfo(property->getUnit(&this->unit));
    checkErrorInfo(property->getMinValue(&this->minValue));
    checkErrorInfo(property->getMaxValue(&this->maxValue));

    checkErrorInfo(propertyInternal->getDefaultValueUnresolved(&this->defaultValue));
    this->visible = propertyPtr.getVisible();
    this->readOnly = propertyPtr.getReadOnly();
    checkErrorInfo(property->getSelectionValues(&this->selectionValues));
    checkErrorInfo(property->getSuggestedValues(&this->suggestedValues));
    checkErrorInfo(propertyInternal->getReferencedPropertyUnresolved(&this->refProp));

    checkErrorInfo(property->getCoercer(&this->coercer));
    checkErrorInfo(property->getValidator(&this->validator));
    checkErrorInfo(property->getCallableInfo(&this->callableInfo));

    checkErrorInfo(propertyInternal->getClassOnPropertyValueRead(&this->onValueRead));
    checkErrorInfo(propertyInternal->getClassOnPropertyValueWrite(&this->onValueWrite));

    checkErrorInfo(propertyPtr->getOnSelectionValuesRead(&this->onSuggestedValuesRead));
    checkErrorInfo(propertyPtr->getOnSuggestedValuesRead(&this->onSelectionValuesRead));
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::build(IProperty** property)
{
    OPENDAQ_PARAM_NOT_NULL(property);
    
    const auto propertyBuilderPtr = this->borrowPtr<PropertyBuilderPtr>();

    return daqTry([&]()
    {
        *property = PropertyFromBuilder(propertyBuilderPtr).detach(); 
        return OPENDAQ_SUCCESS;
    });
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setValueType(CoreType type)
{
    this->valueType = type;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getValueType(CoreType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    
    *type = this->valueType;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setName(IString* name)
{
    this->name = name;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    
    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setDescription(IString* description)
{
    this->description = description;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);
    
    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setUnit(IUnit* unit)
{
    this->unit = unit;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getUnit(IUnit** unit)
{
    OPENDAQ_PARAM_NOT_NULL(unit); 

    *unit = this->unit.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setMinValue(INumber* min)
{
    this->minValue = min;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getMinValue(INumber** min)
{
    OPENDAQ_PARAM_NOT_NULL(min);
    
    *min = this->minValue.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setMaxValue(INumber* max)
{
    this->maxValue = max;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getMaxValue(INumber** max)
{
    OPENDAQ_PARAM_NOT_NULL(max);
    
    *max = this->maxValue.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setDefaultValue(IBaseObject* value)
{
    if (value != nullptr)
    {
        const auto valuePtr = BaseObjectPtr::Borrow(value);
        if (valuePtr.assigned() && !valuePtr.supportsInterface(IPropertyObject::Id))
            if (const auto freezable = valuePtr.asPtrOrNull<IFreezable>(); freezable.assigned())
            {
                const auto err = freezable->freeze();
                OPENDAQ_RETURN_IF_FAILED(err);
            }
    }

    this->defaultValue = value;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getDefaultValue(IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    *value = this->defaultValue.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setSuggestedValues(IList* values)
{
    if (values != nullptr)
    {
        const auto valuePtr = BaseObjectPtr::Borrow(values);
        if (valuePtr.assigned())
            if (const auto freezable = valuePtr.asPtrOrNull<IFreezable>(); freezable.assigned())
            {
                const auto err = freezable->freeze();
                OPENDAQ_RETURN_IF_FAILED(err);
            }
    }

    this->suggestedValues = values;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getSuggestedValues(IList** values)
{
    OPENDAQ_PARAM_NOT_NULL(values);

    *values = this->suggestedValues.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setVisible(IBoolean* visible)
{
    this->visible = visible;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getVisible(IBoolean** visible)
{
    OPENDAQ_PARAM_NOT_NULL(visible);
    
    *visible = this->visible.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setReadOnly(IBoolean* readOnly)
{
    this->readOnly = readOnly;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getReadOnly(IBoolean** readOnly)
{
    OPENDAQ_PARAM_NOT_NULL(readOnly);
    
    *readOnly = this->readOnly.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setSelectionValues(IBaseObject* values)
{
    if (values != nullptr)
    {
        const auto valuePtr = BaseObjectPtr::Borrow(values);
        if (valuePtr.assigned())
            if (const auto freezable = valuePtr.asPtrOrNull<IFreezable>(); freezable.assigned())
            {
                const auto err = freezable->freeze();
                OPENDAQ_RETURN_IF_FAILED(err);
            }
    }

    this->selectionValues = values;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getSelectionValues(IBaseObject** values)
{
    OPENDAQ_PARAM_NOT_NULL(values);
    
    *values = this->selectionValues.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setReferencedProperty(IEvalValue* propertyEval)
{
    this->refProp = propertyEval;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getReferencedProperty(IEvalValue** propertyEval)
{
    OPENDAQ_PARAM_NOT_NULL(propertyEval);
    
    *propertyEval = this->refProp.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setValidator(IValidator* validator)
{
    this->validator = validator;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getValidator(IValidator** validator)
{
    OPENDAQ_PARAM_NOT_NULL(validator);
    
    *validator = this->validator.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setCoercer(ICoercer* coercer)
{
    this->coercer = coercer;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getCoercer(ICoercer** coercer)
{
    OPENDAQ_PARAM_NOT_NULL(coercer);

    *coercer = this->coercer.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setCallableInfo(ICallableInfo* callable)
{
    callableInfo = callable;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getCallableInfo(ICallableInfo** callable)
{
    OPENDAQ_PARAM_NOT_NULL(callable);
    
    *callable = this->callableInfo.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setOnPropertyValueWrite(IEvent* event)
{
    this->onValueWrite = event;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getOnPropertyValueWrite(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);
    
    *event = this->onValueWrite.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::setOnPropertyValueRead(IEvent* event)
{
    this->onValueRead = event;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PropertyBuilderImpl::getOnPropertyValueRead(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);
    
    *event = this->onValueRead.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyBuilderImpl::setOnSuggestedValuesRead(IEvent* event)
{
    this->onSuggestedValuesRead = event;
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyBuilderImpl::getOnSuggestedValuesRead(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);
    
    *event = this->onSuggestedValuesRead.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyBuilderImpl::setOnSelectionValuesRead(IEvent* event)
{
    this->onSelectionValuesRead = event;
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyBuilderImpl::getOnSelectionValuesRead(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);
    
    *event = this->onSelectionValuesRead.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY,
    PropertyBuilder,
    IPropertyBuilder,
    createPropertyBuilder,
    IString*,
    name
    )

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createBoolPropertyBuilder,
    IString*, name,
    IBoolean*, defaultValue
)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createIntPropertyBuilder,
    IString*, name,
    IInteger*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createFloatPropertyBuilder,
    IString*, name,
    IFloat*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createStringPropertyBuilder,
    IString*, name,
    IString*, defaultValue
)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createListPropertyBuilder,
    IString*, name,
    IList*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createDictPropertyBuilder,
    IString*, name,
    IDict*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createRatioPropertyBuilder,
    IString*, name,
    IRatio*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createObjectPropertyBuilder,
    IString*, name,
    IPropertyObject*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createFunctionPropertyBuilder,
    IString*, name,
    ICallableInfo*, callableInfo
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createReferencePropertyBuilder,
    IString*, name,
    IEvalValue*, referencedPropertyEval
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createSelectionPropertyBuilder,
    IString*, name,
    IList*, selectionValues,
    IInteger*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createSparseSelectionPropertyBuilder,
    IString*, name,
    IDict*, selectionValues,
    IInteger*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createStructPropertyBuilder,
    IString*, name,
    IStruct*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createEnumerationPropertyBuilder,
    IString*, name,
    IEnumeration*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createPropertyBuilderFromExisting,
    IProperty*, property
)

END_NAMESPACE_OPENDAQ
