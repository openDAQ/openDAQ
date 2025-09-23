#include <copendaq.h>

#include <gtest/gtest.h>

using CCoretypesTest = testing::Test;

TEST_F(CCoretypesTest, BaseObject)
{
    daqBaseObject* obj = nullptr;
    daqErrCode err = 0;
    err = daqBaseObject_create(&obj);
    ASSERT_EQ(err, 0);
    ASSERT_NE(obj, nullptr);
    err = daqBaseObject_releaseRef(obj);
    ASSERT_EQ(err, 0);
}

TEST_F(CCoretypesTest, Binarydata)
{
    daqBinaryData* data = nullptr;
    void* dataPtr = nullptr;
    daqSizeT size = 0;
    daqErrCode err = 0;
    err = daqBinaryData_createBinaryData(&data, 10);
    ASSERT_EQ(err, 0);
    err = daqBinaryData_getAddress(data, &dataPtr);
    ASSERT_EQ(err, 0);
    ASSERT_NE(dataPtr, nullptr);
    err = daqBinaryData_getSize(data, &size);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(size, 10);
    daqBaseObject_releaseRef(data);
}

TEST_F(CCoretypesTest, Boolean)
{
    daqBoolean* b = nullptr;
    daqErrCode err = 0;
    daqBool value = False;
    err = daqBoolean_createBoolean(&b, value);
    ASSERT_EQ(err, 0);
    err = daqBoolean_getValue(b, &value);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(value, False);
    daqBaseObject_releaseRef(b);
}

TEST_F(CCoretypesTest, Cloneable)
{
    daqList* list = nullptr;
    daqList_createList(&list);

    daqInteger* i1 = nullptr;
    daqInteger_createInteger(&i1, 1);
    daqInteger* i2 = nullptr;
    daqInteger_createInteger(&i2, 2);

    daqList_pushBack(list, i1);

    daqCloneable* c = nullptr;
    daqBaseObject_borrowInterface(list, DAQ_CLONEABLE_INTF_ID, (daqBaseObject**) &c);

    daqList* clonedList = nullptr;
    daqCloneable_clone(c, (daqBaseObject**) &clonedList);
    ASSERT_NE(clonedList, nullptr);

    daqBool eq = False;
    daqBaseObject_equals(list, clonedList, &eq);
    ASSERT_EQ(eq, True);

    daqList_pushBack(clonedList, i2);

    daqSizeT count = 0;
    daqList_getCount(list, &count);
    ASSERT_EQ(count, 1);

    daqBaseObject_equals(list, clonedList, &eq);
    ASSERT_EQ(eq, False);

    daqBaseObject_releaseRef(clonedList);
    daqBaseObject_releaseRef(list);
    daqBaseObject_releaseRef(i1);
    daqBaseObject_releaseRef(i2);
}

TEST_F(CCoretypesTest, Comparable)
{
}

TEST_F(CCoretypesTest, ComplexNumber)
{
    daqComplexNumber* cn = nullptr;
    daqErrCode err = 0;
    daqFloat real = 1.0;
    daqFloat imag = 2.0;
    err = daqComplexNumber_createComplexNumber(&cn, real, imag);
    ASSERT_EQ(err, 0);
    err = daqComplexNumber_getReal(cn, &real);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(real, 1.0);
    err = daqComplexNumber_getImaginary(cn, &imag);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(imag, 2.0);
    daqBaseObject_releaseRef(cn);
}

TEST_F(CCoretypesTest, Convertible)
{
    daqString* s = nullptr;
    daqConvertible* c = nullptr;
    daqErrCode err = 0;
    daqFloat f = 0.0;

    err = daqString_createString(&s, "1.5");
    ASSERT_EQ(err, 0);
    err = daqBaseObject_borrowInterface(s, DAQ_CONVERTIBLE_INTF_ID, (daqBaseObject**) &c);
    ASSERT_EQ(err, 0);
    err = daqConvertible_toFloat(c, &f);
    ASSERT_EQ(err, 0);
    ASSERT_DOUBLE_EQ(f, 1.5);
    daqBaseObject_releaseRef(s);
}

TEST_F(CCoretypesTest, CoreType)
{
    daqInteger* i = nullptr;
    daqInteger_createInteger(&i, 1);

    daqCoreTypeObject* coreTypeObj = nullptr;
    daqBaseObject_borrowInterface(i, DAQ_CORE_TYPE_INTF_ID, (daqBaseObject**) &coreTypeObj);

    daqCoreType coreType = daqCoreType::daqCtUndefined;
    daqCoreType_getCoreType(coreTypeObj, &coreType);

    ASSERT_EQ(coreType, daqCoreType::daqCtInt);

    daqBaseObject_releaseRef(i);
}

TEST_F(CCoretypesTest, Dictobject)
{
    daqDict* dict = nullptr;
    daqErrCode err = 0;

    daqString* key = nullptr;
    daqString* value = nullptr;
    err = daqString_createString(&key, "key");
    ASSERT_EQ(err, 0);
    err = daqString_createString(&value, "value");
    ASSERT_EQ(err, 0);

    err = daqDict_createDict(&dict);
    ASSERT_EQ(err, 0);
    err = daqDict_set(dict, key, value);
    ASSERT_EQ(err, 0);

    daqSizeT count = 0;
    err = daqDict_getCount(dict, &count);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(count, 1);

    daqString* value2 = nullptr;
    err = daqDict_get(dict, key, (daqBaseObject**) &value2);
    ASSERT_EQ(err, 0);
    daqConstCharPtr str = nullptr;
    err = daqString_getCharPtr(value2, &str);
    ASSERT_EQ(err, 0);
    ASSERT_STREQ(str, "value");

    daqBaseObject_releaseRef(key);
    daqBaseObject_releaseRef(value);
    daqBaseObject_releaseRef(value2);
    daqBaseObject_releaseRef(dict);
}

TEST_F(CCoretypesTest, Enumerations)
{
    daqEnumerationType* et = nullptr;

    daqDict* enumerators = nullptr;
    daqDict_createDict(&enumerators);
    daqInteger* i1 = nullptr;
    daqInteger* i2 = nullptr;
    daqInteger_createInteger(&i1, 1);
    daqInteger_createInteger(&i2, 2);

    daqString* s1 = nullptr;
    daqString* s2 = nullptr;
    daqString_createString(&s1, "One");
    daqString_createString(&s2, "Two");

    daqDict_set(enumerators, s1, i1);
    daqDict_set(enumerators, s2, i2);

    daqString* typeName = nullptr;
    daqString_createString(&typeName, "MyEnum");

    daqEnumerationType_createEnumerationTypeWithValues(&et, typeName, enumerators);

    daqSizeT count = 0;
    daqEnumerationType_getCount(et, &count);
    ASSERT_EQ(count, 2);

    daqEnumeration* e = nullptr;
    daqEnumeration_createEnumerationWithType(&e, et, s2);
    daqInt value = 0;
    daqEnumeration_getIntValue(e, &value);

    ASSERT_EQ(value, 2);

    daqBaseObject_releaseRef(e);
    daqBaseObject_releaseRef(typeName);
    daqBaseObject_releaseRef(s2);
    daqBaseObject_releaseRef(s1);
    daqBaseObject_releaseRef(i2);
    daqBaseObject_releaseRef(i1);
    daqBaseObject_releaseRef(enumerators);
    daqBaseObject_releaseRef(et);
}

TEST_F(CCoretypesTest, Event)
{
    daqEvent* e = nullptr;
    daqErrCode err = 0;
    err = daqEvent_createEvent(&e);
    ASSERT_EQ(err, 0);

    daqSizeT count = 1;
    daqEvent_getSubscriberCount(e, &count);
    ASSERT_EQ(count, 0);
    daqBaseObject_releaseRef(e);
}

TEST_F(CCoretypesTest, EventArgs)
{
    daqErrCode err = 0;
    daqEventArgs* args = nullptr;
    daqString* name = nullptr;
    daqString_createString(&name, "test_event");
    err = daqEventArgs_createEventArgs(&args, 10, name);
    ASSERT_EQ(err, 0);
    daqInt id = 0;
    daqEventArgs_getEventId(args, &id);
    ASSERT_EQ(id, 10);
    daqString* name2 = nullptr;
    daqEventArgs_getEventName(args, &name2);
    daqConstCharPtr str = nullptr;
    daqString_getCharPtr(name2, &str);
    ASSERT_STREQ(str, "test_event");
    daqBaseObject_releaseRef(name2);
    daqBaseObject_releaseRef(name);
    daqBaseObject_releaseRef(args);
}

static daqBool eventCalled = False;
static void onEvent(daqBaseObject* sender, daqBaseObject* args)
{
    eventCalled = True;
    daqBaseObject_releaseRef(sender);
    daqBaseObject_releaseRef(args);
}

TEST_F(CCoretypesTest, EventHandler)
{
    daqEventHandler* eh = nullptr;
    daqBaseObject* sender = nullptr;
    daqBaseObject_create(&sender);
    daqBaseObject* args = nullptr;
    daqBaseObject_create(&args);
    daqEventHandler_createEventHandler(&eh, onEvent);
    daqEventHandler_handleEvent(eh, sender, (daqEventArgs*) args);
    ASSERT_EQ(eventCalled, True);

    daqBaseObject_releaseRef(sender);
    daqBaseObject_releaseRef(args);
    daqBaseObject_releaseRef(eh);
}

TEST_F(CCoretypesTest, Float)
{
    daqFloatObject* f = nullptr;
    daqErrCode err = 0;
    daqFloat value = 1.0;
    err = daqFloatObject_createFloat(&f, value);
    ASSERT_EQ(err, 0);
    err = daqFloatObject_getValue(f, &value);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(value, 1.0);
    daqBaseObject_releaseRef(f);
}

TEST_F(CCoretypesTest, Freezable)
{
    daqList* list = nullptr;
    daqFreezable* f = nullptr;
    daqErrCode err = 0;
    err = daqList_createList(&list);
    ASSERT_EQ(err, 0);
    daqBool isFrozen = False;
    err = daqBaseObject_borrowInterface(list, DAQ_FREEZABLE_INTF_ID, (daqBaseObject**) &f);
    ASSERT_EQ(err, 0);

    err = daqFreezable_isFrozen(f, &isFrozen);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(isFrozen, False);

    err = daqFreezable_freeze(f);
    ASSERT_EQ(err, 0);

    err = daqFreezable_isFrozen(f, &isFrozen);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(isFrozen, True);

    daqBaseObject_releaseRef(list);
}

static daqBool b = False;
static daqErrCode func_call(daqBaseObject*, daqBaseObject**)
{
    b = True;
    return 0;
}

TEST_F(CCoretypesTest, Function)
{
    daqFunction* f = nullptr;
    daqErrCode err = 0;
    err = daqFunction_createFunction(&f, func_call);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(b, False);

    daqBaseObject* params = nullptr;
    daqBaseObject* result = nullptr;

    daqBaseObject_create(&params);
    daqBaseObject_create(&result);

    err = daqFunction_call(f, params, &result);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(b, True);
    daqBaseObject_releaseRef(f);
    daqBaseObject_releaseRef(params);
    daqBaseObject_releaseRef(result);
}

TEST_F(CCoretypesTest, Integer)
{
    daqInteger* i = nullptr;
    daqErrCode err = 0;
    daqInt value = 1;
    err = daqInteger_createInteger(&i, value);
    ASSERT_EQ(err, 0);
    err = daqInteger_getValue(i, &value);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(value, 1);
    daqBaseObject_releaseRef(i);
}

TEST_F(CCoretypesTest, Iterable)
{
    // filling list
    daqErrCode err = 0;
    daqList* list = nullptr;
    err = daqList_createList(&list);
    ASSERT_EQ(err, 0);

    daqInteger* i1 = nullptr;
    daqInteger* i2 = nullptr;
    daqInteger_createInteger(&i1, 3);
    daqInteger_createInteger(&i2, 4);

    daqList_moveBack(list, i1);
    daqList_moveBack(list, i2);

    // cast to iterable
    daqIterable* iter = nullptr;
    err = daqBaseObject_borrowInterface(list, DAQ_ITERABLE_INTF_ID, (daqBaseObject**) &iter);
    ASSERT_EQ(err, 0);

    // create iterators
    daqIterator* itb = nullptr;
    daqIterator* ite = nullptr;
    err = daqIterable_createStartIterator(iter, &itb);
    ASSERT_EQ(err, 0);
    err = daqIterable_createEndIterator(iter, &ite);
    ASSERT_EQ(err, 0);

    // iterate
    daqBool eq = False;
    int i = 0;
    int a[2] = {3, 4};
    daqIterator_moveNext(itb);  // iterator needs to be moved for the first use
    daqBaseObject_equals(itb, ite, &eq);
    while (eq == False)
    {
        daqInteger* tmp = nullptr;
        err = daqIterator_getCurrent(itb, (daqBaseObject**) &tmp);
        ASSERT_EQ(err, 0);

        daqInt val = 0;
        err = daqInteger_getValue(tmp, &val);

        ASSERT_EQ(err, 0);
        ASSERT_EQ(val, a[i++]);

        daqIterator_moveNext(itb);
        daqBaseObject_equals(itb, ite, &eq);
    }

    // clean up
    int refc = 0;
    refc = daqBaseObject_releaseRef(itb);
    ASSERT_EQ(refc, 0);
    refc = daqBaseObject_releaseRef(ite);
    ASSERT_EQ(refc, 0);
    refc = daqBaseObject_releaseRef(list);
    ASSERT_EQ(refc, 0);
}

TEST_F(CCoretypesTest, Listobject)
{
    daqList* list = nullptr;
    daqErrCode err = 0;
    err = daqList_createList(&list);
    ASSERT_EQ(err, 0);

    daqInteger* i1 = nullptr;
    daqInteger* i2 = nullptr;
    daqInteger* i3 = nullptr;
    daqInteger_createInteger(&i1, 1);
    daqInteger_createInteger(&i2, 2);
    daqInteger_createInteger(&i3, 3);

    err = daqList_pushBack(list, i1);
    ASSERT_EQ(err, 0);
    err = daqList_pushBack(list, i2);
    ASSERT_EQ(err, 0);
    err = daqList_pushBack(list, i3);
    ASSERT_EQ(err, 0);

    daqSizeT count = 0;
    err = daqList_getCount(list, &count);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(count, 3);

    daqInteger* i = nullptr;
    err = daqList_popFront(list, (daqBaseObject**) &i);
    ASSERT_EQ(err, 0);
    daqInt value = 0;
    err = daqInteger_getValue(i, &value);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(value, 1);

    daqBaseObject_releaseRef(i);
    i = nullptr;

    err = daqList_removeAt(list, 1, (daqBaseObject**) &i);
    ASSERT_EQ(err, 0);
    err = daqInteger_getValue(i, &value);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(value, 3);

    daqBaseObject_releaseRef(i);

    err = daqList_clear(list);
    ASSERT_EQ(err, 0);
    err = daqList_getCount(list, &count);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(count, 0);

    daqBaseObject_releaseRef(list);
    daqBaseObject_releaseRef(i3);
    daqBaseObject_releaseRef(i2);
    daqBaseObject_releaseRef(i1);
}

TEST_F(CCoretypesTest, Number)
{
    daqFloatObject* f1 = nullptr;
    daqFloat f = 2.2;
    daqErrCode err = 0;
    daqNumber* n1 = nullptr;
    daqInt i = 0;

    err = daqFloatObject_createFloat(&f1, f);
    ASSERT_EQ(err, 0);
    err = daqBaseObject_borrowInterface(f1, DAQ_NUMBER_INTF_ID, (daqBaseObject**) &n1);
    ASSERT_EQ(err, 0);
    daqNumber_getIntValue(n1, &i);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(i, 2);
    daqBaseObject_releaseRef(f1);
}

typedef daqErrCode (*ProcCall)(daqBaseObject*);

static daqBool b2 = False;
static daqErrCode proc_call(daqBaseObject*)
{
    b2 = True;
    return 0;
}

TEST_F(CCoretypesTest, Procedure)
{
    daqProcedure* p = nullptr;
    daqErrCode err = 0;
    err = daqProcedure_createProcedure(&p, proc_call);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(b2, False);
    err = daqProcedure_dispatch(p, nullptr);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(b2, True);
    daqBaseObject_releaseRef(p);
}

TEST_F(CCoretypesTest, Ratio)
{
    daqRatio* r = nullptr;
    daqErrCode err = 0;

    err = daqRatio_createRatio(&r, 1, 2);
    ASSERT_EQ(err, 0);

    daqInt numerator = 0;
    daqInt denominator = 0;
    err = daqRatio_getNumerator(r, &numerator);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(numerator, 1);
    err = daqRatio_getDenominator(r, &denominator);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(denominator, 2);
    daqBaseObject_releaseRef(r);
}

TEST_F(CCoretypesTest, Serializable)
{
    daqList* list = nullptr;
    daqSerializable* s = nullptr;
    daqSerializer* serializer = nullptr;
    daqErrCode err = 0;

    err = daqSerializer_createJsonSerializer(&serializer, False);
    ASSERT_EQ(err, 0);
    err = daqList_createList(&list);
    ASSERT_EQ(err, 0);
    err = daqBaseObject_borrowInterface(list, DAQ_SERIALIZABLE_INTF_ID, (daqBaseObject**) &s);
    ASSERT_EQ(err, 0);
    err = daqSerializable_serialize(s, serializer);

    daqString* serialized = nullptr;
    err = daqSerializer_getOutput(serializer, &serialized);
    ASSERT_EQ(err, 0);
    daqConstCharPtr str = nullptr;
    err = daqString_getCharPtr(serialized, &str);
    ASSERT_EQ(err, 0);

    // Disabled due to inconsistent output format
    // ASSERT_STREQ(str, "[]");
    // ASSERT_STREQ(str, "{\"__type\":\"List\",\"values\":[]}");

    daqBaseObject_releaseRef(serialized);
    daqBaseObject_releaseRef(serializer);
    daqBaseObject_releaseRef(list);
}

TEST_F(CCoretypesTest, SerializedList)
{
}

TEST_F(CCoretypesTest, SerializedObject)
{
}

TEST_F(CCoretypesTest, SimpleType)
{
    daqSimpleType* st = nullptr;
    daqErrCode err = 0;
    err = daqSimpleType_createSimpleType(&st, daqCoreType::daqCtBool);
    ASSERT_EQ(err, 0);
    daqBaseObject_releaseRef(st);
}

TEST_F(CCoretypesTest, Stringobject)
{
    daqString* s = nullptr;
    daqErrCode err = 0;
    err = daqString_createString(&s, "Hello");
    ASSERT_EQ(err, 0);
    daqConstCharPtr str = nullptr;
    err = daqString_getCharPtr(s, &str);
    ASSERT_EQ(err, 0);
    ASSERT_STREQ(str, "Hello");
    daqBaseObject_releaseRef(s);
}

TEST_F(CCoretypesTest, Struct)
{
    daqErrCode err = 0;

    daqList* fieldNames = nullptr;
    daqList_createList(&fieldNames);

    daqList* fieldTypes = nullptr;
    daqList_createList(&fieldTypes);

    daqString* fieldName = nullptr;
    daqString_createString(&fieldName, "int");

    daqSimpleType* st = nullptr;
    daqSimpleType_createSimpleType(&st, daqCoreType::daqCtInt);

    daqInteger* i = nullptr;
    daqInteger_createInteger(&i, 10);

    daqList_pushBack(fieldTypes, st);
    daqList_pushBack(fieldNames, fieldName);

    daqStructType* type = nullptr;
    daqString* typeName = nullptr;
    daqString_createString(&typeName, "test");
    daqStructType_createStructTypeNoDefaults(&type, typeName, fieldNames, fieldTypes);

    daqTypeManager* manager = nullptr;
    daqTypeManager_createTypeManager(&manager);
    daqTypeManager_addType(manager, (daqType*) type);

    daqStructBuilder* sb = nullptr;
    err = daqStructBuilder_createStructBuilder(&sb, typeName, manager);
    daqStructBuilder_set(sb, fieldName, i);

    daqStruct* s = nullptr;
    err = daqStructBuilder_build(sb, &s);
    ASSERT_EQ(err, 0);

    daqInteger* i2 = nullptr;
    daqStruct_get(s, fieldName, (daqBaseObject**) &i2);
    daqInt value = 0;
    err = daqInteger_getValue(i2, &value);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(value, 10);

    daqBaseObject_releaseRef(i2);
    daqBaseObject_releaseRef(i);

    daqBaseObject_releaseRef(s);
    daqBaseObject_releaseRef(sb);

    daqBaseObject_releaseRef(manager);
    daqBaseObject_releaseRef(type);
    daqBaseObject_releaseRef(typeName);

    daqBaseObject_releaseRef(fieldNames);
    daqBaseObject_releaseRef(fieldTypes);

    daqBaseObject_releaseRef(fieldName);
    daqBaseObject_releaseRef(st);
}

TEST_F(CCoretypesTest, TypeManager)
{
    daqErrCode err = 0;

    daqList* fieldNames = nullptr;
    daqList* fieldTypes = nullptr;

    err = daqList_createList(&fieldNames);
    err = daqList_createList(&fieldTypes);

    daqString* fieldName = nullptr;
    err = daqString_createString(&fieldName, "int");

    daqSimpleType* st = nullptr;
    err = daqSimpleType_createSimpleType(&st, daqCoreType::daqCtInt);

    err = daqList_pushBack(fieldTypes, st);
    err = daqList_pushBack(fieldNames, fieldName);

    daqString* typeName = nullptr;
    err = daqString_createString(&typeName, "test");

    daqStructType* type = nullptr;
    err = daqStructType_createStructTypeNoDefaults(&type, typeName, fieldNames, fieldTypes);

    daqTypeManager* manager = nullptr;
    err = daqTypeManager_createTypeManager(&manager);
    ASSERT_EQ(err, 0);
    err = daqTypeManager_addType(manager, (daqType*) type);
    ASSERT_EQ(err, 0);
    err = daqTypeManager_removeType(manager, typeName);
    ASSERT_EQ(err, 0);

    daqBaseObject_releaseRef(fieldNames);
    daqBaseObject_releaseRef(fieldTypes);
    daqBaseObject_releaseRef(fieldName);
    daqBaseObject_releaseRef(typeName);
    daqBaseObject_releaseRef(st);
    daqBaseObject_releaseRef(type);
    daqBaseObject_releaseRef(manager);
}

TEST_F(CCoretypesTest, Updatable)
{
}

TEST_F(CCoretypesTest, VersionInfo)
{
    daqVersionInfo* vi = nullptr;
    daqErrCode err = 0;
    err = daqVersionInfo_createVersionInfo(&vi, 1, 2, 3);
    ASSERT_EQ(err, 0);
    daqSizeT major = 0;
    daqSizeT minor = 0;
    daqSizeT patch = 0;
    err = daqVersionInfo_getMajor(vi, &major);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(major, 1);
    err = daqVersionInfo_getMinor(vi, &minor);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(minor, 2);
    err = daqVersionInfo_getPatch(vi, &patch);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(patch, 3);
    daqBaseObject_releaseRef(vi);
}
