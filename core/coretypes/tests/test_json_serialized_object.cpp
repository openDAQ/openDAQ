#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

class JsonSerializedObjectTest : public testing::Test
{
protected:
    void SetUp() override
    {
        deserializer = JsonDeserializer();
    }

    void TearDown() override
    {
        daqUnregisterSerializerFactory(factoryId.data());
        deserializer.release();
    }

    void registerFactory(daqDeserializerFactory factory)
    {
        daqRegisterSerializerFactory(factoryId.data(), factory);
    }

    static const std::string factoryId;
    DeserializerPtr deserializer;
};

const std::string JsonSerializedObjectTest::factoryId = "test";

static ErrCode intFactory(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction*, IBaseObject** obj)
{
    Int value;
    ErrCode errCode = serialized->readInt(String("Int"), &value);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    *obj = Integer_Create(value);
    return OPENDAQ_SUCCESS;
}

static ErrCode floatFactory(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction*, IBaseObject** obj)
{
    Float value;
    ErrCode errCode = serialized->readFloat(String("Float"), &value);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    *obj = Float_Create(value);
    return OPENDAQ_SUCCESS;
}

static ErrCode boolFactory(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction*, IBaseObject** obj)
{
    Bool value;
    ErrCode errCode = serialized->readBool(String("Bool"), &value);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    *obj = Boolean_Create(value);
    return OPENDAQ_SUCCESS;
}

static ErrCode stringFactory(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction*, IBaseObject** obj)
{
    IString* value;
    ErrCode errCode = serialized->readString(String("String"), &value);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    *obj = value;
    return OPENDAQ_SUCCESS;
}

static ErrCode serializedObjectFactory(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction*, IBaseObject** obj)
{
    return serialized->readSerializedList(String("List"), reinterpret_cast<ISerializedList**>(obj));
}

static ErrCode listFactory(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction*, IBaseObject** obj)
{
    IList* list;
    ErrCode errCode = serialized->readList(String("List"), nullptr, nullptr, &list);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    *obj = list;
    return OPENDAQ_SUCCESS;
}

TEST_F(JsonSerializedObjectTest, readIntPositive)
{
    registerFactory(intFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","Int":1})";
    IntPtr ptr = deserializer.deserialize(json.data());

    Int intVal = ptr;

    ASSERT_EQ(intVal, 1);
}

TEST_F(JsonSerializedObjectTest, readIntNegative)
{
    registerFactory(intFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","Int":-1})";
    IntPtr ptr = deserializer.deserialize(json.data());

    Int intVal = ptr;

    ASSERT_EQ(intVal, -1);
}

TEST_F(JsonSerializedObjectTest, readIntInvalidType)
{
    registerFactory(intFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","Int":1.0})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

TEST_F(JsonSerializedObjectTest, readNonExistentInt)
{
    registerFactory(intFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","Integer":1})";
    ASSERT_THROW(deserializer.deserialize(json.data()), NotFoundException);
}

TEST_F(JsonSerializedObjectTest, readFloatPositive)
{
    registerFactory(floatFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","Float":1.5})";
    Float floatVal = deserializer.deserialize(json.data());

    ASSERT_EQ(floatVal, 1.5);
}

TEST_F(JsonSerializedObjectTest, readFloatNegative)
{
    registerFactory(floatFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","Float":-1.5})";
    Float floatVal = deserializer.deserialize(json.data());

    ASSERT_EQ(floatVal, -1.5);
}

TEST_F(JsonSerializedObjectTest, readNonExistentFloat)
{
    registerFactory(floatFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","Floating":1.0})";
    ASSERT_THROW(deserializer.deserialize(json.data()), NotFoundException);
}

TEST_F(JsonSerializedObjectTest, readFloatInvalidType)
{
    registerFactory(floatFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","Float":1})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

TEST_F(JsonSerializedObjectTest, readBoolTrue)
{
    registerFactory(boolFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","Bool":true})";
    BooleanPtr ptr = deserializer.deserialize(json.data());

    Bool boolVal = ptr;
    ASSERT_EQ(boolVal, True);
}

TEST_F(JsonSerializedObjectTest, readBoolFalse)
{
    registerFactory(boolFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","Bool":false})";
    BooleanPtr ptr = deserializer.deserialize(json.data());

    Bool boolVal = ptr;
    ASSERT_EQ(boolVal, False);
}

TEST_F(JsonSerializedObjectTest, readBoolInvalidType)
{
    registerFactory(boolFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","Bool":1})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

TEST_F(JsonSerializedObjectTest, readNonExistentBool)
{
    registerFactory(boolFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","Boolean":true})";
    ASSERT_THROW(deserializer.deserialize(json.data()), NotFoundException);
}

TEST_F(JsonSerializedObjectTest, readString)
{
    registerFactory(stringFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","String":"Test"})";
    StringPtr ptr = deserializer.deserialize(json.data());

    std::string strVal = ptr.toStdString();
    ASSERT_EQ(strVal, "Test");
}

TEST_F(JsonSerializedObjectTest, readStringInvalidType)
{
    registerFactory(stringFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","String":0})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

TEST_F(JsonSerializedObjectTest, readNonExistentString)
{
    registerFactory(stringFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","str":"Test"})";
    ASSERT_THROW(deserializer.deserialize(json.data()), NotFoundException);
}

TEST_F(JsonSerializedObjectTest, testHasKeyTrue)
{
    registerFactory(
        [](ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj) -> ErrCode
    {
        Bool hasKey;
        ErrCode errCode = serialized->hasKey(String("str"), &hasKey);

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        *obj = Boolean_Create(hasKey);
        return OPENDAQ_SUCCESS;
    });

    std::string json = R"({"__type":")" + factoryId + R"(","str":"Test"})";
    BooleanPtr boolPtr = deserializer.deserialize(json.data());
    Bool hasKey = boolPtr;

    ASSERT_EQ(hasKey, True);
}

TEST_F(JsonSerializedObjectTest, testHasKeyFalse)
{
    registerFactory(
        [](ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj) -> ErrCode
    {
        Bool hasKey;
        ErrCode errCode = serialized->hasKey(String("str"), &hasKey);

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        *obj = Boolean_Create(hasKey);
        return OPENDAQ_SUCCESS;
    });

    std::string json = R"({"__type":")" + factoryId + R"(","String":"Test"})";
    BooleanPtr boolPtr = deserializer.deserialize(json.data());
    Bool hasKey = boolPtr;

    ASSERT_EQ(hasKey, False);
}

TEST_F(JsonSerializedObjectTest, readEmptyObjectKeys)
{
    registerFactory(
        [](ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj) -> ErrCode
    {
        ISerializedObject* serializedObj;
        ErrCode errCode = serialized->readSerializedObject(String("Object"), &serializedObj);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        IList* keys;
        errCode = serializedObj->getKeys(&keys);
        if (OPENDAQ_FAILED(errCode))
        {
            serializedObj->releaseRef();
            return errCode;
        }

        serializedObj->releaseRef();
        *obj = keys;
        return OPENDAQ_SUCCESS;
    });

    std::string json = R"({"__type":")" + factoryId + R"(","Object":{}})";
    ListPtr<IString> keys = deserializer.deserialize(json.data());

    ASSERT_EQ(keys.getCount(), 0u);
}

TEST_F(JsonSerializedObjectTest, readObjectKeys)
{
    registerFactory(
        [](ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj) -> ErrCode
    {
        ISerializedObject* serializedObj;
        ErrCode errCode = serialized->readSerializedObject(String("Object"), &serializedObj);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        IList* keys;
        errCode = serializedObj->getKeys(&keys);
        if (OPENDAQ_FAILED(errCode))
        {
            serializedObj->releaseRef();
            return errCode;
        }

        serializedObj->releaseRef();
        *obj = keys;
        return OPENDAQ_SUCCESS;
    });

    std::string json = R"({"__type":")" + factoryId +
                       R"(","Object":{"key1":1,"key2":0.0,"key3":false,"key4":"String","key5":[],"key6":{}}})";
    ListPtr<IString> keys = deserializer.deserialize(json.data());

    ASSERT_EQ(keys.getCount(), 6u);
}

TEST_F(JsonSerializedObjectTest, readNonExistentObject)
{
    registerFactory(
        [](ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj) -> ErrCode
    {
        ErrCode errCode = serialized->readObject(String("doesNotExist"), nullptr, nullptr, obj);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
        return OPENDAQ_SUCCESS;
    });

    std::string json = R"({"__type":")" + factoryId + R"(","Object":{}})";
    ASSERT_THROW(deserializer.deserialize(json.data()), NotFoundException);
}

TEST_F(JsonSerializedObjectTest, readSerializedObjectInvalidType)
{
    registerFactory(
        [](ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** /*obj*/) -> ErrCode
    {
        ISerializedObject* serializedObj;
        ErrCode errCode = serialized->readSerializedObject(String("Object"), &serializedObj);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        serializedObj->releaseRef();
        return OPENDAQ_SUCCESS;
    });

    std::string json = R"({"__type":")" + factoryId + R"(","Object":[]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

TEST_F(JsonSerializedObjectTest, readNonExistentSerializedObject)
{
    registerFactory(
        [](ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** /*obj*/) -> ErrCode
    {
        ISerializedObject* serializedObj;
        ErrCode errCode = serialized->readSerializedObject(String("Object"), &serializedObj);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        serializedObj->releaseRef();
        return OPENDAQ_SUCCESS;
    });

    std::string json = R"({"__type":")" + factoryId + R"(","obj":{}})";
    ASSERT_THROW(deserializer.deserialize(json.data()), NotFoundException);
}

TEST_F(JsonSerializedObjectTest, readEmptySerializedList)
{
    registerFactory(serializedObjectFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","List":[]})";
    SerializedListPtr listPtr = deserializer.deserialize(json.data());

    SizeT size = listPtr.getCount();
    ASSERT_EQ(size, 0u);
}

TEST_F(JsonSerializedObjectTest, readNonExistingSerializedList)
{
    registerFactory(serializedObjectFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","array":[]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), NotFoundException);
}

TEST_F(JsonSerializedObjectTest, readSerializedListInvalidType)
{
    registerFactory(serializedObjectFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","List":false})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

TEST_F(JsonSerializedObjectTest, readEmptyList)
{
    registerFactory(listFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","List":[]})";
    ListPtr<IBaseObject> listPtr = deserializer.deserialize(json.data());

    SizeT size = listPtr.getCount();
    ASSERT_EQ(size, 0u);
}

TEST_F(JsonSerializedObjectTest, readNonExistingList)
{
    registerFactory(listFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","array":[]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), NotFoundException);
}

TEST_F(JsonSerializedObjectTest, readListInvalidType)
{
    registerFactory(listFactory);

    std::string json = R"({"__type":")" + factoryId + R"(","List":false})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("ISerializedObject", "daq");

TEST_F(JsonSerializedObjectTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, ISerializedObject::Id);
}

TEST_F(JsonSerializedObjectTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<ISerializedObject>(), "{EC052FCE-7ADC-5335-9929-66731EA35698}");
}
