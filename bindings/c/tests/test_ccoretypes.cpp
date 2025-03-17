#include <copendaq.h>
#include <gtest/gtest.h>
#include "ccommon.h"
#include "ccoretypes/base_object.h"

using CCoretypesTest = testing::Test;

TEST_F(CCoretypesTest, BaseObject)
{
    BaseObject* obj = nullptr;
    BaseObject_create(&obj);
    ASSERT_NE(obj, nullptr);
    BaseObject_releaseRef(obj);
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
    // TODO: add err
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
    // TODO: add later, cannot test now
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
    // TODO: add later, cannot test now
}

TEST_F(CCoretypesTest, EventArgs)
{
    // TODO: add later, cannot test now
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
    // TODO: add later, cannot test now
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
    // TODO: add later, cannot test now
}

TEST_F(CCoretypesTest, Iterator)
{
    // TODO: add later, cannot test now
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
    // TODO: add later, cannot test now
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
    // TODO: add later, cannot test now
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
    // TODO: add later, cannot test now
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
