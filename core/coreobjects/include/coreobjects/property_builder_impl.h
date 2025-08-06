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
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_ptr.h>
#include <coreobjects/unit_ptr.h>
#include <coretypes/coretypes.h>

BEGIN_NAMESPACE_OPENDAQ

class PropertyBuilderImpl : public ImplementationOf<IPropertyBuilder>
{
protected:
    PropertyBuilderImpl();

public:
    // Property(name)
    PropertyBuilderImpl(const StringPtr& name);
    PropertyBuilderImpl(const StringPtr& name, const BaseObjectPtr& defaultValue);

    // BoolProperty()
    PropertyBuilderImpl(const StringPtr& name, IBoolean* defaultValue);

    // IntProperty()
    PropertyBuilderImpl(const StringPtr& name, IInteger* defaultValue);

    // FloatProperty()
    PropertyBuilderImpl(const StringPtr& name, IFloat* defaultValue);

    // StringProperty()
    PropertyBuilderImpl(const StringPtr& name, IString* defaultValue);

    // ListProperty()
    PropertyBuilderImpl(const StringPtr& name, IList* defaultValue);

    // DictProperty()
    PropertyBuilderImpl(const StringPtr& name, IDict* defaultValue);

    // RatioProperty()
    PropertyBuilderImpl(const StringPtr& name, IRatio* defaultValue);

    // ObjectProperty()
    PropertyBuilderImpl(const StringPtr& name, IPropertyObject* defaultValue);

    // FunctionProperty()
    PropertyBuilderImpl(const StringPtr& name, ICallableInfo* callableInfo);

    // ReferenceProperty()
    PropertyBuilderImpl(const StringPtr& name, IEvalValue* referencedProperty);

    // SelectionProperty()
    PropertyBuilderImpl(const StringPtr& name, IList* selectionValues, IInteger* defaultValue),

    // SparseSelectionProperty()
    PropertyBuilderImpl(const StringPtr& name, IDict* selectionValues, IInteger* defaultValue);

    // StructureProperty()
    PropertyBuilderImpl(const StringPtr& name, IStruct* defaultValue);

    // EnumerationProperty()
    PropertyBuilderImpl(const StringPtr& name, IEnumeration* defaultValue);

    ErrCode INTERFACE_FUNC build(IProperty** property) override;
    ErrCode INTERFACE_FUNC setValueType(CoreType type) override;
    ErrCode INTERFACE_FUNC getValueType(CoreType* type) override;
    ErrCode INTERFACE_FUNC setName(IString* name) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC setDescription(IString* description) override;
    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC setUnit(IUnit* unit) override;
    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override;
    ErrCode INTERFACE_FUNC setMinValue(INumber* min) override;
    ErrCode INTERFACE_FUNC getMinValue(INumber** min) override;
    ErrCode INTERFACE_FUNC setMaxValue(INumber* max) override;
    ErrCode INTERFACE_FUNC getMaxValue(INumber** max) override;
    ErrCode INTERFACE_FUNC setDefaultValue(IBaseObject* value) override;
    ErrCode INTERFACE_FUNC getDefaultValue(IBaseObject** value) override;
    ErrCode INTERFACE_FUNC setSuggestedValues(IList* values) override;
    ErrCode INTERFACE_FUNC getSuggestedValues(IList** values) override;
    ErrCode INTERFACE_FUNC setVisible(IBoolean* visible) override;
    ErrCode INTERFACE_FUNC getVisible(IBoolean** visible) override;
    ErrCode INTERFACE_FUNC setReadOnly(IBoolean* readOnly) override;
    ErrCode INTERFACE_FUNC getReadOnly(IBoolean** readOnly) override;
    ErrCode INTERFACE_FUNC setSelectionValues(IBaseObject* values) override;
    ErrCode INTERFACE_FUNC getSelectionValues(IBaseObject** values) override;
    ErrCode INTERFACE_FUNC setReferencedProperty(IEvalValue* propertyEval) override;
    ErrCode INTERFACE_FUNC getReferencedProperty(IEvalValue** propertyEval) override;
    ErrCode INTERFACE_FUNC setValidator(IValidator* validator) override;
    ErrCode INTERFACE_FUNC getValidator(IValidator** validator) override;
    ErrCode INTERFACE_FUNC setCoercer(ICoercer* coercer) override;
    ErrCode INTERFACE_FUNC getCoercer(ICoercer** coercer) override;
    ErrCode INTERFACE_FUNC setCallableInfo(ICallableInfo* callable) override;
    ErrCode INTERFACE_FUNC getCallableInfo(ICallableInfo** callable) override;
    ErrCode INTERFACE_FUNC setOnPropertyValueWrite(IEvent* event) override;
    ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IEvent** event) override;
    ErrCode INTERFACE_FUNC setOnPropertyValueRead(IEvent* event) override;
    ErrCode INTERFACE_FUNC getOnPropertyValueRead(IEvent** event) override;
    ErrCode INTERFACE_FUNC setOnSuggestedValuesRead(IEvent* event) override;
    ErrCode INTERFACE_FUNC getOnSuggestedValuesRead(IEvent** event) override;
    ErrCode INTERFACE_FUNC setOnSelectionValuesRead(IEvent* event) override;
    ErrCode INTERFACE_FUNC getOnSelectionValuesRead(IEvent** event) override;

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
    EventEmitter<PropertyObjectPtr, PropertyMetadataReadArgsPtr> onSuggestedValuesRead;
    EventEmitter<PropertyObjectPtr, PropertyMetadataReadArgsPtr> onSelectionValuesRead;
};

END_NAMESPACE_OPENDAQ
