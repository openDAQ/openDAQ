#include <copendaq.h>
#include <gtest/gtest.h>

using CCoretypesTest = testing::Test;

TEST_F(CCoretypesTest, BaseObject)
{
    BaseObject* obj = nullptr;
    ErrCode err = 0;
    err = BaseObject_create(&obj);
    ASSERT_EQ(err, 0);
    ASSERT_NE(obj, nullptr);
    err = BaseObject_releaseRef(obj);
    ASSERT_EQ(err, 0);
}

TEST_F(CCoretypesTest, Binarydata)
{
    BinaryData* data = nullptr;
    void* dataPtr = nullptr;
    SizeT size = 0;
    ErrCode err = 0;
    err = BinaryData_createBinaryData(&data, 10);
    ASSERT_EQ(err, 0);
    err = BinaryData_getAddress(data, &dataPtr);
    ASSERT_EQ(err, 0);
    ASSERT_NE(dataPtr, nullptr);
    err = BinaryData_getSize(data, &size);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(size, 10);
    BaseObject_releaseRef(data);
}

TEST_F(CCoretypesTest, Boolean)
{
    Boolean* b = nullptr;
    ErrCode err = 0;
    Bool value = False;
    err = Boolean_createBoolean(&b, value);
    ASSERT_EQ(err, 0);
    err = Boolean_getValue(b, &value);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(value, False);
    BaseObject_releaseRef(b);
}

TEST_F(CCoretypesTest, ComplexNumber)
{
    ComplexNumber* cn = nullptr;
    ErrCode err = 0;
    Float real = 1.0;
    Float imag = 2.0;
    err = ComplexNumber_createComplexNumber(&cn, real, imag);
    ASSERT_EQ(err, 0);
    err = ComplexNumber_getReal(cn, &real);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(real, 1.0);
    err = ComplexNumber_getImaginary(cn, &imag);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(imag, 2.0);
    BaseObject_releaseRef(cn);
}

TEST_F(CCoretypesTest, Convertible)
{
    String* s = nullptr;
    Convertible* c = nullptr;
    ErrCode err = 0;
    Float f = 0.0;

    err = String_createString(&s, "1.5");
    ASSERT_EQ(err, 0);
    err = BaseObject_borrowInterface(s, CONVERTIBLE_INTF_ID, reinterpret_cast<BaseObject**>(&c));
    ASSERT_EQ(err, 0);
    err = Convertible_toFloat(c, &f);
    ASSERT_EQ(err, 0);
    ASSERT_FLOAT_EQ(f, 1.5);
    BaseObject_releaseRef(s);
}

TEST_F(CCoretypesTest, Dictobject)
{
    Dict* dict = nullptr;
    ErrCode err = 0;

    String* key = nullptr;
    String* value = nullptr;
    err = String_createString(&key, "key");
    ASSERT_EQ(err, 0);
    err = String_createString(&value, "value");
    ASSERT_EQ(err, 0);

    err = Dict_createDict(&dict);
    ASSERT_EQ(err, 0);
    err = Dict_set(dict, key, value);
    ASSERT_EQ(err, 0);

    SizeT count = 0;
    err = Dict_getCount(dict, &count);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(count, 1);

    String* value2 = nullptr;
    err = Dict_get(dict, key, (BaseObject**) &value2);
    ASSERT_EQ(err, 0);
    ConstCharPtr str = nullptr;
    err = String_getCharPtr(value2, &str);
    ASSERT_EQ(err, 0);
    ASSERT_STREQ(str, "value");

    BaseObject_releaseRef(key);
    BaseObject_releaseRef(value);
    BaseObject_releaseRef(value2);
    BaseObject_releaseRef(dict);
}

TEST_F(CCoretypesTest, Enumerations)
{
    EnumerationType* et = nullptr;

    Dict* enumerators = nullptr;
    Dict_createDict(&enumerators);
    Integer* i1 = nullptr;
    Integer* i2 = nullptr;
    Integer_createInteger(&i1, 1);
    Integer_createInteger(&i2, 2);

    String* s1 = nullptr;
    String* s2 = nullptr;
    String_createString(&s1, "One");
    String_createString(&s2, "Two");

    Dict_set(enumerators, s1, i1);
    Dict_set(enumerators, s2, i2);

    String* typeName = nullptr;
    String_createString(&typeName, "MyEnum");

    EnumerationType_createEnumerationTypeWithValues(&et, typeName, enumerators);

    SizeT count = 0;
    EnumerationType_getCount(et, &count);
    ASSERT_EQ(count, 2);

    Enumeration* e = nullptr;
    Enumeration_createEnumerationWithType(&e, et, s2);
    Int value = 0;
    Enumeration_getIntValue(e, &value);

    ASSERT_EQ(value, 2);

    BaseObject_releaseRef(e);
    BaseObject_releaseRef(typeName);
    BaseObject_releaseRef(s2);
    BaseObject_releaseRef(s1);
    BaseObject_releaseRef(i2);
    BaseObject_releaseRef(i1);
    BaseObject_releaseRef(enumerators);
    BaseObject_releaseRef(et);
}

TEST_F(CCoretypesTest, Event)
{
    Event* e = nullptr;
    ErrCode err = 0;
    err = Event_createEvent(&e);
    ASSERT_EQ(err, 0);

    SizeT count = 1;
    Event_getSubscriberCount(e, &count);
    ASSERT_EQ(count, 0);
    BaseObject_releaseRef(e);
}

TEST_F(CCoretypesTest, EventArgs)
{
    ErrCode err = 0;
    EventArgs* args = nullptr;
    String* name = nullptr;
    String_createString(&name, "test_event");
    err = EventArgs_createEventArgs(&args, 10, name);
    ASSERT_EQ(err, 0);
    Int id = 0;
    EventArgs_getEventId(args, &id);
    ASSERT_EQ(id, 10);
    String* name2 = nullptr;
    EventArgs_getEventName(args, &name2);
    ConstCharPtr str = nullptr;
    String_getCharPtr(name2, &str);
    ASSERT_STREQ(str, "test_event");
    BaseObject_releaseRef(name2);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(args);
}

TEST_F(CCoretypesTest, EventHandler)
{
    // TODO: add later, cannot test now
}

TEST_F(CCoretypesTest, Float)
{
    FloatObject* f = nullptr;
    ErrCode err = 0;
    Float value = 1.0;
    err = FloatObject_createFloat(&f, value);
    ASSERT_EQ(err, 0);
    err = FloatObject_getValue(f, &value);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(value, 1.0);
    BaseObject_releaseRef(f);
}

TEST_F(CCoretypesTest, Freezable)
{
    List* list = nullptr;
    Freezable* f = nullptr;
    ErrCode err = 0;
    err = List_createList(&list);
    ASSERT_EQ(err, 0);
    Bool isFrozen = False;
    err = BaseObject_borrowInterface(list, FREEZABLE_INTF_ID, reinterpret_cast<BaseObject**>(&f));
    ASSERT_EQ(err, 0);

    err = Freezable_isFrozen(f, &isFrozen);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(isFrozen, False);

    err = Freezable_freeze(f);
    ASSERT_EQ(err, 0);

    err = Freezable_isFrozen(f, &isFrozen);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(isFrozen, True);

    BaseObject_releaseRef(list);
}

static Bool b = False;
static ErrCode func_call(BaseObject*, BaseObject**)
{
    b = True;
    return 0;
}

TEST_F(CCoretypesTest, Function)
{
    Function* f = nullptr;
    ErrCode err = 0;
    err = Function_createFunction(&f, func_call);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(b, False);

    BaseObject* params = nullptr;
    BaseObject* result = nullptr;

    BaseObject_create(&params);
    BaseObject_create(&result);

    err = Function_call(f, params, &result);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(b, True);
    BaseObject_releaseRef(f);
    BaseObject_releaseRef(params);
    BaseObject_releaseRef(result);
}

TEST_F(CCoretypesTest, Integer)
{
    Integer* i = nullptr;
    ErrCode err = 0;
    Int value = 1;
    err = Integer_createInteger(&i, value);
    ASSERT_EQ(err, 0);
    err = Integer_getValue(i, &value);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(value, 1);
    BaseObject_releaseRef(i);
}

TEST_F(CCoretypesTest, Iterable)
{
    // filling list
    ErrCode err = 0;
    List* list = nullptr;
    err = List_createList(&list);
    ASSERT_EQ(err, 0);

    Integer* i1 = nullptr;
    Integer* i2 = nullptr;
    Integer_createInteger(&i1, 3);
    Integer_createInteger(&i2, 4);

    List_moveBack(list, i1);
    List_moveBack(list, i2);

    // cast to iterable
    Iterable* iter = nullptr;
    err = BaseObject_borrowInterface(list, ITERABLE_INTF_ID, reinterpret_cast<BaseObject**>(&iter));
    ASSERT_EQ(err, 0);

    // create iterators
    Iterator* itb = nullptr;
    Iterator* ite = nullptr;
    err = Iterable_createStartIterator(iter, &itb);
    ASSERT_EQ(err, 0);
    err = Iterable_createEndIterator(iter, &ite);
    ASSERT_EQ(err, 0);

    // iterate
    Bool eq = False;
    int i = 0;
    int a[2] = {3, 4};
    Iterator_moveNext(itb);  // iterator needs to be moved for the first use
    BaseObject_equals(itb, ite, &eq);
    while (eq == False)
    {
        Integer* tmp = nullptr;
        err = Iterator_getCurrent(itb, (BaseObject**) &tmp);
        ASSERT_EQ(err, 0);

        Int val = 0;
        err = Integer_getValue(tmp, &val);

        ASSERT_EQ(err, 0);
        ASSERT_EQ(val, a[i++]);

        Iterator_moveNext(itb);
        BaseObject_equals(itb, ite, &eq);
    }

    // clean up
    int refc = 0;
    refc = BaseObject_releaseRef(itb);
    ASSERT_EQ(refc, 0);
    refc = BaseObject_releaseRef(ite);
    ASSERT_EQ(refc, 0);
    refc = BaseObject_releaseRef(list);
    ASSERT_EQ(refc, 0);
}

TEST_F(CCoretypesTest, Listobject)
{
    List* list = nullptr;
    ErrCode err = 0;
    err = List_createList(&list);
    ASSERT_EQ(err, 0);

    Integer* i1 = nullptr;
    Integer* i2 = nullptr;
    Integer* i3 = nullptr;
    Integer_createInteger(&i1, 1);
    Integer_createInteger(&i2, 2);
    Integer_createInteger(&i3, 3);

    err = List_pushBack(list, i1);
    ASSERT_EQ(err, 0);
    err = List_pushBack(list, i2);
    ASSERT_EQ(err, 0);
    err = List_pushBack(list, i3);
    ASSERT_EQ(err, 0);

    SizeT count = 0;
    err = List_getCount(list, &count);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(count, 3);

    Integer* i = nullptr;
    err = List_popFront(list, (BaseObject**) &i);
    ASSERT_EQ(err, 0);
    Int value = 0;
    err = Integer_getValue(i, &value);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(value, 1);

    BaseObject_releaseRef(i);
    i = nullptr;

    err = List_removeAt(list, 1, (BaseObject**) &i);
    ASSERT_EQ(err, 0);
    err = Integer_getValue(i, &value);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(value, 3);

    BaseObject_releaseRef(i);

    err = List_clear(list);
    ASSERT_EQ(err, 0);
    err = List_getCount(list, &count);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(count, 0);

    BaseObject_releaseRef(list);
    BaseObject_releaseRef(i3);
    BaseObject_releaseRef(i2);
    BaseObject_releaseRef(i1);
}

TEST_F(CCoretypesTest, Number)
{
    FloatObject* f1 = nullptr;
    Float f = 2.2;
    ErrCode err = 0;
    Number* n1 = nullptr;
    Int i = 0;

    err = FloatObject_createFloat(&f1, f);
    ASSERT_EQ(err, 0);
    err = BaseObject_borrowInterface(f1, NUMBER_INTF_ID, reinterpret_cast<BaseObject**>(&n1));
    ASSERT_EQ(err, 0);
    Number_getIntValue(n1, &i);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(i, 2);
    BaseObject_releaseRef(f1);
}

typedef ErrCode (*ProcCall)(BaseObject*);

static Bool b2 = False;
static ErrCode proc_call(BaseObject*)
{
    b2 = True;
    return 0;
}

TEST_F(CCoretypesTest, Procedure)
{
    Procedure* p = nullptr;
    ErrCode err = 0;
    err = Procedure_createProcedure(&p, proc_call);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(b2, False);
    err = Procedure_dispatch(p, nullptr);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(b2, True);
    BaseObject_releaseRef(p);
}

TEST_F(CCoretypesTest, Ratio)
{
    Ratio* r = nullptr;
    ErrCode err = 0;

    err = Ratio_createRatio(&r, 1, 2);
    ASSERT_EQ(err, 0);

    Int numerator = 0;
    Int denominator = 0;
    err = Ratio_getNumerator(r, &numerator);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(numerator, 1);
    err = Ratio_getDenominator(r, &denominator);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(denominator, 2);
    BaseObject_releaseRef(r);
}

TEST_F(CCoretypesTest, Serializable)
{
    List* list = nullptr;
    Serializable* s = nullptr;
    Serializer* serializer = nullptr;
    ErrCode err = 0;

    err = Serializer_createJsonSerializer(&serializer, False);
    ASSERT_EQ(err, 0);
    err = List_createList(&list);
    ASSERT_EQ(err, 0);
    err = BaseObject_borrowInterface(list, SERIALIZABLE_INTF_ID, reinterpret_cast<BaseObject**>(&s));
    ASSERT_EQ(err, 0);
    err = Serializable_serialize(s, serializer);

    String* serialized = nullptr;
    err = Serializer_getOutput(serializer, &serialized);
    ASSERT_EQ(err, 0);
    ConstCharPtr str = nullptr;
    err = String_getCharPtr(serialized, &str);
    ASSERT_EQ(err, 0);
    ASSERT_STREQ(str, "[]");

    BaseObject_releaseRef(serialized);
    BaseObject_releaseRef(serializer);
    BaseObject_releaseRef(list);
}

TEST_F(CCoretypesTest, SerializedList)
{
    // TODO: add later, cannot test now
}

TEST_F(CCoretypesTest, SerializedObject)
{
    // TODO: add later, cannot test now
}

TEST_F(CCoretypesTest, Serializer)
{
    // TODO: add later, cannot test now
}

TEST_F(CCoretypesTest, SimpleType)
{
    SimpleType* st = nullptr;
    ErrCode err = 0;
    err = SimpleType_createSimpleType(&st, CoreType::ctBool);
    ASSERT_EQ(err, 0);
    BaseObject_releaseRef(st);
}

TEST_F(CCoretypesTest, Stringobject)
{
    String* s = nullptr;
    ErrCode err = 0;
    err = String_createString(&s, "Hello");
    ASSERT_EQ(err, 0);
    ConstCharPtr str = nullptr;
    err = String_getCharPtr(s, &str);
    ASSERT_EQ(err, 0);
    ASSERT_STREQ(str, "Hello");
    BaseObject_releaseRef(s);
}

TEST_F(CCoretypesTest, Struct)
{
    // TODO: add later, cannot test now
}

TEST_F(CCoretypesTest, StructType)
{
    // TODO: add later, cannot test now
}

TEST_F(CCoretypesTest, Type)
{
    // TODO: add later, cannot test now
}

TEST_F(CCoretypesTest, TypeManager)
{
    // TODO: add later, cannot test now
}

TEST_F(CCoretypesTest, Updatable)
{
    // TODO: add later, cannot test now
}

TEST_F(CCoretypesTest, VersionInfo)
{
    VersionInfo* vi = nullptr;
    ErrCode err = 0;
    err = VersionInfo_createVersionInfo(&vi, 1, 2, 3);
    ASSERT_EQ(err, 0);
    SizeT major = 0;
    SizeT minor = 0;
    SizeT patch = 0;
    err = VersionInfo_getMajor(vi, &major);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(major, 1);
    err = VersionInfo_getMinor(vi, &minor);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(minor, 2);
    err = VersionInfo_getPatch(vi, &patch);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(patch, 3);
    BaseObject_releaseRef(vi);
}
