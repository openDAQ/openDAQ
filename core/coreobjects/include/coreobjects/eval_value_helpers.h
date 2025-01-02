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
#include <coretypes/coretypes.h>
#include <coreobjects/unit.h>

BEGIN_NAMESPACE_OPENDAQ

class IInteger_Helper : public IInteger
{
public:
    virtual ErrCode Integer_GetValue(Int* value) = 0;
    virtual ErrCode Integer_EqualsValue(Int value, Bool* equal) = 0;

    ErrCode INTERFACE_FUNC getValue(Int* value) override
    {
        return Integer_GetValue(value);
    }

    ErrCode INTERFACE_FUNC equalsValue(const Int value, Bool* equal) override
    {
        return Integer_EqualsValue(value, equal);
    }
};

class IFloat_Helper : public IFloat
{
public:
    virtual ErrCode Float_GetValue(Float* value) = 0;
    virtual ErrCode Float_EqualsValue(Float value, Bool* equal) = 0;

    ErrCode INTERFACE_FUNC getValue(Float* value) override
    {
        return Float_GetValue(value);
    }

    ErrCode INTERFACE_FUNC equalsValue(const Float value, Bool* equal) override
    {
        return Float_EqualsValue(value, equal);
    }
};

class IBoolean_Helper : public IBoolean
{
public:
    virtual ErrCode Boolean_GetValue(Bool* value) = 0;
    virtual ErrCode Boolean_EqualsValue(Bool value, Bool* equal) = 0;

    ErrCode INTERFACE_FUNC getValue(Bool* value) override
    {
        return Boolean_GetValue(value);
    }

    ErrCode INTERFACE_FUNC equalsValue(const Bool value, Bool* equal) override
    {
        return Boolean_EqualsValue(value, equal);
    }
};

class IString_Helper : public IString
{
public:
    virtual ErrCode StringObject_GetCharPtr(ConstCharPtr* value) = 0;
    virtual ErrCode StringObject_GetLength(SizeT* size) = 0;

    ErrCode INTERFACE_FUNC getCharPtr(ConstCharPtr* value) override
    {
        return StringObject_GetCharPtr(value);
    }

    ErrCode INTERFACE_FUNC getLength(SizeT* size) override
    {
        return StringObject_GetLength(size);
    }
};

class IUnit_Helper : public IUnit
{
public:
    virtual ErrCode UnitObject_GetId(Int* id) = 0;
    virtual ErrCode UnitObject_GetSymbol(IString** symbol) = 0;
    virtual ErrCode UnitObject_GetName(IString** name) = 0;
    virtual ErrCode UnitObject_GetQuantity(IString** quantity) = 0;

    ErrCode INTERFACE_FUNC getId(Int* id) override
    {
        return UnitObject_GetId(id);
    }

    ErrCode INTERFACE_FUNC getSymbol(IString** symbol) override
    {
        return UnitObject_GetSymbol(symbol);
    }

    ErrCode INTERFACE_FUNC getName(IString** name) override
    {
        return UnitObject_GetName(name);
    }

    ErrCode INTERFACE_FUNC getQuantity(IString** quantity) override
    {
        return UnitObject_GetQuantity(quantity);
    }
};

class IStruct_Helper : public IStruct
{
public:
    virtual ErrCode StructObject_getStructType(IStructType** type) = 0;
    virtual ErrCode StructObject_getFieldNames(IList** names) = 0;
    virtual ErrCode StructObject_getFieldValues(IList** values) = 0;
    virtual ErrCode StructObject_get(IString* name, IBaseObject** field) = 0;
    virtual ErrCode StructObject_hasField(IString* name, Bool* contains) = 0;
    virtual ErrCode StructObject_getAsDictionary(IDict** dictionary) = 0;

    ErrCode INTERFACE_FUNC getStructType(IStructType** type) override
    {
        return StructObject_getStructType(type);
    }

    ErrCode INTERFACE_FUNC getFieldNames(IList** names) override
    {
        return StructObject_getFieldNames(names);
    }

    ErrCode INTERFACE_FUNC getFieldValues(IList** values) override
    {
        return StructObject_getFieldValues(values);
    }

    ErrCode INTERFACE_FUNC get(IString* name, IBaseObject** field) override
    {
        return StructObject_get(name, field);
    }

    ErrCode INTERFACE_FUNC hasField(IString* name, Bool* contains) override
    {
        return StructObject_hasField(name, contains);
    }

    ErrCode INTERFACE_FUNC getAsDictionary(IDict** dictionary) override
    {
        return StructObject_getAsDictionary(dictionary);
    }
};

class IProperty_Helper : public IProperty
{
public:
    virtual ErrCode Property_GetValueType(CoreType* type)  = 0;
    virtual ErrCode Property_GetKeyType(CoreType* type)  = 0;
    virtual ErrCode Property_GetItemType(CoreType* type)  = 0;
    virtual ErrCode Property_GetName(IString** name)  = 0;
    virtual ErrCode Property_GetDescription(IString** description)  = 0;
    virtual ErrCode Property_GetUnit(IUnit** unit)  = 0;
    virtual ErrCode Property_GetMinValue(INumber** min)  = 0;
    virtual ErrCode Property_GetMaxValue(INumber** max)  = 0;
    virtual ErrCode Property_GetDefaultValue(IBaseObject** value)  = 0;
    virtual ErrCode Property_GetSuggestedValues(IList** values)  = 0;
    virtual ErrCode Property_GetVisible(Bool* visible)  = 0;
    virtual ErrCode Property_GetReadOnly(Bool* readOnly)  = 0;
    virtual ErrCode Property_GetSelectionValues(IBaseObject** values)  = 0;
    virtual ErrCode Property_GetReferencedProperty(IProperty** property)  = 0;
    virtual ErrCode Property_GetIsReferenced(Bool* isReferenced)  = 0;
    virtual ErrCode Property_GetValidator(IValidator** validator)  = 0;
    virtual ErrCode Property_GetCoercer(ICoercer** coercer)  = 0;
    virtual ErrCode Property_GetCallableInfo(ICallableInfo** callable)  = 0;
    virtual ErrCode Property_GetStructType(IStructType** structType)  = 0;
    virtual ErrCode Property_GetOnPropertyValueWrite(IEvent** event)  = 0;
    virtual ErrCode Property_GetOnPropertyValueRead(IEvent** event)  = 0;
    virtual ErrCode Property_GetValue(IBaseObject** value)  = 0;
    virtual ErrCode Property_SetValue(IBaseObject* value)  = 0;

    ErrCode INTERFACE_FUNC getValueType(CoreType* type) override
    {
        return Property_GetValueType(type);
    }

    ErrCode INTERFACE_FUNC getKeyType(CoreType* type) override
    {
        return Property_GetKeyType(type);
    }

    ErrCode INTERFACE_FUNC getItemType(CoreType* type) override
    {
        return Property_GetItemType(type);
    }

    ErrCode INTERFACE_FUNC getName(IString** name) override
    {
        return Property_GetName(name);
    }

    ErrCode INTERFACE_FUNC getDescription(IString** description) override
    {
        return Property_GetDescription(description);
    }

    ErrCode INTERFACE_FUNC getUnit(IUnit** unit) override
    {
        return Property_GetUnit(unit);
    }

    ErrCode INTERFACE_FUNC getMinValue(INumber** min) override
    {
        return Property_GetMinValue(min);
    }

    ErrCode INTERFACE_FUNC getMaxValue(INumber** max) override
    {
        return Property_GetMaxValue(max);
    }

    ErrCode INTERFACE_FUNC getDefaultValue(IBaseObject** value) override
    {
        return Property_GetDefaultValue(value);
    }

    ErrCode INTERFACE_FUNC getSuggestedValues(IList** values) override
    {
        return Property_GetSuggestedValues(values);
    }

    ErrCode INTERFACE_FUNC getVisible(Bool* visible) override
    {
        return Property_GetVisible(visible);
    }

    ErrCode INTERFACE_FUNC getReadOnly(Bool* readOnly) override
    {
        return Property_GetReadOnly(readOnly);
    }

    ErrCode INTERFACE_FUNC getSelectionValues(IBaseObject** values) override
    {
        return Property_GetSelectionValues(values);
    }

    ErrCode INTERFACE_FUNC getReferencedProperty(IProperty** property) override
    {
        return Property_GetReferencedProperty(property);
    }

    ErrCode INTERFACE_FUNC getIsReferenced(Bool* isReferenced) override
    {
        return Property_GetIsReferenced(isReferenced);
    }

    ErrCode INTERFACE_FUNC getValidator(IValidator** validator) override
    {
        return Property_GetValidator(validator);
    }

    ErrCode INTERFACE_FUNC getCoercer(ICoercer** coercer) override
    {
        return Property_GetCoercer(coercer);
    }

    ErrCode INTERFACE_FUNC getCallableInfo(ICallableInfo** callable) override
    {
        return Property_GetCallableInfo(callable);
    }

    ErrCode INTERFACE_FUNC getStructType(IStructType** structType) override
    {
        return Property_GetStructType(structType);
    }

    ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IEvent** event) override
    {
        return Property_GetOnPropertyValueWrite(event);
    }

    ErrCode INTERFACE_FUNC getOnPropertyValueRead(IEvent** event) override
    {
        return Property_GetOnPropertyValueRead(event);
    }

    ErrCode INTERFACE_FUNC getValue(IBaseObject** value) override
    {
        return Property_GetValue(value);
    }

    ErrCode INTERFACE_FUNC setValue(IBaseObject* value) override
    {
        return Property_SetValue(value);
    }
};

END_NAMESPACE_OPENDAQ
