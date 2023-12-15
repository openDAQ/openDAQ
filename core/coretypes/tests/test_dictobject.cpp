#include <gtest/gtest.h>
#include <coretypes/coretypes.h>
#include <coretypes/dict_element_type.h>
#include <gmock/gmock-matchers.h>

using DictObjectTest = testing::Test;

DECLARE_OPENDAQ_INTERFACE(ITestObject, daq::IBaseObject)
{
};

using namespace daq;

class TestObjectImpl : public ImplementationOf<ITestObject>
{
public:
    TestObjectImpl(const size_t hash)
        : hash(hash)
    {
    }

    ErrCode INTERFACE_FUNC getHashCode(SizeT* hashCode) override
    {
        *hashCode = hash;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equals) const override
    {
        *equals = false;
        ITestObject* otherObj;

        if (OPENDAQ_SUCCEEDED(other->queryInterface(ITestObject::Id, reinterpret_cast<void**>(&otherObj))))
        {
            size_t otherHash;
            otherObj->getHashCode(&otherHash);

            *equals = otherHash == hash;
            otherObj->releaseRef();
        }

        return OPENDAQ_SUCCESS;
    }

private:
    size_t hash;
};

static ObjectPtr<ITestObject> TestObj(size_t hash)
{
    ITestObject* testObject = new TestObjectImpl(hash);
    testObject->addRef();
    ObjectPtr<ITestObject> testObj(std::move(testObject));
    return testObj;
}

using TestObjectPtr = ObjectPtr<ITestObject>;

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IDict", "daq");

TEST_F(DictObjectTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IDict::Id);
}

TEST_F(DictObjectTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IDict>(), "{E3DE60DA-0366-5DA5-8334-F9DCADFF5AD0}");
}

TEST_F(DictObjectTest, Setting)
{
    auto dict = Dict<IBaseObject, IBaseObject>();

    dict.set(TestObj(1), BaseObject());
    ASSERT_EQ(dict.getCount(), 1u);
}

TEST_F(DictObjectTest, Getting)
{
    auto dict = Dict<IBaseObject, IBaseObject>();

    auto obj = BaseObject();
    dict.set(TestObj(1), obj);

    auto obj1 = dict.get(TestObj(1));
    ASSERT_EQ(obj, obj1);
}

TEST_F(DictObjectTest, TryGet)
{
    auto dict = Dict<IBaseObject, IBaseObject>();

    auto obj = BaseObject();
    dict.set(TestObj(1), obj);

    BaseObjectPtr ptr;
    ASSERT_TRUE(dict.tryGet(TestObj(1), ptr));
    ASSERT_EQ(ptr, obj);
}

TEST_F(DictObjectTest, TryGetFalse)
{
    auto dict = Dict<IBaseObject, IBaseObject>();

    auto obj = BaseObject();
    dict.set(TestObj(1), obj);

    BaseObjectPtr ptr;
    ASSERT_FALSE(dict.tryGet(TestObj(2), ptr));
    ASSERT_FALSE(ptr.assigned());
}

TEST_F(DictObjectTest, TryGetCustomType)
{
    auto dict = Dict<IString, IString>();

    StringPtr str = "value";
    dict.set(StringPtr("key"), str);

    StringPtr ptr;
    ASSERT_TRUE(dict.tryGet(StringPtr("key"), ptr));
    ASSERT_EQ(ptr, "value");
}

TEST_F(DictObjectTest, TryGetCustomTypeFalse)
{
    auto dict = Dict<IString, IString>();

    StringPtr str = "value";
    dict.set(StringPtr("key"), str);

    StringPtr ptr;
    ASSERT_FALSE(dict.tryGet(StringPtr("non-existent"), ptr));
    ASSERT_FALSE(ptr.assigned());
}

TEST_F(DictObjectTest, Updating)
{
    auto dict = Dict<IBaseObject, IBaseObject>();

    dict.set(TestObj(1), BaseObject());
    dict.set(TestObj(2), BaseObject());

    auto obj = BaseObject();
    dict.set(TestObj(2), obj);

    auto obj1 = dict.get(TestObj(2));
    ASSERT_EQ(obj, obj1);

    ASSERT_EQ(dict.getCount(), 2u);
}

TEST_F(DictObjectTest, Clearing)
{
    auto dict = Dict<IBaseObject, IBaseObject>();

    dict.set(TestObj(1), BaseObject());
    dict.set(TestObj(2), BaseObject());

    dict.clear();

    ASSERT_EQ(dict.getCount(), 0u);
}

TEST_F(DictObjectTest, Removing)
{
    auto dict = Dict<IBaseObject, IBaseObject>();

    dict.set(TestObj(1), BaseObject());
    dict.remove(TestObj(1));
    ASSERT_EQ(dict.getCount(), 0u);
}

TEST_F(DictObjectTest, Delete)
{
    auto dict = Dict<IBaseObject, IBaseObject>();

    dict.set(TestObj(1), BaseObject());
    dict.deleteItem(TestObj(1));
    ASSERT_EQ(dict.getCount(), 0u);
}

TEST_F(DictObjectTest, EmptyValues)
{
    auto dict = Dict<IBaseObject, IBaseObject>();

    dict.set(TestObj(1), nullptr);
    dict.set(TestObj(2), nullptr);
    dict.set(TestObj(3), nullptr);

    auto obj = dict.get(TestObj(1));
    ASSERT_EQ(obj, nullptr);

    dict.deleteItem(TestObj(1));
    dict.remove(TestObj(2));

    dict.clear();
}

TEST_F(DictObjectTest, EmptyKey)
{
    auto dict = Dict<IBaseObject, IBaseObject>();

    ASSERT_ANY_THROW(dict.set(nullptr, nullptr));
    ASSERT_ANY_THROW(dict.get(nullptr));
    ASSERT_ANY_THROW(dict.deleteItem(nullptr));
    ASSERT_ANY_THROW(dict.remove(nullptr));
}

TEST_F(DictObjectTest, NotFound)
{
    auto dict = Dict<IBaseObject, IBaseObject>();

    ASSERT_ANY_THROW(dict.get(TestObj(1)));
    ASSERT_ANY_THROW(dict.deleteItem(TestObj(1)));
    ASSERT_ANY_THROW(dict.remove(TestObj(1)));
}

TEST_F(DictObjectTest, UnassignedDict)
{
    DictObjectPtr<IDict, IBaseObject, IBaseObject> dict;

    ASSERT_ANY_THROW(dict.get(TestObj(1)));
    ASSERT_ANY_THROW(dict.set(TestObj(1), TestObj(1)));
    ASSERT_ANY_THROW(dict.deleteItem(TestObj(1)));
    ASSERT_ANY_THROW(dict.remove(TestObj(1)));
    ASSERT_ANY_THROW(dict.clear());
    ASSERT_ANY_THROW(dict.getCount());
}

TEST_F(DictObjectTest, Freeze)
{
    auto dict = Dict<IBaseObject, IBaseObject>();
    dict.freeze();

    ASSERT_TRUE(dict.isFrozen());
}

TEST_F(DictObjectTest, SetWhenFrozen)
{
    auto dict = Dict<IBaseObject, IBaseObject>();
    dict.freeze();

    ASSERT_THROW(dict.set(nullptr, nullptr), FrozenException);
}

TEST_F(DictObjectTest, RemoveWhenFrozen)
{
    auto dict = Dict<IBaseObject, IBaseObject>();
    dict.freeze();

    ASSERT_THROW(dict.remove(nullptr), FrozenException);
}

TEST_F(DictObjectTest, DeleteWhenFrozen)
{
    auto dict = Dict<IBaseObject, IBaseObject>();
    dict.freeze();

    ASSERT_THROW(dict.deleteItem(nullptr), FrozenException);
}

TEST_F(DictObjectTest, ClearWhenFrozen)
{
    auto dict = Dict<IBaseObject, IBaseObject>();
    dict.freeze();

    ASSERT_THROW(dict.clear(), FrozenException);
}

TEST_F(DictObjectTest, IsFrozenFalse)
{
    auto dict = Dict<IBaseObject, IBaseObject>();

    ASSERT_FALSE(dict.isFrozen());
}

TEST_F(DictObjectTest, DoubleFreeze)
{
    auto dict = Dict<IBaseObject, IBaseObject>();
    dict.freeze();

    ASSERT_NO_THROW(dict.freeze());
    ASSERT_EQ(dict.asPtr<IFreezable>(true)->freeze(), OPENDAQ_IGNORED);
}

TEST_F(DictObjectTest, SerializeJson)
{
    auto dict = Dict<IBaseObject, IBaseObject>();
    dict.set(L"Test0", L"Value0");
    dict.set(L"Test1", L"Value1");
    dict.set(L"Test2", L"Value2");
    dict.set(L"Test3", L"Value3");

    SerializerPtr ser = JsonSerializer();
    dict.serialize(ser);

    StringPtr str = ser.getOutput();

    DeserializerPtr dser = JsonDeserializer();

    IBaseObject* deserialized = nullptr;
    dser->deserialize(str, nullptr, nullptr, &deserialized);

    IDict* deserializedDict;
    ErrCode err = deserialized->borrowInterface(IDict::Id, (void**) &deserializedDict);
    ASSERT_EQ(err, OPENDAQ_SUCCESS);

    StringPtr value0;
    deserializedDict->get(String("Test0"), (IBaseObject**) &value0);
    ASSERT_STREQ(value0.getCharPtr(), "Value0");

    StringPtr value1;
    deserializedDict->get(String("Test1"), (IBaseObject**) &value1);
    ASSERT_STREQ(value1.getCharPtr(), "Value1");

    StringPtr value2;
    deserializedDict->get(String("Test2"), (IBaseObject**) &value2);
    ASSERT_STREQ(value2.getCharPtr(), "Value2");

    StringPtr value3;
    deserializedDict->get(String("Test3"), (IBaseObject**) &value3);
    ASSERT_STREQ(value3.getCharPtr(), "Value3");

    deserialized->releaseRef();
}

TEST_F(DictObjectTest, SerializeJsonNullValue)
{
    auto dict = Dict<IBaseObject, IBaseObject>();
    dict.set(L"Test0", nullptr);

    SerializerPtr ser = JsonSerializer();
    dict.serialize(ser);

    StringPtr str;
    ser->getOutput(&str);

    const auto expected = R"({"__type":")"
                          + std::string(dict.getSerializeId())
                          + R"(","values":[{"key":"Test0","value":null}]})";

    ASSERT_EQ(str.toStdString(), expected);
}

TEST_F(DictObjectTest, SerializeJsonDictValueNotSerializable)
{
    auto dict = Dict<IBaseObject, IBaseObject>();
    dict.set(L"Test0", BaseObject());

    SerializerPtr ser = JsonSerializer();
    ASSERT_THROW(dict.serialize(ser), NotSerializableException);
}

TEST_F(DictObjectTest, SerializeJsonDictKeyNotSerializable)
{
    auto dict = Dict<IBaseObject, IBaseObject>();
    dict.set(BaseObject(), "Test0");

    SerializerPtr ser = JsonSerializer();
    ASSERT_THROW(dict.serialize(ser), NotSerializableException);
}

TEST_F(DictObjectTest, SerializeJsonEmptyDictionary)
{
    SerializerPtr serializer = JsonSerializer();

    auto dictionary = Dict<IBaseObject, IBaseObject>();
    dictionary.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized,
              R"({"__type":")" +
              std::string(dictionary.getSerializeId()) +
              R"(","values":[]})"
    );
}

TEST_F(DictObjectTest, DeserializeJson)
{
    std::vector<std::string> keys =
    {
        "Test0",
        "Test1",
        "Test2",
        "Test3",
    };

    std::vector<std::string> values =
    {
        "Value0",
        "Value1",
        "Value2",
        "Value3",
    };

    auto dict = Dict<IBaseObject, IBaseObject>();
    dict.set(keys[0], values[0]);
    dict.set(keys[1], values[1]);
    dict.set(keys[2], values[2]);
    dict.set(keys[3], values[3]);

    SerializerPtr serializer = JsonSerializer();
    dict.serialize(serializer);

    StringPtr serializedDict = serializer.getOutput();
    {
        auto deserializer = JsonDeserializer();

        IDict* deserializedDict = nullptr;
        ErrCode errCode = deserializer->deserialize(serializedDict, nullptr, nullptr, reinterpret_cast<IBaseObject**>(&deserializedDict));
        ASSERT_FALSE((bool)errCode);
        {
            StringPtr value0;
            deserializedDict->get(String(keys[0].data()), reinterpret_cast<IBaseObject**>(&value0));
            ASSERT_STREQ(value0.getCharPtr(), values[0].data());

            StringPtr value1;
            deserializedDict->get(String(keys[1].data()), reinterpret_cast<IBaseObject**>(&value1));
            ASSERT_STREQ(value1.getCharPtr(), values[1].data());

            StringPtr value2;
            deserializedDict->get(String(keys[2].data()), reinterpret_cast<IBaseObject**>(&value2));
            ASSERT_STREQ(value2.getCharPtr(), values[2].data());

            StringPtr value3;
            deserializedDict->get(String(keys[3].data()), reinterpret_cast<IBaseObject**>(&value3));
            ASSERT_STREQ(value3.getCharPtr(), values[3].data());
        }
        deserializedDict->releaseRef();
    }
}

TEST_F(DictObjectTest, DeserializeKeyErrorJson)
{
    StringPtr serializedDict = R"({"__type":"Dict","values":[{"key":{"__type":"Ratioooo","num":1,"den":2},"value":1}]})";
    auto deserializer = JsonDeserializer();

    IDict* deserializedDict = nullptr;
    ErrCode errCode = deserializer->deserialize(serializedDict, nullptr, nullptr, reinterpret_cast<IBaseObject**>(&deserializedDict));
    ASSERT_NE(errCode, OPENDAQ_SUCCESS);
}

TEST_F(DictObjectTest, DeserializeValueErrorJson)
{
    StringPtr serializedDict = R"({"__type":"Dict","values":[{"key":"Key1","value":{"__type":"Ratiooo","num":1,"den":2}}]})";
    auto deserializer = JsonDeserializer();

    IDict* deserializedDict = nullptr;
    ErrCode errCode = deserializer->deserialize(serializedDict, nullptr, nullptr, reinterpret_cast<IBaseObject**>(&deserializedDict));
    ASSERT_NE(errCode, OPENDAQ_SUCCESS);
}

TEST_F(DictObjectTest, CoreType)
{
    auto dict = Dict<IBaseObject, IBaseObject>();
    enum CoreType coreType = dict.getCoreType();

    ASSERT_EQ(coreType, ctDict);
}

TEST_F(DictObjectTest, GetKeyList)
{
    auto dict = Dict<IInteger, IString>();
    dict.set(1, "a");
    dict.set(2, "b");

    auto keys = dict.getKeyList();
    ASSERT_EQ(keys.getCount(), 2u);
    ASSERT_TRUE(keys.getItemAt(0) == 1 || keys.getItemAt(0) == 2);
    ASSERT_TRUE(keys.getItemAt(1) == 1 || keys.getItemAt(1) == 2);
    ASSERT_TRUE(keys.getItemAt(0) != keys.getItemAt(1));
}

TEST_F(DictObjectTest, GetKeys)
{
    auto dict = Dict<IInteger, IString>();
    dict.set(1, "a");
    dict.set(2, "b");

    auto keyEnumerable = dict.getKeys();

    auto start = keyEnumerable.createStartIteratorInterface();
    auto end = keyEnumerable.createEndIteratorInterface();

    ASSERT_TRUE(start != end);

    int val1 = *start;
    ASSERT_TRUE(val1 == 1 || val1 == 2);

    ++start;

    ASSERT_TRUE(start != end);

    int val2 = *start;
    ASSERT_TRUE(val2 == 1 || val2 == 2);

    ASSERT_NE(val1, val2);

    ++start;

    ASSERT_FALSE(start != end);
}

TEST_F(DictObjectTest, HasKeyTrue)
{
    auto dict = Dict<IInteger, IString>();
    dict.set(1, "a");

    ASSERT_TRUE(dict.hasKey(1));
}

TEST_F(DictObjectTest, HasKeyFalse)
{
    auto dict = Dict<IInteger, IString>();
    dict.set(1, "a");

    ASSERT_FALSE(dict.hasKey(2));
}

TEST_F(DictObjectTest, GetValueList)
{
    auto dict = Dict<IInteger, IString>();
    dict.set(1, "a");
    dict.set(2, "b");

    auto values = dict.getValueList();
    ASSERT_EQ(values.getCount(), 2u);
    ASSERT_TRUE(values.getItemAt(0) == "a" || values.getItemAt(0) == "b");
    ASSERT_TRUE(values.getItemAt(1) == "a" || values.getItemAt(1) == "b");
    ASSERT_TRUE(values.getItemAt(0) != values.getItemAt(1));
}

TEST_F(DictObjectTest, GetValues)
{
    auto dict = Dict<IInteger, IString>();
    dict.set(1, "a");
    dict.set(2, "b");

    auto valueEnumerable = dict.getValues();

    auto start = valueEnumerable.createStartIteratorInterface();
    auto end = valueEnumerable.createEndIteratorInterface();

    ASSERT_TRUE(start != end);

    std::string val1 = *start;
    ASSERT_TRUE(val1 == "a" || val1 == "b");

    ++start;

    ASSERT_TRUE(start != end);

    std::string val2 = *start;
    ASSERT_TRUE(val2 == "a" || val2 == "b");

    ASSERT_NE(val1, val2);

    ++start;

    ASSERT_FALSE(start != end);
}

TEST_F(DictObjectTest, GetItemWithSubscriptionOperator)
{
    auto dict = Dict<IInteger, IString>();
    dict.set(1, "a");
    dict.set(2, "b");

    ASSERT_EQ(dict[1], "a");
    ASSERT_EQ(dict[2], "b");
}

TEST_F(DictObjectTest, GetItemWithPtrSubscriptionOperator)
{
    auto dict = Dict<IString, IInteger>();
    dict.set("a", 1);
    dict.set("b", 2);

    StringPtr a = "a";
    StringPtr b = "b";

    ASSERT_EQ(dict[a], 1);
    ASSERT_EQ(dict[b], 2);
}

TEST(DictObjectTest, SetValueUsingOperator)
{
    auto dict = Dict<IString, IInteger>();
    dict.set("a", 1);
    dict.set("b", 2);

    dict["a"] = 5;

    ASSERT_EQ(dict.get("a"), 5);
}

TEST_F(DictObjectTest, InsertionOrder)
{
    auto dict = Dict<IInteger, IInteger>();

    static constexpr size_t N = 100;
    for (size_t i = 0; i < N; i++)
        dict[i] = i;

    //update 50th
    dict[50] = 50;

    //reinsert last
    dict.remove(N - 1);
    dict[N - 1] = N - 1;

    auto checkSequence = [](const IterablePtr<IInteger>& iterator) {
        std::vector<Int> list(iterator.begin(), iterator.end());
        for (size_t i = 0; i < list.size(); i++)
            ASSERT_EQ(list[i], static_cast<Int>(i));
    };

    checkSequence(dict.getKeyList());
    checkSequence(dict.getKeys());
    checkSequence(dict.getValueList());
    checkSequence(dict.getValues());
}

TEST_F(DictObjectTest, NonInterfaceTypeParam)
{
    auto dict = Dict<std::string, float>();
}

TEST_F(DictObjectTest, EqualsEmpty)
{
    auto d1 = Dict<IString, IBaseObject>();
    auto d2 = Dict<IString, IBaseObject>();

    Bool eq{false};
    d1->equals(d2, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(DictObjectTest, EqualsDifferentSize)
{
    auto d1 = Dict<IString, IBaseObject>();
    d1.set("name", "jon");
    d1.set("age", 20);

    auto d2 = Dict<IString, IBaseObject>();
    d2.set("name", "jon");
    d2.set("age", 20);
    d2.set("weight", 70);

    Bool eq{true};
    d1->equals(d2, &eq);
    ASSERT_FALSE(eq);
}

TEST_F(DictObjectTest, EqualsSameSize)
{
    auto d1 = Dict<IString, IBaseObject>();
    d1.set("name", "jon");
    d1.set("age", 20);

    auto d2 = Dict<IString, IBaseObject>();
    d2.set("name", "jon");
    d2.set("age", 20);

    auto d3 = Dict<IString, IBaseObject>();
    d3.set("name", "jessica");
    d3.set("age", 20);

    Bool eq{false};
    d1->equals(d2, &eq);
    ASSERT_TRUE(eq);

    d1->equals(d3, &eq);
    ASSERT_FALSE(eq);
}

TEST_F(DictObjectTest, EqualsDifferentOrder)
{
    auto d1 = Dict<IString, IBaseObject>();
    d1.set("name", "jon");
    d1.set("age", 20);

    auto d2 = Dict<IString, IBaseObject>();
    d2.set("age", 20);
    d2.set("name", "jon");

    Bool eq{false};
    d1->equals(d2, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(DictObjectTest, Inspectable)
{
    auto obj = Dict<IString, IBaseObject>();

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IDict::Id);
}

TEST_F(DictObjectTest, ImplementationName)
{
    auto obj = Dict<IString, IBaseObject>();

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::DictImpl");
}

struct DictTypes
{
    IntfID key;
    IntfID value;
};

static DictTypes getInterfaceIds(const ObjectPtr<IBaseObject>& obj)
{
    DictTypes pair;
    auto elementTypes = obj.asPtrOrNull<IDictElementType>(true);

    ErrCode errCode = elementTypes->getKeyInterfaceId(&pair.key);
    checkErrorInfo(errCode);

    errCode = elementTypes->getValueInterfaceId(&pair.value);
    checkErrorInfo(errCode);

    return pair;
}

TEST_F(DictObjectTest, ElementTypeNative)
{
    // Element type in template arguments can be proper types
    auto obj1 = Dict<std::string, int>();
    auto iid = getInterfaceIds(obj1);

    ASSERT_EQ(iid.key, IString::Id);
    ASSERT_EQ(iid.value, IInteger::Id);
}

TEST_F(DictObjectTest, ElementTypeInterface)
{
    // Element type in template arguments can be interfaces
    auto obj1 = Dict<IString, IInteger>();
    auto iid = getInterfaceIds(obj1);

    ASSERT_EQ(iid.key, IString::Id);
    ASSERT_EQ(iid.value, IInteger::Id);
}

TEST_F(DictObjectTest, ElementTypePtr)
{
    auto obj1 = Dict<IString, IInteger>();

    ASSERT_EQ(obj1.getKeyInterfaceId(), IString::Id);
    ASSERT_EQ(obj1.getValueInterfaceId(), IInteger::Id);
}

TEST_F(DictObjectTest, IteratorStartType)
{
    auto obj1 = Dict<IString, IInteger>({{"one", 1}, {"two", 2}, {"three", 3}});

    auto iterable = obj1.asPtrOrNull<IIterable>(true);
    ASSERT_TRUE(iterable.assigned());

    ObjectPtr<IIterator> iter;
    ErrCode errCode = iterable->createStartIterator(&iter);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    auto elType = iter.asPtrOrNull<IDictElementType>(true);
    ASSERT_TRUE(elType.assigned());

    IntfID keyId{};
    errCode = elType->getKeyInterfaceId(&keyId);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));
    ASSERT_EQ(keyId, IString::Id);

    IntfID valueId{};
    errCode = elType->getValueInterfaceId(&valueId);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));
    ASSERT_EQ(valueId, IInteger::Id);
}

TEST_F(DictObjectTest, IteratorEndType)
{
    auto obj1 = Dict<IString, IInteger>({{"one", 1}, {"two", 2}, {"three", 3}});

    auto iterable = obj1.asPtrOrNull<IIterable>(true);
    ASSERT_TRUE(iterable.assigned());

    ObjectPtr<IIterator> iter;
    ErrCode errCode = iterable->createStartIterator(&iter);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    auto elType = iter.asPtrOrNull<IDictElementType>(true);
    ASSERT_TRUE(elType.assigned());

    IntfID keyId{};
    errCode = elType->getKeyInterfaceId(&keyId);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));
    ASSERT_EQ(keyId, IString::Id);

    IntfID valueId{};
    errCode = elType->getValueInterfaceId(&valueId);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));
    ASSERT_EQ(valueId, IInteger::Id);
}

TEST_F(DictObjectTest, KeysStartType)
{
    auto obj1 = Dict<IString, IInteger>({{"one", 1}, {"two", 2}, {"three", 3}});

    auto keys = obj1.getKeys();

    ObjectPtr<IIterator> iter;
    ErrCode errCode = keys->createStartIterator(&iter);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    auto elType = iter.asPtrOrNull<IListElementType>(true);
    ASSERT_TRUE(elType.assigned());

    IntfID id{};
    errCode = elType->getElementInterfaceId(&id);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));
    ASSERT_EQ(id, IString::Id);
}

TEST_F(DictObjectTest, KeysEndType)
{
    auto obj1 = Dict<IString, IInteger>({{"one", 1}, {"two", 2}, {"three", 3}});

    auto keys = obj1.getKeys();

    ObjectPtr<IIterator> iter;
    ErrCode errCode = keys->createEndIterator(&iter);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    auto elType = iter.asPtrOrNull<IListElementType>(true);
    ASSERT_TRUE(elType.assigned());

    IntfID id{};
    errCode = elType->getElementInterfaceId(&id);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));
    ASSERT_EQ(id, IString::Id);
}

TEST_F(DictObjectTest, ValuesStartType)
{
    auto obj1 = Dict<IString, IInteger>({{"one", 1}, {"two", 2}, {"three", 3}});

    auto values = obj1.getValues();

    ObjectPtr<IIterator> iter;
    ErrCode errCode = values->createStartIterator(&iter);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    auto elType = iter.asPtrOrNull<IListElementType>(true);
    ASSERT_TRUE(elType.assigned());

    IntfID id{};
    errCode = elType->getElementInterfaceId(&id);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));
    ASSERT_EQ(id, IInteger::Id);
}

TEST_F(DictObjectTest, ValuesEndType)
{
    auto obj1 = Dict<IString, IInteger>({{"one", 1}, {"two", 2}, {"three", 3}});

    auto values = obj1.getValues();

    ObjectPtr<IIterator> iter;
    ErrCode errCode = values->createEndIterator(&iter);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    auto elType = iter.asPtrOrNull<IListElementType>(true);
    ASSERT_TRUE(elType.assigned());

    IntfID id{};
    errCode = elType->getElementInterfaceId(&id);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));
    ASSERT_EQ(id, IInteger::Id);
}
