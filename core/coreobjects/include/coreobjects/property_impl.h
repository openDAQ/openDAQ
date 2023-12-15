/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <iostream>

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
        if (valueType == ctBinaryData)
        {
            throw InvalidTypeException{"Properties cannot be BinaryData types"};
        }

        propPtr = this->borrowPtr<PropertyPtr>();
    }

public:
    explicit PropertyImpl(const StringPtr& name)
        : PropertyImpl()
    {
        this->name = name;
    }

    PropertyImpl(IPropertyBuilder* propertyBuilder)
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

    // SparseSelectionProperty()
    PropertyImpl(const StringPtr& name, IStruct* defaultValue, const BooleanPtr& visible)
        : PropertyImpl(name, BaseObjectPtr(defaultValue), visible)
    {
        this->valueType = ctStruct;
        this->selectionValues = BaseObjectPtr(selectionValues);

        const auto err = validateDuringConstruction();
        if (err != OPENDAQ_SUCCESS)
            throwExceptionFromErrorCode(err);
    }

    ErrCode INTERFACE_FUNC getValueType(CoreType* type) override
    {
        if (type == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            bool bound = false;
            const auto prop = bindAndGetRefProp(bound);
            if (bound)
            {
                *type = prop.getValueType();
            }
            else
            {
                *type = this->valueType;
            }
            return OPENDAQ_SUCCESS;
        });
    }
    
    ErrCode INTERFACE_FUNC getKeyType(CoreType* type) override
    {
        if (type == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        *type = ctUndefined;
        BaseObjectPtr defVal;
        ErrCode err = this->getDefaultValue(&defVal);
        if (OPENDAQ_FAILED(err))
        {
            return err;
        }

        if (!defVal.assigned())
        {
            return OPENDAQ_SUCCESS;
        }

        DictPtr<IBaseObject, IBaseObject> value = defVal.asPtrOrNull<IDict>();
        if (!value.assigned())
        {
            return OPENDAQ_SUCCESS;
        }

        IntfID intfID;
        err = value.asPtr<IDictElementType>()->getKeyInterfaceId(&intfID);
        if (OPENDAQ_FAILED(err))
        {
            return err;
        }

        auto coreType = details::intfIdToCoreType(intfID);

        // TODO: Workaround if item type of dict/list is undefined
        if (coreType == ctUndefined && value.getCount() > 0)
        {
            coreType = value.getKeyList()[0].getCoreType();
        }

        *type = coreType;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getItemType(CoreType* type) override
    {
        if (type == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        try
        {
            IntfID intfID = IUnknown::Id;
            *type = ctUndefined;

            BaseObjectPtr defVal;
            ErrCode err = this->getDefaultValue(&defVal);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            BaseObjectPtr selVal;
            err = this->getSelectionValues(&selVal);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            BaseObjectPtr value = defVal.assigned() ? defVal : nullptr;
            value = selVal.assigned() ? selVal : value;
            if (!value.assigned())
            {
                return err;
            }

            const auto dictElementType = value.asPtrOrNull<IDictElementType>();
            if (dictElementType.assigned())
            {
                err = dictElementType->getValueInterfaceId(&intfID);
            }

            const auto listElementType = value.asPtrOrNull<IListElementType>();
            if (listElementType.assigned())
            {
                err = listElementType->getElementInterfaceId(&intfID);
            }

            auto coreType = details::intfIdToCoreType(intfID);

            // TODO: Workaround if item type of dict/list is undefined
            if (coreType == ctUndefined)
            {
                ListPtr<IBaseObject> asList = value.asPtrOrNull<IList>();
                DictPtr<IBaseObject, IBaseObject> asDict = value.asPtrOrNull<IDict>();
                if (asList.assigned() && asList.getCount() > 0)
                {
                    coreType = asList[0].getCoreType();
                    err = OPENDAQ_SUCCESS;
                }
                else if (asDict.assigned() && asDict.getCount() > 0)
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
        if (description == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            bool bound = false;
            const auto prop = bindAndGetRefProp(bound);
            if (bound)
            {
                *description = prop.getDescription().detach();
            }
            else
            {
                StringPtr descriptionBound = bindAndGet(this->description);
                *description = descriptionBound.detach();
            }
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override
    {
        if (unit == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            bool bound = false;
            const auto prop = bindAndGetRefProp(bound);
            if (bound)
            {
                *unit = prop.getUnit().detach();
            }
            else
            {
                UnitPtr unitBound = bindAndGet(this->unit);
                *unit = unitBound.detach();
            }
            return OPENDAQ_SUCCESS;
        });
    }
    
    ErrCode INTERFACE_FUNC getMinValue(INumber** min) override
    {
        if (min == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            bool bound = false;
            const auto prop = bindAndGetRefProp(bound);
            if (bound)
            {
                *min = prop.getMinValue().detach();
            }
            else
            {
                NumberPtr minBound = bindAndGet(this->minValue);
                *min = minBound.detach();
            }
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getMaxValue(INumber** max) override
    {
        if (max == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            bool bound = false;
            const auto prop = bindAndGetRefProp(bound);
            if (bound)
            {
                *max = prop.getMaxValue().detach();
            }
            else
            {
                NumberPtr maxBound = bindAndGet(this->maxValue);
                *max = maxBound.detach();
            }
            return OPENDAQ_SUCCESS;
        });
    }
    
    ErrCode INTERFACE_FUNC getDefaultValue(IBaseObject** value) override
    {
        if (value == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            bool bound = false;
            const auto prop = bindAndGetRefProp(bound);
            if (bound)
            {
                *value = prop.getDefaultValue().detach();
            }
            else
            {
                BaseObjectPtr defaultValueBound = bindAndGet(this->defaultValue);
                *value = defaultValueBound.detach();
            }
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getSuggestedValues(IList** values) override
    {
        if (values == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            bool bound = false;
            const auto prop = bindAndGetRefProp(bound);
            if (bound)
            {
                *values = prop.getSuggestedValues().detach();
            }
            else
            {
                ListPtr<IBaseObject> suggestedValuesBound = bindAndGet(this->suggestedValues);
                *values = suggestedValuesBound.detach();
            }
            return OPENDAQ_SUCCESS;
        });
    }
    
    ErrCode INTERFACE_FUNC getVisible(Bool* visible) override
    {
        if (visible == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            bool bound = false;
            const auto prop = bindAndGetRefProp(bound);
            if (bound)
            {
                *visible = prop.getVisible();
            }
            else
            {
                *visible = bindAndGet(this->visible);
            }
            return OPENDAQ_SUCCESS;
        });
    }
    
    ErrCode INTERFACE_FUNC getReadOnly(Bool* readOnly) override
    {
        if (readOnly == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            bool bound = false;
            const auto prop = bindAndGetRefProp(bound);
            if (bound)
            {
                *readOnly = prop.getReadOnly();
            }
            else
            {
                *readOnly = bindAndGet(this->readOnly);
            }
            return OPENDAQ_SUCCESS;
        });
    }
    
    ErrCode INTERFACE_FUNC getSelectionValues(IBaseObject** values) override
    {
        if (values == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            bool bound = false;
            const auto prop = bindAndGetRefProp(bound);
            if (bound)
            {
                *values = prop.getSelectionValues().detach();
            }
            else
            {
                *values = bindAndGet(this->selectionValues).detach();
            }
            return OPENDAQ_SUCCESS;
        });
    }
    
    ErrCode INTERFACE_FUNC getReferencedProperty(IProperty** property) override
    {
        if (property == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            PropertyPtr prop = bindAndGet(this->refProp);
            *property = prop.detach();
            return OPENDAQ_SUCCESS;
        });
    }
    
    ErrCode INTERFACE_FUNC getIsReferenced(Bool* isReferenced) override
    {
        if (isReferenced == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            *isReferenced = false;
            if (owner.assigned())
            {
                const auto ownerInternal = owner.getRef().asPtr<IPropertyObjectInternal>();
                *isReferenced = ownerInternal.checkForReferences(propPtr);
            }

            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getValidator(IValidator** validator) override
    {
        if (validator == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            bool bound = false;
            const auto prop = bindAndGetRefProp(bound);
            if (bound)
            {
                *validator = prop.getValidator().detach();
            }
            else
            {
                *validator = this->validator.addRefAndReturn();
            }
            return OPENDAQ_SUCCESS;
        });
    }
    
    ErrCode INTERFACE_FUNC getCoercer(ICoercer** coercer) override
    {
        if (coercer == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            bool bound = false;
            const auto prop = bindAndGetRefProp(bound);
            if (bound)
            {
                *coercer = prop.getCoercer().detach();
            }
            else
            {
                *coercer = this->coercer.addRefAndReturn();
            }
            return OPENDAQ_SUCCESS;
        });
    }
    
    ErrCode INTERFACE_FUNC getCallableInfo(ICallableInfo** callable) override
    {
        if (callable == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

        return daqTry([&]() {
            bool bound = false;
            const auto prop = bindAndGetRefProp(bound);
            if (bound)
            {
                *callable = prop.getCallableInfo();
            }
            else
            {
                *callable = this->callableInfo.addRefAndReturn();
            }
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getStructType(IStructType** structType) override
    {
        if (structType == nullptr)
            return OPENDAQ_ERR_ARGUMENT_NULL;

       

        return daqTry(
            [&]()
            {
                bool bound = false;
                const auto prop = bindAndGetRefProp(bound);
                BaseObjectPtr defaultStruct;
                if (bound)
                {
                    defaultStruct = prop.getDefaultValue();
                }
                else
                {
                    checkErrorInfo(this->getDefaultValue(&defaultStruct));
                }

                *structType = defaultValue.asPtr<IStruct>().getStructType().detach();
                return OPENDAQ_SUCCESS;
            });
    }
    
    ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IEvent** event) override
    {
        if (event == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return the event via a null pointer.");
        }

        *event = onValueWrite.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC getOnPropertyValueRead(IEvent** event) override
    {
        if (event == nullptr)
        {
            return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return the event via a null pointer.");
        }

        *event = onValueRead.addRefAndReturn();
        return OPENDAQ_SUCCESS;
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
            Bool shouldFreeze = true;
            if (valueType == ctObject)
            {
                const ErrCode err = getReadOnly(&shouldFreeze);
                if (OPENDAQ_FAILED(err))
                    return err;
            }

            if (const auto freezable = defaultValue.asPtrOrNull<IFreezable>(); shouldFreeze && freezable.assigned())
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

        // TODO: Make callable info serializable
        // if (callableInfo.assigned())
        //{
        //    if (!(valueType == ctProc || valueType == ctFunc))
        //        return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE,
        //                                   fmt::format(R"({}: Callable info can be configured only on function- and procedure-type
        //                                   properties.)", name));
        //}

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
            valid = valid && (selectionValues.asPtrOrNull<IList>().assigned() || selectionValues.asPtrOrNull<IDict>().assigned());
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

            // TODO: Make callableInfo serializable
            // SERIALIZE_PROP_PTR(callableInfo)
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

        // TODO: Make callableInfo serializable

        // BaseObjectPtr callableInfo;
        // errCode = serializedObj->readObject(String("callableInfo"), context, factoryCallback, &callableInfo);
        // if (errCode != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(errCode))
        //{
        //    return errCode;
        //}
        // if (errCode != OPENDAQ_ERR_NOTFOUND)
        //{
        //    propObj->setCallableInfo(callableInfo.asPtr<ICallableInfo>());
        //}

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

        return daqTry([&]() {
            auto prop = PropertyBuilder(name)
                        .setValueType(valueType)
                        .setDescription(description)
                        .setUnit(unit)
                        .setMinValue(minValue)
                        .setMaxValue(maxValue)
                        .setDefaultValue(defaultValue)
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

private:
    PropertyPtr bindAndGetRefProp(bool& bound)
    {
        auto refPropPtr = propPtr.getReferencedProperty();
        if (!refPropPtr.assigned())
        {
            bound = false;
            return propPtr;
        }
        bound = true;
        return refPropPtr;
    }

    BaseObjectPtr bindAndGet(BaseObjectPtr metadata) const
    {
        if (!metadata.assigned())
        {
            return nullptr;
        }

        auto eval = metadata.asPtrOrNull<IEvalValue>();
        if (!eval.assigned())
        {
            return metadata;
        }

        const auto ownerPtr = owner.assigned() ? owner.getRef() : nullptr;
        if (ownerPtr.assigned())
        {
            eval = eval.cloneWithOwner(ownerPtr);
        }

        return eval.getResult();
    }

    BaseObjectPtr getUnresolved(BaseObjectPtr localMetadata) const
    {
        if (!localMetadata.assigned())
        {
            return nullptr;
        }

        auto eval = localMetadata.asPtrOrNull<IEvalValue>();
        if (eval.assigned())
        {
            const auto ownerPtr = owner.assigned() ? owner.getRef() : nullptr;
            if (ownerPtr.assigned())
                eval = eval.cloneWithOwner(ownerPtr);
            return eval;
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
