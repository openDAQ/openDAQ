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
#include <unordered_set>
#include <coretypes/coretypes.h>
#include <coreobjects/eval_value.h>
#include <coreobjects/eval_nodes.h>
#include <coreobjects/ownable.h>
#include <coreobjects/property_object_ptr.h>
#include <coreobjects/eval_value_helpers.h>
#include <coreobjects/unit_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

struct ISerializedObject;

class EvalValueImpl : public ImplementationOf<IEvalValue, IOwnable, ICoreType, IInteger_Helper, ISerializable, IFloat_Helper,
                                              IBoolean_Helper, IString_Helper, IConvertible, IList, INumber, IProperty_Helper, IUnit_Helper, IStruct_Helper>
{
public:
    EvalValueImpl(IString* eval);
    EvalValueImpl(IString* eval, IFunction* func);
    EvalValueImpl(IString* eval, ListPtr<IBaseObject> arguments);

    EvalValueImpl(const EvalValueImpl& ev, IPropertyObject* owner);
    EvalValueImpl(const EvalValueImpl& ev, IPropertyObject* owner, IFunction* func);
    ErrCode INTERFACE_FUNC getEval(IString** evalString) override;

    ErrCode INTERFACE_FUNC getResult(IBaseObject** obj) override;
    ErrCode INTERFACE_FUNC cloneWithOwner(IPropertyObject* newOwner, IEvalValue** clonedValue) override;
    ErrCode INTERFACE_FUNC getParseErrorCode() override;
    ErrCode INTERFACE_FUNC getPropertyReferences(IList** propertyReferences) override;

    // IBaseObject
    ErrCode INTERFACE_FUNC toString(CharPtr* str) override;

    // IOwnable
    ErrCode INTERFACE_FUNC setOwner(IPropertyObject* value) override;

    // ICoreType
    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override;

    // IFloat
    ErrCode Float_GetValue(Float* value) override;
    ErrCode Float_EqualsValue(const Float value, Bool* equals) override;

    // IInteger
    ErrCode Integer_GetValue(Int* value) override;
    ErrCode Integer_EqualsValue(const Int value, Bool* equals) override;

    // IBoolean
    ErrCode Boolean_GetValue(Bool* value) override;
    ErrCode Boolean_EqualsValue(const Bool value, Bool* equals) override;

    // IString
    ErrCode StringObject_GetCharPtr(ConstCharPtr* value) override;
    ErrCode StringObject_GetLength(SizeT* size) override;

    // IConvertible
    ErrCode INTERFACE_FUNC toFloat(Float* val) override;
    ErrCode INTERFACE_FUNC toInt(Int* val) override;
    ErrCode INTERFACE_FUNC toBool(Bool* val) override;

    // IList
    ErrCode INTERFACE_FUNC getItemAt(SizeT index, IBaseObject** obj) override;
    ErrCode INTERFACE_FUNC setItemAt(SizeT index, IBaseObject* obj) override;
    ErrCode INTERFACE_FUNC getCount(SizeT* size) override;

    ErrCode INTERFACE_FUNC pushBack(IBaseObject* obj) override;
    ErrCode INTERFACE_FUNC pushFront(IBaseObject* obj) override;

    ErrCode INTERFACE_FUNC moveBack(IBaseObject* obj) override;
    ErrCode INTERFACE_FUNC moveFront(IBaseObject* obj) override;

    ErrCode INTERFACE_FUNC popBack(IBaseObject** obj) override;
    ErrCode INTERFACE_FUNC popFront(IBaseObject** obj) override;

    ErrCode INTERFACE_FUNC insertAt(SizeT index, IBaseObject* obj) override;
    ErrCode INTERFACE_FUNC removeAt(SizeT index, IBaseObject** obj) override;
    ErrCode INTERFACE_FUNC deleteAt(SizeT index) override;

    ErrCode INTERFACE_FUNC clear() override;

    ErrCode INTERFACE_FUNC createStartIterator(IIterator** iterator) override;
    ErrCode INTERFACE_FUNC createEndIterator(IIterator** iterator) override;

    // INumber
    ErrCode INTERFACE_FUNC getFloatValue(Float* value) override;
    ErrCode INTERFACE_FUNC getIntValue(Int* value) override;

    // IProperty

    ErrCode Property_GetValueType(CoreType* type) override;
    ErrCode Property_GetKeyType(CoreType* type) override;
    ErrCode Property_GetItemType(CoreType* type) override;
    ErrCode Property_GetName(IString** name) override;
    ErrCode Property_GetDescription(IString** description) override;
    ErrCode Property_GetUnit(IUnit** unit) override;
    ErrCode Property_GetMinValue(INumber** min) override;
    ErrCode Property_GetMaxValue(INumber** max) override;
    ErrCode Property_GetDefaultValue(IBaseObject** value) override;
    ErrCode Property_GetSuggestedValues(IList** values) override;
    ErrCode Property_GetVisible(Bool* visible) override;
    ErrCode Property_GetReadOnly(Bool* readOnly) override;
    ErrCode Property_GetSelectionValues(IBaseObject** values) override;
    ErrCode Property_GetReferencedProperty(IProperty** property) override;
    ErrCode Property_GetIsReferenced(Bool* isReferenced) override;
    ErrCode Property_GetValidator(IValidator** validator) override;
    ErrCode Property_GetCoercer(ICoercer** coercer) override;
    ErrCode Property_GetCallableInfo(ICallableInfo** callable) override;
    ErrCode Property_GetStructType(IStructType** structType) override;
    ErrCode Property_GetOnPropertyValueWrite(IEvent** event) override;
    ErrCode Property_GetOnPropertyValueRead(IEvent** event) override;

    // IUnit

    ErrCode UnitObject_GetId(Int* id) override;
    ErrCode UnitObject_GetSymbol(IString** symbol) override;
    ErrCode UnitObject_GetName(IString** name) override;
    ErrCode UnitObject_GetQuantity(IString** quantity) override;

    // IStruct

    ErrCode StructObject_getStructType(IStructType** type) override;
    ErrCode StructObject_getFieldNames(IList** names) override;
    ErrCode StructObject_getFieldValues(IList** values) override;
    ErrCode StructObject_get(IString* name, IBaseObject** field) override;
    ErrCode StructObject_hasField(IString* name, Bool* contains) override;
    ErrCode StructObject_getAsDictionary(IDict** dictionary) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj);
    static ConstCharPtr SerializeId();

private:
    StringPtr eval;
    std::unique_ptr<BaseNode> node;
    std::unique_ptr<std::unordered_set<std::string>> propertyReferences;
    ListPtr<IBaseObject> arguments;
    WeakRefPtr<IPropertyObject> owner;
    StringPtr ownerRefStr;
    ResolveStatus resolveStatus;
    ErrCode parseErrCode;
    std::string strResult;
    std::string parseErrMessage;
    bool calculated;
    bool useFunctionResolver;
    FunctionPtr func;

    BaseObjectPtr getReference(const std::string& str, RefType refType, int argIndex, std::string& postRef) const;
    int resolveReferences();

    ErrCode checkParseAndResolve();

    template <typename T>
    inline ErrCode getValueInternal(T& value);

    template <typename T>
    inline ErrCode equalsValueInternal(const T value, Bool* equals);

    BaseObjectPtr calc();
    void checkForEvalValue(BaseObjectPtr& prop) const;
    BaseObjectPtr getReferenceFromPrefix(const PropertyObjectPtr& propObject, const std::string& str, RefType refType) const;

protected:
    void internalDispose(bool disposing) override;
    void onCreate();
};

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, EvalValue, IString*, eval)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY,
    EvalValue,
    IEvalValue,
    createEvalValueArgs,
    IString*,
    eval,
    IList*,
    args
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY,
    EvalValue,
    IEvalValue,
    createEvalValueFunc,
    IString*,
    eval,
    IFunction*,
    func
)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(EvalValueImpl)

END_NAMESPACE_OPENDAQ
