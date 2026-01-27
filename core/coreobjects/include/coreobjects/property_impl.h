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
#include <coreobjects/ownable.h>
#include <coreobjects/ownable_ptr.h>
#include <coreobjects/property.h>
#include <coreobjects/property_builder_ptr.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_internal_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coreobjects/property_object_ptr.h>
#include <coreobjects/property_ptr.h>
#include <coreobjects/serialization_utils.h>
#include <coreobjects/unit_ptr.h>
#include <coretypes/coretypes.h>
#include <coretypes/exceptions.h>
#include <coretypes/validation.h>
#include <coreobjects/permission_manager_factory.h>
#include <coreobjects/property_metadata_read_args_ptr.h>
#include <coreobjects/permission_manager_internal_ptr.h>
#include <coreobjects/errors.h>
#include <coreobjects/property_object_protected.h>
#include <coreobjects/property_metadata_read_args_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class PropertyImpl : public ImplementationOf<IProperty, ISerializable, IPropertyInternal, IOwnable>
{
protected:
    PropertyImpl();

public:
    explicit PropertyImpl(const StringPtr& name);
    explicit PropertyImpl(IPropertyBuilder* propertyBuilder);

    PropertyImpl(const StringPtr& name, const BaseObjectPtr& defaultValue, const BooleanPtr& visible);

    // BoolProperty()
    PropertyImpl(const StringPtr& name, IBoolean* defaultValue, const BooleanPtr& visible);
    // IntProperty()
    PropertyImpl(const StringPtr& name, IInteger* defaultValue, const BooleanPtr& visible);
    // FloatProperty()
    PropertyImpl(const StringPtr& name, IFloat* defaultValue, const BooleanPtr& visible);
    // StringProperty()
    PropertyImpl(const StringPtr& name, IString* defaultValue, const BooleanPtr& visible);
    // ListProperty()
    PropertyImpl(const StringPtr& name, IList* defaultValue, const BooleanPtr& visible);
    // DictProperty()
    PropertyImpl(const StringPtr& name, IDict* defaultValue, const BooleanPtr& visible);
    // RatioProperty()
    PropertyImpl(const StringPtr& name, IRatio* defaultValue, const BooleanPtr& visible);
    // ObjectProperty()
    PropertyImpl(const StringPtr& name, IPropertyObject* defaultValue);
    // FunctionProperty()
    PropertyImpl(const StringPtr& name, ICallableInfo* callableInfo, const BooleanPtr& visible);
    // ReferenceProperty()
    PropertyImpl(const StringPtr& name, IEvalValue* referencedProperty);
    // SelectionProperty()
    PropertyImpl(const StringPtr& name, IList* selectionValues, IInteger* defaultValue, const BooleanPtr& visible);
    // SparseSelectionProperty()
    PropertyImpl(const StringPtr& name, IDict* selectionValues, IInteger* defaultValue, const BooleanPtr& visible);
    // StructProperty()
    PropertyImpl(const StringPtr& name, IStruct* defaultValue, const BooleanPtr& visible);
    // EnumerationProperty()
    PropertyImpl(const StringPtr& name, IEnumeration* defaultValue, const BooleanPtr& visible);

    ErrCode INTERFACE_FUNC getValueType(CoreType* type) override;
    ErrCode INTERFACE_FUNC getValueTypeNoLock(CoreType* type) override;
    ErrCode INTERFACE_FUNC getValueTypeInternal(CoreType* type, bool lock);

    ErrCode INTERFACE_FUNC getKeyType(CoreType* type) override;
    ErrCode INTERFACE_FUNC getKeyTypeNoLock(CoreType* type) override;
    ErrCode INTERFACE_FUNC getKeyTypeInternal(CoreType* type, bool lock);

    ErrCode INTERFACE_FUNC getItemType(CoreType* type) override;
    ErrCode INTERFACE_FUNC getItemTypeNoLock(CoreType* type) override;
    ErrCode INTERFACE_FUNC getItemTypeInternal(CoreType* type, bool lock);

    ErrCode INTERFACE_FUNC getName(IString** name) override;

    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC getDescriptionNoLock(IString** description) override;
    ErrCode getDescriptionInternal(IString** description, bool lock);

    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override;
    ErrCode INTERFACE_FUNC getUnitNoLock(IUnit** unit) override;
    ErrCode getUnitInternal(IUnit** unit, bool lock);

    ErrCode INTERFACE_FUNC getMinValue(INumber** min) override;
    ErrCode INTERFACE_FUNC getMinValueNoLock(INumber** min) override;
    ErrCode getMinValueInternal(INumber** min, bool lock);

    ErrCode INTERFACE_FUNC getMaxValue(INumber** max) override;
    ErrCode INTERFACE_FUNC getMaxValueNoLock(INumber** max) override;
    ErrCode getMaxValueInternal(INumber** max, bool lock);

    ErrCode INTERFACE_FUNC getDefaultValue(IBaseObject** value) override;
    ErrCode INTERFACE_FUNC getDefaultValueNoLock(IBaseObject** value) override;
    ErrCode getDefaultValueInternal(IBaseObject** value, bool lock);

    ErrCode INTERFACE_FUNC getSuggestedValues(IList** values) override;
    ErrCode INTERFACE_FUNC getSuggestedValuesNoLock(IList** values) override;
    virtual ErrCode getSuggestedValuesInternal(IList** values, bool lock);

    ErrCode INTERFACE_FUNC getVisible(Bool* visible) override;
    ErrCode INTERFACE_FUNC getVisibleNoLock(Bool* visible) override;
    ErrCode getVisibleInternal(Bool* visible, bool lock);

    ErrCode INTERFACE_FUNC getReadOnly(Bool* readOnly) override;
    ErrCode INTERFACE_FUNC getReadOnlyNoLock(Bool* readOnly) override;
    ErrCode getReadOnlyInternal(Bool* readOnly, bool lock);

    ErrCode INTERFACE_FUNC getSelectionValues(IBaseObject** values) override;
    ErrCode INTERFACE_FUNC getSelectionValuesNoLock(IBaseObject** values) override;
    virtual ErrCode getSelectionValuesInternal(IBaseObject** values, bool lock);

    ErrCode INTERFACE_FUNC getReferencedProperty(IProperty** property) override;
    ErrCode INTERFACE_FUNC getReferencedPropertyNoLock(IProperty** property) override;
    ErrCode getReferencedPropertyInternal(IProperty** property, bool lock);

    ErrCode INTERFACE_FUNC getIsReferenced(Bool* isReferenced) override;
    ErrCode INTERFACE_FUNC getIsReferencedNoLock(Bool* isReferenced) override;
    ErrCode getIsReferencedInternal(Bool* isReferenced, bool lock);

    ErrCode INTERFACE_FUNC getValidator(IValidator** validator) override;
    ErrCode INTERFACE_FUNC getValidatorNoLock(IValidator** validator) override;
    ErrCode getValidatorInternal(IValidator** validator, bool lock);

    ErrCode INTERFACE_FUNC getCoercer(ICoercer** coercer) override;
    ErrCode INTERFACE_FUNC getCoercerNoLock(ICoercer** coercer) override;
    ErrCode getCoercerInternal(ICoercer** coercer, bool lock);

    ErrCode INTERFACE_FUNC getCallableInfo(ICallableInfo** callableInfo) override;
    ErrCode INTERFACE_FUNC getCallableInfoNoLock(ICallableInfo** callableInfo) override;
    ErrCode getCallableInfoInternal(ICallableInfo** callableInfo, bool lock);

    ErrCode INTERFACE_FUNC getStructType(IStructType** structType) override;
    ErrCode INTERFACE_FUNC getStructTypeNoLock(IStructType** structType) override;
    ErrCode INTERFACE_FUNC getStructTypeInternal(IStructType** structType, bool lock);

    ErrCode INTERFACE_FUNC overrideDefaultValue(IBaseObject* newDefaultValue) override;

    ErrCode INTERFACE_FUNC getClassOnPropertyValueWrite(IEvent** event) override;
    ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IEvent** event) override;
    ErrCode INTERFACE_FUNC getClassOnPropertyValueRead(IEvent** event) override;
    ErrCode INTERFACE_FUNC getOnPropertyValueRead(IEvent** event) override;
    ErrCode INTERFACE_FUNC getOnSelectionValuesRead(IEvent** event) override;
    ErrCode INTERFACE_FUNC getOnSuggestedValuesRead(IEvent** event) override;

    ErrCode INTERFACE_FUNC getValue(IBaseObject** value) override;
    ErrCode INTERFACE_FUNC setValue(IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setValueProtected(IBaseObject* newValue) override;

    ErrCode INTERFACE_FUNC getHasOnReadListeners(Bool* hasListeners) override;
    ErrCode INTERFACE_FUNC getHasOnGetSuggestedValuesListeners(Bool* hasListeners) override;
    ErrCode INTERFACE_FUNC getHasOnGetSelectionValuesListeners(Bool* hasListeners) override;

    ErrCode INTERFACE_FUNC validate();

    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    static ConstCharPtr SerializeId();
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ErrCode ReadBuilderDeserializeValues(const PropertyBuilderPtr& builder, ISerializedObject* serializedObj, IBaseObject* context, IFunction* factoryCallback);
    static ErrCode Deserialize(ISerializedObject* serializedObj, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    // IPropertyInternal
    ErrCode INTERFACE_FUNC clone(IProperty** clonedProperty) override;
    ErrCode INTERFACE_FUNC cloneWithOwner(IPropertyObject* owner, IProperty** clonedProperty) override;

    ErrCode INTERFACE_FUNC getDescriptionUnresolved(IString** description) override;
    ErrCode INTERFACE_FUNC getUnitUnresolved(IBaseObject** unit) override;
    ErrCode INTERFACE_FUNC getMinValueUnresolved(INumber** min) override;
    ErrCode INTERFACE_FUNC getMaxValueUnresolved(INumber** max) override;
    ErrCode INTERFACE_FUNC getDefaultValueUnresolved(IBaseObject** value) override;
    ErrCode INTERFACE_FUNC getSuggestedValuesUnresolved(IList** values) override;
    ErrCode INTERFACE_FUNC getVisibleUnresolved(IBoolean** visible) override;
    ErrCode INTERFACE_FUNC getReadOnlyUnresolved(IBoolean** readOnly) override;
    ErrCode INTERFACE_FUNC getSelectionValuesUnresolved(IBaseObject** values) override;
    ErrCode INTERFACE_FUNC getReferencedPropertyUnresolved(IEvalValue** propertyEval) override;
    ErrCode INTERFACE_FUNC getValueTypeUnresolved(CoreType* coreType) override;

    // IOwnable
    ErrCode INTERFACE_FUNC setOwner(IPropertyObject* owner) override;

    ErrCode validateDuringConstruction();
    PropertyObjectPtr getOwner() const;

protected:
    PropertyPtr propPtr;
    WeakRefPtr<IPropertyObject> owner;
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
    EventEmitter<PropertyPtr, PropertyMetadataReadArgsPtr> onSelectionValuesRead;
    EventEmitter<PropertyPtr, PropertyMetadataReadArgsPtr> onSuggestedValuesRead;

private:
    PropertyPtr bindAndGetRefProp(bool lock);

    template <typename TPtr>
    TPtr bindAndGet(const BaseObjectPtr& metadata, bool lock) const;

    BaseObjectPtr getUnresolved(const BaseObjectPtr& localMetadata) const;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(PropertyImpl)

END_NAMESPACE_OPENDAQ
