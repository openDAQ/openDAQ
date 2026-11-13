#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

namespace test_json_serialized_list
{


// TODO: Test suite might fail due to memory leaks if registering a new factory requires factory map to allocate more memory

class JsonSerializedListTest : public testing::Test
{
protected:
    static const std::string FactoryId;

    void SetUp() override
    {
        deserializer = JsonDeserializer();
    }

    void TearDown() override
    {
        const ErrCode errCode = daqUnregisterSerializerFactory(FactoryId.data());
        if (OPENDAQ_FAILED(errCode))
            daqClearErrorInfo();
        deserializer.release();
    }

    void registerFactory(daqDeserializerFactory factory)
    {
        const ErrCode errCode = daqRegisterSerializerFactory(FactoryId.data(), factory);
        if (OPENDAQ_FAILED(errCode))
            daqClearErrorInfo();
    }

    DeserializerPtr deserializer;
};

const std::string JsonSerializedListTest::FactoryId = "test";

static ErrCode objectFactory(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    SerializedListPtr serializedList;
    ErrCode errCode = serialized->readSerializedList(String("List"), &serializedList);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    IBaseObject* baseObj;
    errCode = serializedList->readObject(nullptr, nullptr, &baseObj);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    *obj = baseObj;

    return OPENDAQ_SUCCESS;
}

static ErrCode serializedObjectFactory(ISerializedObject* serialized,
                                       IBaseObject* /*context*/,
                                       IFunction* /*factoryCallback*/,
                                       IBaseObject** obj)
{
    SerializedListPtr list;
    ErrCode errCode = serialized->readSerializedList(String("List"), &list);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    return list->readSerializedObject(reinterpret_cast<ISerializedObject**>(obj));
}

static ErrCode listFactory(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    SerializedListPtr serializedList;
    ErrCode errCode = serialized->readSerializedList(String("List"), &serializedList);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    IList* list;
    errCode = serializedList->readList(nullptr, nullptr, &list);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    *obj = list;
    return OPENDAQ_SUCCESS;
}

static ErrCode boolFactory(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    SerializedListPtr serializedList;
    ErrCode errCode = serialized->readSerializedList(String("List"), &serializedList);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    Bool boolean;
    errCode = serializedList->readBool(&boolean);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    *obj = Boolean_Create(boolean);

    return OPENDAQ_SUCCESS;
}

static ErrCode intFactory(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    SerializedListPtr serializedList;
    ErrCode errCode = serialized->readSerializedList(String("List"), &serializedList);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    Int integer;
    errCode = serializedList->readInt(&integer);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    *obj = Integer_Create(integer);

    return OPENDAQ_SUCCESS;
}

static ErrCode floatFactory(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    SerializedListPtr serializedList;
    ErrCode errCode = serialized->readSerializedList(String("List"), &serializedList);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    Float floating;
    errCode = serializedList->readFloat(&floating);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    *obj = Float_Create(floating);

    return OPENDAQ_SUCCESS;
}

static ErrCode stringFactory(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    SerializedListPtr serializedList;
    ErrCode errCode = serialized->readSerializedList(String("List"), &serializedList);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    IString* string;
    errCode = serializedList->readString(&string);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    *obj = string;

    return OPENDAQ_SUCCESS;
}

static ErrCode serializedListFactory(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    SerializedListPtr serializedList;
    ErrCode errCode = serialized->readSerializedList(String("List"), &serializedList);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    ISerializedList* innerList;
    errCode = serializedList->readSerializedList(&innerList);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    *obj = innerList;

    return OPENDAQ_SUCCESS;
}

TEST_F(JsonSerializedListTest, NullObjectInvalidArgumentException)
{
    SerializedListPtr ptr;

    ASSERT_THROW(ptr.readSerializedObject(), InvalidParameterException);
}

TEST_F(JsonSerializedListTest, readSerializedListObjectNull)
{
    SerializedObjectPtr ptr;
    ASSERT_THROW(ptr.readSerializedList("List"), InvalidParameterException);
}

TEST_F(JsonSerializedListTest, readSerializedObjectInvalidValue)
{
    registerFactory(serializedObjectFactory);

    std::string json = R"({"__type":"test","List":[false]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

TEST_F(JsonSerializedListTest, readSerializedObjectOutOfRange)
{
    registerFactory(serializedObjectFactory);

    std::string json = R"({"__type":"test","List":[]})";
    ASSERT_THROW(deserializer.deserialize(json), OutOfRangeException);
}

TEST_F(JsonSerializedListTest, readSerializedObjectNull)
{
    registerFactory(serializedObjectFactory);

    std::string json = R"({"__type":"test","List":[{"__type":1}]})";
    BaseObjectPtr ptr = deserializer.deserialize(json);

    ASSERT_TRUE(ptr.assigned());
}

TEST_F(JsonSerializedListTest, readListInvalidType)
{
    registerFactory(listFactory);

    std::string json = R"({"__type":"test","List":[false]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

TEST_F(JsonSerializedListTest, readList)
{
    registerFactory(listFactory);

    std::string json = R"({"__type":"test","List":[[]]})";
    ListPtr<IBaseObject> listPtr = deserializer.deserialize(json.data());

    ASSERT_EQ(listPtr.getCount(), 0u);
}

TEST_F(JsonSerializedListTest, readListOutOfRange)
{
    registerFactory(listFactory);

    std::string json = R"({"__type":"test","List":[]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), OutOfRangeException);
}

TEST_F(JsonSerializedListTest, readSerializedListInvalidType)
{
    registerFactory(serializedListFactory);

    std::string json = R"({"__type":"test","List":[{}]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

TEST_F(JsonSerializedListTest, readSerializedList)
{
    registerFactory(serializedListFactory);

    std::string json = R"({"__type":"test","List":[[]]})";
    SerializedListPtr listPtr = deserializer.deserialize(json.data());

    ASSERT_EQ(listPtr.getCount(), 0u);
}

TEST_F(JsonSerializedListTest, readSerializedListOutOfRange)
{
    registerFactory(serializedListFactory);

    std::string json = R"({"__type":"test","List":[]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), OutOfRangeException);
}

TEST_F(JsonSerializedListTest, readBoolTrue)
{
    registerFactory(boolFactory);

    std::string json = R"({"__type":"test","List":[true]})";
    BooleanPtr ptr = deserializer.deserialize(json.data());

    Bool boolVal = ptr;
    ASSERT_EQ(boolVal, True);
}

TEST_F(JsonSerializedListTest, readBoolFalse)
{
    registerFactory(boolFactory);

    std::string json = R"({"__type":"test","List":[false]})";
    BooleanPtr ptr = deserializer.deserialize(json.data());

    Bool boolVal = ptr;
    ASSERT_EQ(boolVal, False);
}

TEST_F(JsonSerializedListTest, readBoolInvalidType)
{
    registerFactory(boolFactory);

    std::string json = R"({"__type":"test","List":[1]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

TEST_F(JsonSerializedListTest, readBoolOutOfRange)
{
    registerFactory(boolFactory);

    std::string json = R"({"__type":"test","List":[]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), OutOfRangeException);
}

TEST_F(JsonSerializedListTest, readIntPositive)
{
    registerFactory(intFactory);

    std::string json = R"({"__type":"test","List":[1]})";
    IntPtr ptr = deserializer.deserialize(json.data());

    Int intVal = ptr;

    ASSERT_EQ(intVal, 1);
}

TEST_F(JsonSerializedListTest, readIntNegative)
{
    registerFactory(intFactory);

    std::string json = R"({"__type":"test","List":[-1]})";
    IntPtr ptr = deserializer.deserialize(json.data());

    Int intVal = ptr;

    ASSERT_EQ(intVal, -1);
}

TEST_F(JsonSerializedListTest, readIntInvalidType)
{
    registerFactory(intFactory);

    std::string json = R"({"__type":"test","List":[1.0]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

TEST_F(JsonSerializedListTest, readIntOutOfRange)
{
    registerFactory(intFactory);

    std::string json = R"({"__type":"test","List":[]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), OutOfRangeException);
}

TEST_F(JsonSerializedListTest, readFloatPositive)
{
    registerFactory(floatFactory);

    std::string json = R"({"__type":"test","List":[1.5]})";
    Float floatVal = deserializer.deserialize(json.data());
    ASSERT_EQ(floatVal, 1.5);
}

TEST_F(JsonSerializedListTest, readFloatNegative)
{
    registerFactory(floatFactory);

    std::string json = R"({"__type":"test","List":[-1.5]})";
    Float floatVal = deserializer.deserialize(json.data());

    ASSERT_EQ(floatVal, -1.5);
}

TEST_F(JsonSerializedListTest, readNonExistentFloat)
{
    registerFactory(floatFactory);

    std::string json = R"({"__type":"test","List":[]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), OutOfRangeException);
}

TEST_F(JsonSerializedListTest, readFloatInvalidType)
{
    registerFactory(floatFactory);

    std::string json = R"({"__type":"test","List":[1]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

TEST_F(JsonSerializedListTest, readString)
{
    registerFactory(stringFactory);

    std::string json = R"({"__type":"test","List":["Test"]})";
    StringPtr ptr = deserializer.deserialize(json.data());

    std::string strVal = ptr.toStdString();
    ASSERT_EQ(strVal, "Test");
}

TEST_F(JsonSerializedListTest, readStringInvalidType)
{
    registerFactory(stringFactory);

    std::string json = R"({"__type":"test","List":[0]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), InvalidTypeException);
}

TEST_F(JsonSerializedListTest, readStringOutOfRange)
{
    registerFactory(stringFactory);

    std::string json = R"({"__type":"test","List":[]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), OutOfRangeException);
}

TEST_F(JsonSerializedListTest, readObjectOutOfRange)
{
    registerFactory(objectFactory);

    std::string json = R"({"__type":")" + FactoryId + R"(","List":[]})";
    ASSERT_THROW(deserializer.deserialize(json.data()), OutOfRangeException);
}

TEST_F(JsonSerializedListTest, readObject)
{
    daqRegisterSerializerFactory(FactoryId.data(),
        [](ISerializedObject* /*serialized*/, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj) -> ErrCode
                                  {
                                      *obj = nullptr;
                                      return OPENDAQ_SUCCESS;
                                  });

    std::string json = R"({"__type":")" + FactoryId + R"(","List":[{"__type":"null"}]})";

    BaseObjectPtr ptr = deserializer.deserialize(String(json.data()));
    ASSERT_FALSE(ptr.assigned());
}

TEST_F(JsonSerializedListTest, Inspectable)
{
    registerFactory(serializedListFactory);

    std::string json = R"({"__type":"test","List":[[]]})";
    SerializedListPtr obj = deserializer.deserialize(json.data());

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], ISerializedList::Id);
}

TEST_F(JsonSerializedListTest, ImplementationName)
{
    registerFactory(serializedListFactory);

    std::string json = R"({"__type":"test","List":[[]]})";
    SerializedListPtr obj = deserializer.deserialize(json.data());

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::JsonSerializedList");
}

static constexpr auto JSON_SERIALIZED_LIST_INTERFACE_ID = FromTemplatedTypeName("ISerializedList", "daq");

TEST_F(JsonSerializedListTest, InterfaceId)
{
    ASSERT_EQ(JSON_SERIALIZED_LIST_INTERFACE_ID, ISerializedList::Id);
}

TEST_F(JsonSerializedListTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<ISerializedList>(), "{A9E1FD59-8AD5-5F3C-B4F8-2A9CDE66E598}");
}

// Runs `read` on the serialized list stored under "List"
static void withList(const DeserializerPtr& deserializer, const std::string& json, const std::function<void(const SerializedListPtr&)>& read)
{
    const auto proc = Procedure([&read](const BaseObjectPtr& serialized)
    {
        const auto object = serialized.asPtr<ISerializedObject, SerializedObjectPtr>(true);
        read(object.readSerializedList("List"));
    });

    deserializer.callCustomProc(proc, json);
}

TEST_F(JsonSerializedListTest, readInt64)
{
    withList(deserializer, R"({"List":[9007199254740993,-9223372036854775807,1]})", [](const SerializedListPtr& list)
    {
        ASSERT_EQ(list.getCurrentItemType(), ctInt);
        ASSERT_EQ(list.readInt(), 9007199254740993);
        ASSERT_EQ(list.readInt(), -9223372036854775807);
        ASSERT_EQ(list.readInt(), 1);
    });
}

TEST_F(JsonSerializedListTest, readSerializedObjectNullMovesToNextItem)
{
    withList(deserializer, R"({"List":[null,{"key":1}]})", [](const SerializedListPtr& list)
    {
        ASSERT_FALSE(list.readSerializedObject().assigned());

        const auto object = list.readSerializedObject();
        ASSERT_TRUE(object.assigned());

        Int value = 0;
        ASSERT_EQ(object->readInt(String("key"), &value), OPENDAQ_SUCCESS);
        ASSERT_EQ(value, 1);

        ASSERT_THROW(list.getCurrentItemType(), OutOfRangeException);
    });
}

TEST_F(JsonSerializedListTest, readStringNullMovesToNextItem)
{
    withList(deserializer, R"({"List":[null,"text"]})", [](const SerializedListPtr& list)
    {
        ASSERT_FALSE(list.readString().assigned());
        ASSERT_EQ(list.readString(), "text");
        ASSERT_THROW(list.getCurrentItemType(), OutOfRangeException);
    });
}

TEST_F(JsonSerializedListTest, readListNullMovesToNextItem)
{
    withList(deserializer, R"({"List":[null,[1,2]]})", [](const SerializedListPtr& list)
    {
        ASSERT_FALSE(list.readList<IBaseObject>().assigned());

        const ListPtr<IBaseObject> items = list.readList<IBaseObject>();
        ASSERT_EQ(items.getCount(), 2u);

        ASSERT_THROW(list.getCurrentItemType(), OutOfRangeException);
    });
}
}
// namespace test_json_serialized_list
