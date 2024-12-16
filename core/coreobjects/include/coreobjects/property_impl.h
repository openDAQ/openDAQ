/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <iostream>
#include <coreobjects/permission_manager_factory.h>
#include <coreobjects/permissions_builder_factory.h>
#include <coreobjects/permission_manager_internal_ptr.h>
#include <coreobjects/errors.h>
#include <coreobjects/permission_mask_builder_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace details
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

    static CoreType intfIdToCoreType(IntfID intfID)
    {
        if (intfIdToCoreTypeMap.find(intfID) == intfIdToCoreTypeMap.end())
        {
            return ctUndefined;
        }

        return intfIdToCoreTypeMap.at(intfID);
    }
}

class PropertyImpl : public ImplementationOf<IProperty, ISerializable, IPropertyInternal, IOwnable>
{
protected:
    PropertyImpl()
        : owner(nullptr)
        , valueType(ctUndefined)
        , visible(true)
        , readOnly(false)
    {
        propPtr = this->borrowPtr<PropertyPtr>();

        initDefaultPermissionManager();
    }

public:
    explicit PropertyImpl(const StringPtr& name)
        : PropertyImpl()
    {
        this->name = name;
    }

    explicit PropertyImpl(IPropertyBuilder* propertyBuilder)
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

        propPtr = this->borrowPtr<PropertyPtr>();
        owner = nullptr;

        initDefaultPermissionManager();
        checkErrorInfo(validateDuringConstruction());
    }

    PropertyImpl(const StringPtr& name, const BaseObjectPtr& defaultValue, const BooleanPtr& visible)
        : PropertyImpl(name)
    {
        this->defaultValue = defaultValue;
        this->visible = visible;
    }

    // BoolProperty()
    PropertyImpl(const StringPtr& name, IBoolean* defaultValue, const BooleanPtr& visible)
        : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
    {
        this->valueType = ctBool;

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    // IntProperty()
    PropertyImpl(const StringPtr& name, IInteger* defaultValue, const BooleanPtr& visible)
        : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
    {
        this->valueType = ctInt;

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    // FloatProperty()
    PropertyImpl(const StringPtr& name, IFloat* defaultValue, const BooleanPtr& visible)
        : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
    {
        this->valueType = ctFloat;

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    // StringProperty()
    PropertyImpl(const StringPtr& name, IString* defaultValue, const BooleanPtr& visible)
        : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
    {
        this->valueType = ctString;

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    // ListProperty()
    PropertyImpl(const StringPtr& name, IList* defaultValue, const BooleanPtr& visible)
        : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
    {
        this->valueType = ctList;

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    // DictProperty()
    PropertyImpl(const StringPtr& name, IDict* defaultValue, const BooleanPtr& visible)
        : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
    {
        this->valueType = ctDict;

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    // RatioProperty()
    PropertyImpl(const StringPtr& name, IRatio* defaultValue, const BooleanPtr& visible)
        : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
    {
        this->valueType = ctRatio;

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    // ObjectProperty()
    PropertyImpl(const StringPtr& name, IPropertyObject* defaultValue)
        : PropertyImpl(name, BaseObjectPtr(defaultValue), true)
    {
        this->valueType = ctObject;

        if (defaultValue == nullptr)
            this->defaultValue = PropertyObject().detach();

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);

        if (this->defaultValue.assigned())
        {
            auto defaultValueObj = this->defaultValue.asPtr<IPropertyObject>();
            defaultValueObj.getPermissionManager().asPtr<IPermissionManagerInternal>().setParent(this->defaultPermissionManager);
        }
    }

    // FunctionProperty()
    PropertyImpl(const StringPtr& name, ICallableInfo* callableInfo, const BooleanPtr& visible)
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

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    // ReferenceProperty()
    PropertyImpl(const StringPtr& name, IEvalValue* referencedProperty)
        : PropertyImpl(name)
    {
        this->refProp = referencedProperty;

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    // SelectionProperty()
    PropertyImpl(const StringPtr& name, IList* selectionValues, IInteger* defaultValue, const BooleanPtr& visible)
        : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
    {
        this->valueType = ctInt;
        this->selectionValues = BaseObjectPtr(selectionValues);

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    // SparseSelectionProperty()
    PropertyImpl(const StringPtr& name, IDict* selectionValues, IInteger* defaultValue, const BooleanPtr& visible)
        : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
    {
        this->valueType = ctInt;
        this->selectionValues = BaseObjectPtr(selectionValues);

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    // StructProperty()
    PropertyImpl(const StringPtr& name, IStruct* defaultValue, const BooleanPtr& visible)
        : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
    {
        this->valueType = ctStruct;
        this->selectionValues = BaseObjectPtr(selectionValues);

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    // EnumerationProperty()
    PropertyImpl(const StringPtr& name, IEnumeration* defaultValue, const BooleanPtr& visible)
        : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
    {
        this->valueType = ctEnumeration;
        this->selectionValues = BaseObjectPtr(selectionValues);

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    void initDefaultPermissionManager()
    {
        const auto defaultPermissions =
            PermissionsBuilder().inherit(false).assign("everyone", PermissionMaskBuilder().read().write().execute()).build();

        defaultPermissionManager = PermissionManager();
        defaultPermissionManager.setPermissions(defaultPermissions);
    }

    ErrCode INTERFACE_FUNC getValueType(CoreType* type) override
    {
        return getValueTypeInternal(type, true);
    }

    ErrCode INTERFACE_FUNC getValueTypeNoLock(CoreType* type) override
    {
        return getValueTypeInternal(type, false);
    }

    ErrCode INTERFACE_FUNC getValueTypeInternal(CoreType* type, bool lock)
    {
        OPENDAQ_PARAM_NOT_NULL(type);

	    return daqTry([&]()
            {
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        *type = lock ? prop.getValueType() : prop.asPtr<IPropertyInternal>().getValueTypeNoLock();
		        else
			        *type = this->valueType;
			        
		        return OPENDAQ_SUCCESS;
	        });
    }
    
    ErrCode INTERFACE_FUNC getKeyType(CoreType* type) override
    {
        return getKeyTypeInternal(type, true);    
    }

    ErrCode INTERFACE_FUNC getKeyTypeNoLock(CoreType* type) override
    {
        return getKeyTypeInternal(type, false);    
    }

    ErrCode INTERFACE_FUNC getKeyTypeInternal(CoreType* type, bool lock)
    {
        OPENDAQ_PARAM_NOT_NULL(type);

        *type = ctUndefined;
        BaseObjectPtr defVal;
        auto err = lock ? this->getDefaultValue(&defVal) : this->getDefaultValueNoLock(&defVal);
        if (OPENDAQ_FAILED(err))
            return err;

        if (!defVal.assigned())
            return OPENDAQ_SUCCESS;

        const auto value = defVal.asPtrOrNull<IDict>();
        if (!value.assigned())
            return OPENDAQ_SUCCESS;

        IntfID intfID;
        err = value.asPtr<IDictElementType>()->getKeyInterfaceId(&intfID);
        if (OPENDAQ_FAILED(err))
            return err;

        auto coreType = details::intfIdToCoreType(intfID);

        // TODO: Workaround if item type of dict/list is undefined
        if (coreType == ctUndefined && value.getCount() > 0)
            coreType = value.getKeyList()[0].getCoreType();

        *type = coreType;
        return OPENDAQ_SUCCESS;
    }
        
    ErrCode INTERFACE_FUNC getItemType(CoreType* type) override
    {
        return getItemTypeInternal(type, true);    
    }

    ErrCode INTERFACE_FUNC getItemTypeNoLock(CoreType* type) override
    {
        return getItemTypeInternal(type, false);    
    }

    ErrCode INTERFACE_FUNC getItemTypeInternal(CoreType* type, bool lock)
    {
        OPENDAQ_PARAM_NOT_NULL(type);

        try
        {
            IntfID intfID = IUnknown::Id;
            *type = ctUndefined;

            BaseObjectPtr defVal;
            auto err = lock ? this->getDefaultValue(&defVal) : this->getDefaultValueNoLock(&defVal);
            if (OPENDAQ_FAILED(err))
                return err;

            BaseObjectPtr selVal;
            err = lock ? this->getSelectionValues(&selVal) : this->getSelectionValuesNoLock(&selVal);
            if (OPENDAQ_FAILED(err))
                return err;

            BaseObjectPtr value = defVal.assigned() ? defVal : nullptr;
            value = selVal.assigned() ? selVal : value;
            if (!value.assigned())
                return err;

            const auto dictElementType = value.asPtrOrNull<IDictElementType>();
            if (dictElementType.assigned())
                err = dictElementType->getValueInterfaceId(&intfID);

            const auto listElementType = value.asPtrOrNull<IListElementType>();
            if (listElementType.assigned())
                err = listElementType->getElementInterfaceId(&intfID);

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
        }
        catch (const DaqException& e)
        {
            return errorFromException(e);
        }
        catch (const std::exception& e)
        {
            return makeErrorInfo(OPENDAQ_ERR_GENERALERROR, e.what());
        }
        catch (...)
        {
            return OPENDAQ_ERR_GENERALERROR;
        }
    }

    ErrCode INTERFACE_FUNC getName(IString** name) override
    {
        if (name == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        *name = this->name.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getDescription(IString** description) override
    {
	    return getDescriptionInternal(description, true);
    }

    ErrCode INTERFACE_FUNC getDescriptionNoLock(IString** description) override
    {
	    return getDescriptionInternal(description, false);
    }

    ErrCode getDescriptionInternal(IString** description, bool lock)
    {
        OPENDAQ_PARAM_NOT_NULL(description);

	    return daqTry([&]()
            {
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        *description = lock ? prop.getDescription().detach() : prop.asPtr<IPropertyInternal>().getDescriptionNoLock().detach();
		        else
			        *description = bindAndGet<StringPtr>(this->description, lock).detach();
			        
		        return OPENDAQ_SUCCESS;
	        });
    }

    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override
    {
	    return getUnitInternal(unit, true);
    }

    ErrCode INTERFACE_FUNC getUnitNoLock(IUnit** unit) override
    {
	    return getUnitInternal(unit, false);
    }

    ErrCode getUnitInternal(IUnit** unit, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(unit);

	    return daqTry([&]()
            {
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        *unit = lock ? prop.getUnit().detach() : prop.asPtr<IPropertyInternal>().getUnitNoLock().detach();
		        else
			        *unit = bindAndGet<UnitPtr>(this->unit, lock).detach();
			        
		        return OPENDAQ_SUCCESS;
	        });
    }

    ErrCode INTERFACE_FUNC getMinValue(INumber** min) override
    {
	    return getMinValueInternal(min, true);
    }

    ErrCode INTERFACE_FUNC getMinValueNoLock(INumber** min) override
    {
	    return getMinValueInternal(min, false);
    }

    ErrCode getMinValueInternal(INumber** min, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(min);

	    return daqTry([&]()
            {
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        *min = lock ? prop.getMinValue().detach() : prop.asPtr<IPropertyInternal>().getMinValueNoLock().detach();
		        else
			        *min = bindAndGet<NumberPtr>(this->minValue, lock).detach();
			        
		        return OPENDAQ_SUCCESS;
	        });
    }

    ErrCode INTERFACE_FUNC getMaxValue(INumber** max) override
    {
	    return getMaxValueInternal(max, true);
    }

    ErrCode INTERFACE_FUNC getMaxValueNoLock(INumber** max) override
    {
	    return getMaxValueInternal(max, false);
    }

    ErrCode getMaxValueInternal(INumber** max, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(max);

	    return daqTry([&]()
            {
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        *max = lock ? prop.getMaxValue().detach() : prop.asPtr<IPropertyInternal>().getMaxValueNoLock().detach();
		        else
			        *max = bindAndGet<NumberPtr>(this->maxValue, lock).detach();
			        
		        return OPENDAQ_SUCCESS;
	        });
    }
    
    ErrCode INTERFACE_FUNC getDefaultValue(IBaseObject** value) override
    {
	    return getDefaultValueInternal(value, true);
    }

    ErrCode INTERFACE_FUNC getDefaultValueNoLock(IBaseObject** value) override
    {
	    return getDefaultValueInternal(value, false);
    }

    ErrCode getDefaultValueInternal(IBaseObject** value, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(value);

	    return daqTry([&]()
            {
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        *value = lock ? prop.getDefaultValue().detach() : prop.asPtr<IPropertyInternal>().getDefaultValueNoLock().detach();
		        else
			        *value = bindAndGet<BaseObjectPtr>(this->defaultValue, lock).detach();
			        
		        return OPENDAQ_SUCCESS;
	        });
    }
        
    ErrCode INTERFACE_FUNC getSuggestedValues(IList** values) override
    {
	    return getSuggestedValuesInternal(values, true);
    }

    ErrCode INTERFACE_FUNC getSuggestedValuesNoLock(IList** values) override
    {
	    return getSuggestedValuesInternal(values, false);
    }

    ErrCode getSuggestedValuesInternal(IList** values, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(values);

	    return daqTry([&]()
            {
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        *values = lock ? prop.getSuggestedValues().detach() : prop.asPtr<IPropertyInternal>().getSuggestedValuesNoLock().detach();
		        else
			        *values = bindAndGet<ListPtr<IBaseObject>>(this->suggestedValues, lock).detach();
			        
		        return OPENDAQ_SUCCESS;
	        });
    }
            
    ErrCode INTERFACE_FUNC getVisible(Bool* visible) override
    {
	    return getVisibleInternal(visible, true);
    }

    ErrCode INTERFACE_FUNC getVisibleNoLock(Bool* visible) override
    {
	    return getVisibleInternal(visible, false);
    }

    ErrCode getVisibleInternal(Bool* visible, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(visible);

	    return daqTry([&]()
            {
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        *visible = lock ? prop.getVisible() : prop.asPtr<IPropertyInternal>().getVisibleNoLock();
		        else
			        *visible = bindAndGet<BooleanPtr>(this->visible, lock);
			        
		        return OPENDAQ_SUCCESS;
	        });
    }
    
    ErrCode INTERFACE_FUNC getReadOnly(Bool* readOnly) override
    {
	    return getReadOnlyInternal(readOnly, true);
    }

    ErrCode INTERFACE_FUNC getReadOnlyNoLock(Bool* readOnly) override
    {
	    return getReadOnlyInternal(readOnly, false);
    }

    ErrCode getReadOnlyInternal(Bool* readOnly, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(readOnly);

	    return daqTry([&]()
            {
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        *readOnly = lock ? prop.getReadOnly() : prop.asPtr<IPropertyInternal>().getReadOnlyNoLock();
		        else
			        *readOnly = bindAndGet<BooleanPtr>(this->readOnly, lock);
			        
		        return OPENDAQ_SUCCESS;
	        });
    }

    ErrCode INTERFACE_FUNC getSelectionValues(IBaseObject** values) override
    {
	    return getSelectionValuesInternal(values, true);
    }

    ErrCode INTERFACE_FUNC getSelectionValuesNoLock(IBaseObject** values) override
    {
	    return getSelectionValuesInternal(values, false);
    }

    ErrCode getSelectionValuesInternal(IBaseObject** values, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(values);

	    return daqTry([&]()
            {
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        *values = lock ? prop.getSelectionValues().detach() : prop.asPtr<IPropertyInternal>().getSelectionValuesNoLock().detach();
		        else
			        *values = bindAndGet<BaseObjectPtr>(this->selectionValues, lock).detach();
			        
		        return OPENDAQ_SUCCESS;
	        });
    }
    
    ErrCode INTERFACE_FUNC getReferencedProperty(IProperty** property) override
    {
	    return getReferencedPropertyInternal(property, true);
    }

    ErrCode INTERFACE_FUNC getReferencedPropertyNoLock(IProperty** property) override
    {
	    return getReferencedPropertyInternal(property, false);
    }

    ErrCode getReferencedPropertyInternal(IProperty** property, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(property);

	    return daqTry([&]()
            {
	            *property = bindAndGet<PropertyPtr>(this->refProp, lock).detach();
		        return OPENDAQ_SUCCESS;
	        });
    }

    ErrCode INTERFACE_FUNC getIsReferenced(Bool* isReferenced) override
    {
	    return getIsReferencedInternal(isReferenced, true);
    }

    ErrCode INTERFACE_FUNC getIsReferencedNoLock(Bool* isReferenced) override
    {
	    return getIsReferencedInternal(isReferenced, false);
    }

    ErrCode getIsReferencedInternal(Bool* isReferenced, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(isReferenced);

        return daqTry([&]() {
            *isReferenced = false;
	        const auto ownerPtr = owner.assigned() ? owner.getRef() : nullptr;

            if (owner.assigned())
            {
                const auto ownerInternal = owner.getRef().asPtr<IPropertyObjectInternal>();
                *isReferenced = lock ? ownerInternal.checkForReferences(propPtr) : ownerInternal.checkForReferencesNoLock(propPtr);
            }

            return OPENDAQ_SUCCESS;
        });
    }
        
    ErrCode INTERFACE_FUNC getValidator(IValidator** validator) override
    {
	    return getValidatorInternal(validator, true);
    }

    ErrCode INTERFACE_FUNC getValidatorNoLock(IValidator** validator) override
    {
	    return getValidatorInternal(validator, false);
    }

    ErrCode getValidatorInternal(IValidator** validator, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(validator);

	    return daqTry([&]()
            {
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        *validator = lock ? prop.getValidator().detach() : prop.asPtr<IPropertyInternal>().getValidatorNoLock().detach();
		        else
			        *validator = this->validator.addRefAndReturn();
			        
		        return OPENDAQ_SUCCESS;
	        });
    }
            
    ErrCode INTERFACE_FUNC getCoercer(ICoercer** coercer) override
    {
	    return getCoercerInternal(coercer, true);
    }

    ErrCode INTERFACE_FUNC getCoercerNoLock(ICoercer** coercer) override
    {
	    return getCoercerInternal(coercer, false);
    }

    ErrCode getCoercerInternal(ICoercer** coercer, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(coercer);

	    return daqTry([&]()
            {
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        *coercer = lock ? prop.getCoercer().detach() : prop.asPtr<IPropertyInternal>().getCoercerNoLock().detach();
		        else
			        *coercer = this->coercer.addRefAndReturn();
			        
		        return OPENDAQ_SUCCESS;
	        });
    }
           
    ErrCode INTERFACE_FUNC getCallableInfo(ICallableInfo** callableInfo) override
    {
	    return getCallableInfoInternal(callableInfo, true);
    }

    ErrCode INTERFACE_FUNC getCallableInfoNoLock(ICallableInfo** callableInfo) override
    {
	    return getCallableInfoInternal(callableInfo, false);
    }

    ErrCode getCallableInfoInternal(ICallableInfo** callableInfo, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(callableInfo);

	    return daqTry([&]()
            {
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        *callableInfo = lock ? prop.getCallableInfo().detach() : prop.asPtr<IPropertyInternal>().getCallableInfoNoLock().detach();
		        else
			        *callableInfo = this->callableInfo.addRefAndReturn();
			        
		        return OPENDAQ_SUCCESS;
	        });
    }

    ErrCode INTERFACE_FUNC getStructType(IStructType** structType) override
    {
	    return getStructTypeInternal(structType, true);
    }

    ErrCode INTERFACE_FUNC getStructTypeNoLock(IStructType** structType) override
    {
	    return getStructTypeInternal(structType, false);
    }

    ErrCode getStructTypeInternal(IStructType** structType, bool lock)
    {
	    OPENDAQ_PARAM_NOT_NULL(structType);

	    return daqTry([&]()
            {
                BaseObjectPtr defaultStruct;
		        if (const PropertyPtr prop = bindAndGetRefProp(lock); prop.assigned())
			        defaultStruct = lock ? prop.getDefaultValue().detach() : prop.asPtr<IPropertyInternal>().getDefaultValueNoLock().detach();
                else if (lock)
                    checkErrorInfo(this->getDefaultValue(&defaultStruct));
                else
                    checkErrorInfo(this->getDefaultValueNoLock(&defaultStruct));

                *structType = defaultStruct.asPtr<IStruct>().getStructType().detach();
		        return OPENDAQ_SUCCESS;
	        });
    }

    ErrCode INTERFACE_FUNC getClassOnPropertyValueWrite(IEvent** event) override
    {
        if (event == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return the event via a null pointer.");
        }

        *event = onValueWrite.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IEvent** event) override
    {
        if (event == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return the event via a null pointer.");
        }
        const auto ownerPtr = owner.assigned() ? owner.getRef() : nullptr;
        if (ownerPtr.assigned())
        {
            return ownerPtr->getOnPropertyValueWrite(this->name, event);
        }

        *event = onValueWrite.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getClassOnPropertyValueRead(IEvent** event) override
    {
        if (event == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return the event via a null pointer.");
        }

        *event = onValueRead.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getOnPropertyValueRead(IEvent** event) override
    {
        if (event == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return the event via a null pointer.");
        }

        const auto ownerPtr = owner.assigned() ? owner.getRef() : nullptr;
        if (ownerPtr.assigned())
        {
            return ownerPtr->getOnPropertyValueRead(this->name, event);
        }

        *event = onValueRead.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getValue(IBaseObject** value) override
    {
        if (!owner.assigned() || !owner.getRef().assigned())
            return OPENDAQ_ERR_NO_OWNER;

        return owner.getRef()->getPropertyValue(this->name, value);
    }

    ErrCode INTERFACE_FUNC setValue(IBaseObject* value) override
    {
        if (!owner.assigned() || !owner.getRef().assigned())
            return OPENDAQ_ERR_NO_OWNER;

        return owner.getRef()->setPropertyValue(this->name, value);
    }

    ErrCode INTERFACE_FUNC validate()
    {
        if (!name.assigned() || name == "opendaq_unassigned")
        {
            name = "opendaq_unassigned";
            return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Property name is not assigned");
        }

        if (valueType == ctFunc || valueType == ctProc)
        {
            if (defaultValue.assigned())
            {
                return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE,
                                           fmt::format(R"(Function/procedure property "{}" cannot have a default value)", name));
            }
        }
        else if (refProp.assigned())
        {
            if (defaultValue.assigned())
            {
                return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE,
                                           fmt::format(R"(Reference property {} cannot have default values)", name));
            }
        }
        else if (!defaultValue.assigned())
        {
            return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, fmt::format(R"(Property {} is missing its default value)", name));
        }

        if (defaultValue.assigned())
        {
            if (const auto freezable = defaultValue.asPtrOrNull<IFreezable>(); freezable.assigned())
            {
                const ErrCode err = freezable->freeze();
                if (OPENDAQ_FAILED(err))
                    return err;
            }
        }

        if (valueType == ctObject)
        {
            bool valid = !selectionValues.assigned() && !suggestedValues.assigned();
            valid = valid && !coercer.assigned() && !validator.assigned();
            valid = valid && !unit.assigned();

            if (!valid)
                return this->makeErrorInfo(
                    OPENDAQ_ERR_INVALIDSTATE,
                    fmt::format(R"(Object-type property {} can only have its name, description, read-only, visible, and default value configured)", name));
        }

        if (minValue.assigned() || maxValue.assigned())
        {
            if (valueType != ctInt && valueType != ctFloat)
                return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE,
                                           fmt::format(R"({}: Min/max can only be configured on Int, Float, and Ratio properties)", name));
        }

        if (callableInfo.assigned())
        {
            if (!(valueType == ctProc || valueType == ctFunc))
                return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE,
                                           fmt::format(R"({}: Callable info can be configured only on function- and procedure-type
                                           properties.)", name));
        }

        if (refProp.assigned())
        {
            bool valid = valueType == ctUndefined;
            valid = valid && !description.assigned() || description == "";
            valid = valid && !readOnly;
            valid = valid && !selectionValues.assigned() && !suggestedValues.assigned();
            valid = valid && !coercer.assigned() && !validator.assigned();

            if (!valid)
                return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, fmt::format(R"(Reference property {} has invalid metadata.)", name));
        }

        if (selectionValues.assigned())
        {
            bool valid = valueType == ctInt;
            valid = valid && (selectionValues.supportsInterface<IList>() || selectionValues.supportsInterface<IDict>());
            if (!valid)
                return this->makeErrorInfo(
                    OPENDAQ_ERR_INVALIDSTATE,
                    fmt::format(
                        R"(Selection property {} must have the value type ctInt, and the selection values must be a list or dictionary)",
                        name));
        }

        if (suggestedValues.assigned() && (valueType != ctInt && valueType != ctFloat))
        {
            return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE,
                                       fmt::format(R"({}: Only numerical properties can have a list of suggested values)", name));
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
                return this->makeErrorInfo(
                    OPENDAQ_ERR_INVALIDSTATE,
                    fmt::format(R"(Container type property {} cannot have keys/items that are object-types.)", name));

            if (itemType == ctFunc || keyType == ctFunc || itemType == ctProc || keyType == ctProc)
                return this->makeErrorInfo(
                    OPENDAQ_ERR_INVALIDSTATE,
                    fmt::format(R"(Container type property {} cannot have keys/items that are function-types.)", name));

            if (itemType == ctList || keyType == ctList || itemType == ctDict || keyType == ctDict)
                return this->makeErrorInfo(
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
                return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, fmt::format(R"(Structure property {} has invalid metadata.)", name));
        }

        if (valueType == ctEnumeration)
        {
            bool valid = !selectionValues.assigned() && !suggestedValues.assigned();
            valid = valid && !coercer.assigned() && !validator.assigned();
            valid = valid && !maxValue.assigned() && !minValue.assigned();
            valid = valid && !unit.assigned() && !callableInfo.assigned();

            if (!valid)
                return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, fmt::format(R"(Enumeration property {} has invalid metadata.)", name));
        }

        // TODO: Make callable info serializable
        // if ((valueType == ctProc || valueType == ctFunc) && !callableInfo.assigned())
        //{
        //    return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, fmt::format(R"(Function- and procedure-type property {} must have
        //    Callable info configured)", name));
        //}

        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC toString(CharPtr* str) override
    {
        if (str == nullptr)
        {
            return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Parameter must not be null");
        }

        std::ostringstream stream;
        stream << "Property {" << name << "}";
        return daqDuplicateCharPtr(stream.str().c_str(), str);
    }

    //
    // ISerializable
    //

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override
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
            SERIALIZE_PROP_PTR(selectionValues)
            SERIALIZE_PROP_PTR(coercer)
            SERIALIZE_PROP_PTR(validator)
            SERIALIZE_PROP_PTR(suggestedValues)
            SERIALIZE_PROP_PTR(callableInfo)
        }
        serializer->endObject();

        return OPENDAQ_SUCCESS;
    }

    static ConstCharPtr SerializeId()
    {
        return "Property";
    }

    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override
    {
        *id = SerializeId();
        return OPENDAQ_SUCCESS;
    }

    static ErrCode Deserialize(ISerializedObject* serializedObj, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
    {
        StringPtr name;
        ErrCode errCode = serializedObj->readString(String("name"), &name);
        if (errCode != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        const auto propObj = PropertyBuilder(name);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        errCode = deserializeMember<decltype(valueType)>(serializedObj, "valueType", propObj, context, factoryCallback, &IPropertyBuilder::setValueType);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        DESERIALIZE_MEMBER(context, factoryCallback, description, setDescription)

        BaseObjectPtr unit;
        errCode = serializedObj->readObject(String("unit"), context, factoryCallback, &unit);
        if (errCode != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
        if (errCode != OPENDAQ_ERR_NOTFOUND)
        {
            propObj->setUnit(unit.asPtr<IUnit>());
        }

        DESERIALIZE_MEMBER(context, factoryCallback, defaultValue, setDefaultValue)

        BaseObjectPtr refProp;
        errCode = serializedObj->readObject(String("refProp"), context, factoryCallback, &refProp);
        if (errCode != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
        if (errCode != OPENDAQ_ERR_NOTFOUND)
        {
            propObj->setReferencedProperty(refProp.asPtr<IEvalValue>());
        }

        DESERIALIZE_MEMBER(context, factoryCallback, selectionValues, setSelectionValues)

        BaseObjectPtr suggestedValues;
        errCode = serializedObj->readObject(String("suggestedValues"), context, factoryCallback, &suggestedValues);
        if (errCode != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
        if (errCode != OPENDAQ_ERR_NOTFOUND)
        {
            propObj->setSuggestedValues(suggestedValues.asPtr<IList>());
        }

        BaseObjectPtr visible;
        errCode = serializedObj->readObject(String("visible"), context, factoryCallback, &visible);
        if (errCode != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
        if (errCode != OPENDAQ_ERR_NOTFOUND)
        {
            propObj->setVisible(visible.asPtr<IBoolean>());
        }

        BaseObjectPtr readOnly;
        errCode = serializedObj->readObject(String("readOnly"), context, factoryCallback, &readOnly);
        if (errCode != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
        if (errCode != OPENDAQ_ERR_NOTFOUND)
        {
            propObj->setReadOnly(readOnly.asPtr<IBoolean>());
        }

        BaseObjectPtr minValue;
        errCode = serializedObj->readObject(String("minValue"), context, factoryCallback, &minValue);
        if (errCode != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
        if (errCode != OPENDAQ_ERR_NOTFOUND)
        {
            propObj->setMinValue(minValue.asPtr<INumber>());
        }

        BaseObjectPtr maxValue;
        errCode = serializedObj->readObject(String("maxValue"), context, factoryCallback, &maxValue);
        if (errCode != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
        if (errCode != OPENDAQ_ERR_NOTFOUND)
        {
            propObj->setMaxValue(maxValue.asPtr<INumber>());
        }

        BaseObjectPtr coercer;
        errCode = serializedObj->readObject(String("coercer"), context, factoryCallback, &coercer);
        if (errCode != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
        if (errCode != OPENDAQ_ERR_NOTFOUND)
        {
            propObj->setCoercer(coercer.asPtr<ICoercer>());
        }

        BaseObjectPtr validator;
        errCode = serializedObj->readObject(String("validator"), context, factoryCallback, &validator);
        if (errCode != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
        if (errCode != OPENDAQ_ERR_NOTFOUND)
        {
            propObj->setValidator(validator.asPtr<IValidator>());
        }

        BaseObjectPtr callableInfo;
        errCode = serializedObj->readObject(String("callableInfo"), context, factoryCallback, &callableInfo);
        if (errCode != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
        if (errCode != OPENDAQ_ERR_NOTFOUND)
        {
            propObj->setCallableInfo(callableInfo.asPtr<ICallableInfo>());
        }

        *obj = propObj.build().detach();
        return OPENDAQ_SUCCESS;
    }

    //
    // IPropertyInternal
    //
    
    ErrCode INTERFACE_FUNC clone(IProperty** clonedProperty) override
    {
        if (clonedProperty == nullptr)
        {
            return OPENDAQ_ERR_ARGUMENT_NULL;
        }

        return daqTry([&]
        {
            auto defaultValueObj = defaultValue;

            if (defaultValueObj.assigned())
            {
                auto cloneableDefaultValue = defaultValue.asPtrOrNull<IPropertyObjectInternal>();
                if (cloneableDefaultValue.assigned())
                    defaultValueObj = cloneableDefaultValue.clone();
            }

            auto prop = PropertyBuilder(name)
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
                        .setOnPropertyValueWrite(onValueWrite).build();

            *clonedProperty = prop.detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC cloneWithOwner(IPropertyObject* owner, IProperty** clonedProperty) override
    {
        if (clonedProperty == nullptr)
        {
            return OPENDAQ_ERR_ARGUMENT_NULL;
        }

        if (this->owner.assigned() && owner == this->owner.getRef())
        {
            this->addRef();
            *clonedProperty = this;
            return OPENDAQ_SUCCESS;
        }

        PropertyPtr prop;
        ErrCode err = clone(&prop);
        if (OPENDAQ_FAILED(err))
        {
            return err;
        }

        return daqTry([&] {
            prop.asPtr<IOwnable>().setOwner(owner);

            *clonedProperty = prop.detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getDescriptionUnresolved(IString** description) override
    {
        if (description == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            StringPtr descriptionPtr = getUnresolved(this->description);
            *description = descriptionPtr.detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getUnitUnresolved(IBaseObject** unit) override
    {
        if (unit == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            BaseObjectPtr unitPtr = getUnresolved(this->unit);
            *unit = unitPtr.detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getMinValueUnresolved(INumber** min) override
    {
        if (min == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        if (!this->minValue.assigned())
        {
            *min = nullptr;
            return OPENDAQ_SUCCESS;
        }

        return daqTry([&]() {
            NumberPtr minPtr = getUnresolved(this->minValue);
            *min = minPtr.detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getMaxValueUnresolved(INumber** max) override
    {
        if (max == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            NumberPtr maxPtr = getUnresolved(this->maxValue);
            *max = maxPtr.detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getDefaultValueUnresolved(IBaseObject** value) override
    {
        if (value == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            BaseObjectPtr defaultValuePtr = getUnresolved(this->defaultValue);
            *value = defaultValuePtr.detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getSuggestedValuesUnresolved(IList** values) override
    {
        if (values == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            ListPtr<IBaseObject> suggestedValuesPtr = getUnresolved(this->suggestedValues);
            *values = suggestedValuesPtr.detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getVisibleUnresolved(IBoolean** visible) override
    {
        if (visible == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            BoolPtr visiblePtr = getUnresolved(this->visible);
            *visible = visiblePtr.detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getReadOnlyUnresolved(IBoolean** readOnly) override
    {
        if (readOnly == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            BoolPtr readOnlyPtr = getUnresolved(this->readOnly);
            *readOnly = readOnlyPtr.detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getSelectionValuesUnresolved(IBaseObject** values) override
    {
        if (values == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            BaseObjectPtr selectionValuesPtr = getUnresolved(this->selectionValues);
            *values = selectionValuesPtr.detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getReferencedPropertyUnresolved(IEvalValue** propertyEval) override
    {
        if (propertyEval == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            EvalValuePtr propertyEvalPtr = getUnresolved(this->refProp);
            *propertyEval = propertyEvalPtr.detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getValueTypeUnresolved(CoreType* coreType) override
    {
        if (coreType == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        *coreType = this->valueType;
        return OPENDAQ_SUCCESS;
    }

    //
    // IOwnable
    //

    ErrCode INTERFACE_FUNC setOwner(IPropertyObject* owner) override
    {
        if (this->owner.assigned())
        {
            return makeErrorInfo(OPENDAQ_ERR_ALREADYEXISTS, "Owner is already assigned.");
        }

        this->owner = owner;

        if (this->defaultValue.assigned())
        {
            const auto parentManager = this->owner.getRef().getPermissionManager();
            const auto defaultValueObj = this->defaultValue.asPtrOrNull<IPropertyObject>();

            if (defaultValueObj.assigned())
                defaultValueObj.getPermissionManager().asPtr<IPermissionManagerInternal>().setParent(parentManager);
        }

        return OPENDAQ_SUCCESS;
    }

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
    PermissionManagerPtr defaultPermissionManager;

private:

    PropertyPtr bindAndGetRefProp(bool lock)
    {
	    PropertyPtr refPropPtr;
	    checkErrorInfo(getReferencedPropertyInternal(&refPropPtr, lock));
	    if (!refPropPtr.assigned())
		    return nullptr;
		    
	    return refPropPtr;
    }

    template <typename TPtr>
    TPtr bindAndGet(const BaseObjectPtr& metadata, bool lock) const
    {
	    if (!metadata.assigned())
		    return nullptr;
		    
	    auto eval = metadata.asPtrOrNull<IEvalValue>();
	    if (!eval.assigned())
		    return metadata;

	    const auto ownerPtr = owner.assigned() ? owner.getRef() : nullptr;
	    if (ownerPtr.assigned())
		    eval = eval.cloneWithOwner(ownerPtr);

	    return lock ? eval.getResult() : eval.getResultNoLock();
    }

    BaseObjectPtr getUnresolved(const BaseObjectPtr& localMetadata) const
    {
        if (!localMetadata.assigned())
            return nullptr;

        if (const auto eval = localMetadata.asPtrOrNull<IEvalValue>(); eval.assigned())
        {
            const auto ownerPtr = owner.assigned() ? owner.getRef() : nullptr;
            if (ownerPtr.assigned())
                return eval.cloneWithOwner(ownerPtr);
        }

        return localMetadata;
    }

    ErrCode validateDuringConstruction()
    {
        this->internalAddRefNoCheck();
        const ErrCode err = validate();
        this->internalReleaseRef();
        return err;
    }
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(PropertyImpl)

END_NAMESPACE_OPENDAQ
