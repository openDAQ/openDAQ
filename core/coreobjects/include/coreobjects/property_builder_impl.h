/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <coreobjects/callable_info_ptr.h>
#include <coreobjects/eval_value_ptr.h>
#include <coreobjects/property_builder.h>
#include <coreobjects/property_builder_ptr.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_ptr.h>
#include <coreobjects/unit_ptr.h>
#include <coretypes/coretypes.h>
#include <coretypes/exceptions.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

class PropertyBuilderImpl : public ImplementationOf<IPropertyBuilder>
{
protected:
    PropertyBuilderImpl()
        : valueType(ctUndefined)
        , visible(true)
        , readOnly(false)
    {
        if (valueType == ctBinaryData)
        {
            DAQ_THROW_EXCEPTION(InvalidTypeException, "Properties cannot be BinaryData types");
        }
    }

public:
    // Property(name)
    explicit PropertyBuilderImpl(const StringPtr& name)
        : PropertyBuilderImpl()
    {
        this->name = name;
        this->visible = true;
    }

    PropertyBuilderImpl(const StringPtr& name, const BaseObjectPtr& defaultValue)
        : PropertyBuilderImpl(name)
    {
        this->defaultValue = defaultValue;
    }

    // BoolProperty()
    PropertyBuilderImpl(const StringPtr& name, IBoolean* defaultValue)
        : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
    {
        this->valueType = ctBool;
    }

    // IntProperty()
    PropertyBuilderImpl(const StringPtr& name, IInteger* defaultValue)
        : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
    {
        this->valueType = ctInt;
    }

    // FloatProperty()
    PropertyBuilderImpl(const StringPtr& name, IFloat* defaultValue)
        : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
    {
        this->valueType = ctFloat;
    }

    // StringProperty()
    PropertyBuilderImpl(const StringPtr& name, IString* defaultValue)
        : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
    {
        this->valueType = ctString;
    }

    // ListProperty()
    PropertyBuilderImpl(const StringPtr& name, IList* defaultValue)
        : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
    {
        this->valueType = ctList;
    }

    // DictProperty()
    PropertyBuilderImpl(const StringPtr& name, IDict* defaultValue)
        : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
    {
        this->valueType = ctDict;
    }

    // RatioProperty()
    PropertyBuilderImpl(const StringPtr& name, IRatio* defaultValue)
        : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
    {
        this->valueType = ctRatio;
    }

    // ObjectProperty()
    PropertyBuilderImpl(const StringPtr& name, IPropertyObject* defaultValue)
        : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
    {
        this->valueType = ctObject;
        this->readOnly = true;
        if (defaultValue == nullptr)
            this->defaultValue = PropertyObject().detach();
    }

    // FunctionProperty()
    PropertyBuilderImpl(const StringPtr& name, ICallableInfo* callableInfo)
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
    PropertyBuilderImpl(const StringPtr& name, IEvalValue* referencedProperty)
        : PropertyBuilderImpl(name)
    {
        this->refProp = referencedProperty;
    }

    // SelectionProperty()
    PropertyBuilderImpl(const StringPtr& name, IList* selectionValues, IInteger* defaultValue)
        : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
    {
        this->valueType = ctInt;
        this->selectionValues = BaseObjectPtr(selectionValues);
    }

    // SparseSelectionProperty()
    PropertyBuilderImpl(const StringPtr& name, IDict* selectionValues, IInteger* defaultValue)
        : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
    {
        this->valueType = ctInt;
        this->selectionValues = BaseObjectPtr(selectionValues);
    }

    // StructureProperty()
    PropertyBuilderImpl(const StringPtr& name, IStruct* defaultValue)
        : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
    {
        this->valueType = ctStruct;
    }

    // EnumerationProperty()
    PropertyBuilderImpl(const StringPtr& name, IEnumeration* defaultValue)
        : PropertyBuilderImpl(name, BaseObjectPtr(defaultValue))
    {
        this->valueType = ctEnumeration;
    }

    ErrCode INTERFACE_FUNC build(IProperty** property) override
    {
        OPENDAQ_PARAM_NOT_NULL(property);
        
        const auto propertyBuilderPtr = this->borrowPtr<PropertyBuilderPtr>();

        return daqTry([&]()
        {
            *property = PropertyFromBuilder(propertyBuilderPtr).detach(); 
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC setValueType(CoreType type) override
    {
        this->valueType = type;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getValueType(CoreType* type) override
    {
        OPENDAQ_PARAM_NOT_NULL(type);
        
        *type = this->valueType;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setName(IString* name) override
    {
        this->name = name;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getName(IString** name) override
    {
        OPENDAQ_PARAM_NOT_NULL(name);
        
        *name = this->name.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }
    
    ErrCode INTERFACE_FUNC setDescription(IString* description) override
    {
        this->description = description;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getDescription(IString** description) override
    {
        OPENDAQ_PARAM_NOT_NULL(description);
        
        *description = this->description.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }
    
    ErrCode INTERFACE_FUNC setUnit(IUnit* unit) override
    {
        this->unit = unit;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override
    {
        OPENDAQ_PARAM_NOT_NULL(unit); 

        *unit = this->unit.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setMinValue(INumber* min) override
    {
        this->minValue = min;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getMinValue(INumber** min) override
    {
        OPENDAQ_PARAM_NOT_NULL(min);
        
        *min = this->minValue.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setMaxValue(INumber* max) override
    {
        this->maxValue = max;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getMaxValue(INumber** max) override
    {
        OPENDAQ_PARAM_NOT_NULL(max);
        
        *max = this->maxValue.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setDefaultValue(IBaseObject* value) override
    {
        if (value != nullptr)
        {
            const auto valuePtr = BaseObjectPtr::Borrow(value);
            if (const auto freezable = valuePtr.asPtrOrNull<IFreezable>(); freezable.assigned())
            {
                const auto err = freezable->freeze();
                OPENDAQ_RETURN_IF_FAILED(err);
            }
        }

        this->defaultValue = value;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getDefaultValue(IBaseObject** value) override
    {
        OPENDAQ_PARAM_NOT_NULL(value);
    
        *value = this->defaultValue.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setSuggestedValues(IList* values) override
    {
        if (values != nullptr)
        {
            const auto valuePtr = BaseObjectPtr::Borrow(values);
            if (const auto freezable = valuePtr.asPtrOrNull<IFreezable>(); freezable.assigned())
            {
                const auto err = freezable->freeze();
                OPENDAQ_RETURN_IF_FAILED(err);
            }       
        }

        this->suggestedValues = values;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getSuggestedValues(IList** values) override
    {
        OPENDAQ_PARAM_NOT_NULL(values);

        *values = this->suggestedValues.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }
    
    ErrCode INTERFACE_FUNC setVisible(IBoolean* visible) override
    {
        this->visible = visible;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getVisible(IBoolean** visible) override
    {
        OPENDAQ_PARAM_NOT_NULL(visible);
        
        *visible = this->visible.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }
    
    ErrCode INTERFACE_FUNC setReadOnly(IBoolean* readOnly) override
    {
        this->readOnly = readOnly;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getReadOnly(IBoolean** readOnly) override
    {
        OPENDAQ_PARAM_NOT_NULL(readOnly);
        
        *readOnly = this->readOnly.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setSelectionValues(IBaseObject* values) override
    {
        if (values != nullptr)
        {
            const auto valuePtr = BaseObjectPtr::Borrow(values);
            if (const auto freezable = valuePtr.asPtrOrNull<IFreezable>(); freezable.assigned())
            {
                const auto err = freezable->freeze();
                OPENDAQ_RETURN_IF_FAILED(err);
            }
        }

        this->selectionValues = values;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getSelectionValues(IBaseObject** values) override
    {
        OPENDAQ_PARAM_NOT_NULL(values);
        
        *values = this->selectionValues.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setReferencedProperty(IEvalValue* propertyEval) override
    {
        this->refProp = propertyEval;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getReferencedProperty(IEvalValue** propertyEval) override
    {
        OPENDAQ_PARAM_NOT_NULL(propertyEval);
        
        *propertyEval = this->refProp.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setValidator(IValidator* validator) override
    {
        this->validator = validator;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getValidator(IValidator** validator) override
    {
        OPENDAQ_PARAM_NOT_NULL(validator);
        
        *validator = this->validator.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setCoercer(ICoercer* coercer) override
    {
        this->coercer = coercer;
        return OPENDAQ_SUCCESS;
    }
    
    ErrCode INTERFACE_FUNC getCoercer(ICoercer** coercer) override
    {
        OPENDAQ_PARAM_NOT_NULL(coercer);
    
        *coercer = this->coercer.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setCallableInfo(ICallableInfo* callable) override
    {
        callableInfo = callable;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getCallableInfo(ICallableInfo** callable) override
    {
        OPENDAQ_PARAM_NOT_NULL(callable);
        
        *callable = this->callableInfo.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setOnPropertyValueWrite(IEvent* event) override
    {
        this->onValueWrite = event;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IEvent** event) override
    {
        OPENDAQ_PARAM_NOT_NULL(event);
        
        *event = this->onValueWrite.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setOnPropertyValueRead(IEvent* event) override
    {
        this->onValueRead = event;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getOnPropertyValueRead(IEvent** event) override
    {
        OPENDAQ_PARAM_NOT_NULL(event);
        
        *event = this->onValueRead.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

private:
    CoreType valueType;

    StringPtr name;
    StringPtr description;

    UnitPtr unit;

    NumberPtr minValue;
    NumberPtr maxValue;

    BaseObjectPtr defaultValue;

    BooleanPtr visible;

    BooleanPtr readOnly;
    BaseObjectPtr selectionValues;
    ListPtr<IBaseObject> suggestedValues;

    EvalValuePtr refProp;

    CoercerPtr coercer;
    ValidatorPtr validator;

    CallableInfoPtr callableInfo;
    EventEmitter<PropertyObjectPtr, PropertyValueEventArgsPtr> onValueWrite;
    EventEmitter<PropertyObjectPtr, PropertyValueEventArgsPtr> onValueRead;
};

END_NAMESPACE_OPENDAQ
