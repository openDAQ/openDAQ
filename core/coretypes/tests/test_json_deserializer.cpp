#include <gtest/gtest.h>
#include <testutils/testutils.h>
#include <limits>
#include <cmath>
#include <coretypes/coretypes.h>

using namespace daq;

static ErrCode serializedObjectFactory(ISerializedObject*, IBaseObject*, IFunction*, IBaseObject**)
{
    return OPENDAQ_SUCCESS;
}

static ErrCode errorFactory(ISerializedObject*, IBaseObject*, IFunction*, IBaseObject**)
{
    return OPENDAQ_ERR_GENERALERROR;
}

class JsonDeserializerTest : public testing::Test
{
protected:
    void SetUp() override
    {
        deserializer = JsonDeserializer();
    }

    void TearDown() override
    {
        daqUnregisterSerializerFactory(factoryId);
        deserializer.release();
    }

    void registerFactory(daqDeserializerFactory factory)
    {
        daqRegisterSerializerFactory(factoryId, factory);
    }

    DeserializerPtr deserializer;
    static ConstCharPtr factoryId;

    Float floatMin = std::numeric_limits<Float>::min();
    Float floatMax = std::numeric_limits<Float>::max();
    Int intMin = std::numeric_limits<Int>::min();
    Int intMax = std::numeric_limits<Int>::max();
};

ConstCharPtr JsonDeserializerTest::factoryId = "test";

TEST_F(JsonDeserializerTest, DeserializeInvalidJson)
{
    ASSERT_THROW(deserializer.deserialize("..."), DeserializeException);
}

TEST_F(JsonDeserializerTest, boolTrue)
{
    BooleanPtr boolean = deserializer.deserialize("true");

    ASSERT_TRUE(boolean.getValue<bool>(false));
}

TEST_F(JsonDeserializerTest, boolFalse)
{
    BooleanPtr boolean = deserializer.deserialize("false");

    ASSERT_FALSE(boolean.getValue<bool>(true));
}

TEST_F(JsonDeserializerTest, floatZero)
{
    FloatPtr floating = deserializer.deserialize("0.0");

    ASSERT_EQ(floating.getValue<Float>(-1), 0.0);
}

TEST_F(JsonDeserializerTest, floatMax)
{
    FloatPtr deserialized = deserializer.deserialize("1.7976931348623157e308");
    ASSERT_EQ(deserialized.getValue<Float>(-1), floatMax);
}

TEST_F(JsonDeserializerTest, floatMin)
{
    FloatPtr deserialized = deserializer.deserialize("2.2250738585072014e-308");
    ASSERT_EQ(deserialized.getValue<Float>(-1), floatMin);
}

TEST_F(JsonDeserializerTest, floatNaN)
{
    FloatPtr deserialized = deserializer.deserialize("NaN");
    ASSERT_TRUE(std::isnan(deserialized.getValue<Float>(-1)));
}

TEST_F(JsonDeserializerTest, floatInf)
{
    FloatPtr deserialized = deserializer.deserialize("Infinity");
    ASSERT_TRUE(std::isinf(deserialized.getValue<Float>(-1)));
}

TEST_F(JsonDeserializerTest, intZero)
{
    IntPtr deserialized = deserializer.deserialize("0");
    ASSERT_EQ(deserialized.getValue<Int>(-1), 0);
}

TEST_F(JsonDeserializerTest, intMax)
{
    IntPtr deserialized = deserializer.deserialize("9223372036854775807");
    ASSERT_EQ(deserialized.getValue<Int>(-1), intMax);
}

TEST_F(JsonDeserializerTest, intMin)
{
    IntPtr deserialized = deserializer.deserialize("-9223372036854775808");
    ASSERT_EQ(deserialized.getValue<Int>(-1), intMin);
}

TEST_F(JsonDeserializerTest, asciiStr)
{
    const auto expected = R"( !"#$%&'()*+'-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)";

    StringPtr deserialized =
        deserializer.deserialize(R"(" !\"#$%&'()*+'-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~")");
    ASSERT_EQ(deserialized.toStdString(), expected);
}

TEST_F(JsonDeserializerTest, null)
{
    BaseObjectPtr deserialized = deserializer.deserialize("null");
    ASSERT_FALSE(deserialized.assigned());
}

TEST_F(JsonDeserializerTest, emptyList)
{
    ListPtr<IBaseObject> deserialized = deserializer.deserialize("[]");

    ASSERT_EQ(deserialized.getCount(), 0u);
}

TEST_F(JsonDeserializerTest, stringListOne)
{
    ListPtr<IString> deserialized = deserializer.deserialize(R"(["Item1"])");
    ASSERT_EQ(deserialized.getCount(), 1u);

    StringPtr item1 = deserialized.getItemAt(0);

    ConstCharPtr item1Str;
    item1->getCharPtr(&item1Str);

    ASSERT_STREQ(item1Str, "Item1");
}

TEST_F(JsonDeserializerTest, stringListMultiple)
{
    ListPtr<IString> deserialized = deserializer.deserialize(R"(["Item1","Item2"])");
    ASSERT_EQ(deserialized.getCount(), 2u);

    StringPtr item1 = deserialized.getItemAt(0);
    StringPtr item2 = deserialized.getItemAt(1);

    ConstCharPtr itemStr;
    item1->getCharPtr(&itemStr);
    ASSERT_STREQ(itemStr, "Item1");

    item2->getCharPtr(&itemStr);
    ASSERT_STREQ(itemStr, "Item2");
}

TEST_F(JsonDeserializerTest, boolListTrue)
{
    ListPtr<IBoolean> deserialized = deserializer.deserialize("[true]");
    ASSERT_EQ(deserialized.getCount(), 1u);

    BooleanPtr truePtr = deserialized.getItemAt(0);
    ASSERT_TRUE(IsTrue(truePtr));
}

TEST_F(JsonDeserializerTest, boolListFalse)
{
    ListPtr<IBoolean> deserialized = deserializer.deserialize("[false]");
    ASSERT_EQ(deserialized.getCount(), 1u);

    BooleanPtr falsePtr = deserialized.getItemAt(0);
    ASSERT_TRUE(IsFalse(falsePtr));
}

TEST_F(JsonDeserializerTest, boolList)
{
    ListPtr<IBoolean> deserialized = deserializer.deserialize("[false,true]");
    ASSERT_EQ(deserialized.getCount(), 2u);

    BooleanPtr falsePtr = deserialized.getItemAt(0);
    ASSERT_TRUE(IsFalse(falsePtr));

    BooleanPtr truePtr = deserialized.getItemAt(1);
    ASSERT_TRUE(IsTrue(truePtr));
}

TEST_F(JsonDeserializerTest, floatListOne)
{
    ListPtr<IFloat> deserialized = deserializer.deserialize("[0.0]");
    ASSERT_EQ(deserialized.getCount(), 1u);

    auto item1 = deserialized.getItemAt(0);

    Float zero;
    item1->getValue(&zero);

    ASSERT_EQ(zero, 0.0);
}

TEST_F(JsonDeserializerTest, floatListMultiple)
{
    ListPtr<IFloat> deserialized = deserializer.deserialize("[0.0,2.2250738585072014e-308,1.7976931348623157e308]");
    ASSERT_EQ(deserialized.getCount(), 3u);

    auto zeroPtr = deserialized.getItemAt(0);
    auto minPtr = deserialized.getItemAt(1);
    auto maxPtr = deserialized.getItemAt(2);

    Float zero;
    ErrCode errCode = zeroPtr->getValue(&zero);
    ASSERT_SUCCEEDED(errCode);
    ASSERT_EQ(zero, 0.0);

    Float min;
    errCode = minPtr->getValue(&min);
    ASSERT_EQ(min, floatMin);

    Float max;
    errCode = maxPtr->getValue(&max);
    ASSERT_EQ(max, floatMax);
}

TEST_F(JsonDeserializerTest, intListOne)
{
    ListPtr<IInteger> deserialized = deserializer.deserialize("[0]");
    ASSERT_EQ(deserialized.getCount(), 1u);

    auto zeroPtr = deserialized.getItemAt(0);

    Int zero;
    ErrCode errCode = zeroPtr->getValue(&zero);
    ASSERT_SUCCEEDED(errCode);
    ASSERT_EQ(zero, 0);
}

TEST_F(JsonDeserializerTest, intListMultiple)
{
    ListPtr<IInteger> deserialized = deserializer.deserialize("[0,-9223372036854775808,9223372036854775807]");
    ASSERT_EQ(deserialized.getCount(), 3u);

    auto zeroPtr = deserialized.getItemAt(0);
    auto minPtr = deserialized.getItemAt(1);
    auto maxPtr = deserialized.getItemAt(2);

    Int zero;
    ErrCode errCode = zeroPtr->getValue(&zero);
    ASSERT_SUCCEEDED(errCode);
    ASSERT_EQ(zero, 0);

    Int min;
    errCode = minPtr->getValue(&min);
    ASSERT_SUCCEEDED(errCode);
    ASSERT_EQ(min, intMin);

    Int max;
    errCode = maxPtr->getValue(&max);
    ASSERT_SUCCEEDED(errCode);
    ASSERT_EQ(max, intMax);
}

TEST_F(JsonDeserializerTest, mixedList)
{
    ListPtr<IBaseObject> deserialized = deserializer.deserialize(
        R"([0.0,0,-2.5,1.5,1,-2,2.2250738585072014e-308,1.7976931348623157e308,-9223372036854775808,9223372036854775807,"Test1"])");
    ASSERT_EQ(deserialized.getCount(), 11u);

    auto zeroFloatPtr = deserialized.getItemAt(0);
    Float zeroFloat = zeroFloatPtr;
    ASSERT_EQ(zeroFloat, 0.0);

    auto zeroIntPtr = deserialized.getItemAt(1);
    Int zeroInt = zeroIntPtr;
    ASSERT_EQ(zeroInt, 0);

    auto negativeFloatPtr = deserialized.getItemAt(2);
    Float negativeFloat = negativeFloatPtr;
    ASSERT_EQ(negativeFloat, -2.5);

    auto pozitiveFloatPtr = deserialized.getItemAt(3);
    Float pozitiveFloat = pozitiveFloatPtr;
    ASSERT_EQ(pozitiveFloat, 1.5);

    auto pozitiveIntPtr = deserialized.getItemAt(4);
    Int pozitiveInt = pozitiveIntPtr;
    ASSERT_EQ(pozitiveInt, 1);

    auto negativeIntPtr = deserialized.getItemAt(5);
    Int negativeInt = negativeIntPtr;
    ASSERT_EQ(negativeInt, -2);

    auto floatMinPtr = deserialized.getItemAt(6);
    Float minFloat = floatMinPtr;
    ASSERT_EQ(minFloat, floatMin);

    auto floatMaxPtr = deserialized.getItemAt(7);
    Float maxFloat = floatMaxPtr;
    ASSERT_EQ(maxFloat, floatMax);

    auto intMinPtr = deserialized.getItemAt(8);
    Int minInt = intMinPtr;
    ASSERT_EQ(minInt, intMin);

    auto intMaxPtr = deserialized.getItemAt(9);
    Int maxInt = intMaxPtr;
    ASSERT_EQ(maxInt, intMax);

    auto stringPtr = deserialized.getItemAt(10);
    std::string string = stringPtr;
    ASSERT_EQ(string, "Test1");
}

TEST_F(JsonDeserializerTest, deserializerIsNull)
{
    DeserializerPtr ptr;
    ASSERT_THROW(ptr.deserialize(nullptr), InvalidParameterException);
}

TEST_F(JsonDeserializerTest, unknownObjectType)
{
    std::string json = R"({"__type":")" + std::string("unknown") + "\"}";

    IBaseObject* obj;
    ErrCode errCode = deserializer->deserialize(String(json.data()), nullptr, nullptr, &obj);

    ASSERT_EQ(errCode, OPENDAQ_ERR_FACTORY_NOT_REGISTERED);
}

TEST_F(JsonDeserializerTest, objectTypeTagNotInt)
{
    IBaseObject* obj;
    ErrCode errCode = deserializer->deserialize(String(R"({"__type":0.0})"), nullptr, nullptr, &obj);

    ASSERT_EQ(errCode, OPENDAQ_ERR_DESERIALIZE_UNKNOWN_TYPE);
}

TEST_F(JsonDeserializerTest, noObjectType)
{
    IBaseObject* obj;
    ErrCode errCode = deserializer->deserialize(String(R"({"test":0})"), nullptr, nullptr, &obj);

    ASSERT_EQ(errCode, OPENDAQ_ERR_DESERIALIZE_NO_TYPE);
}

TEST_F(JsonDeserializerTest, deserializeNullString)
{
    ASSERT_THROW(deserializer.deserialize(nullptr), ArgumentNullException);
}

TEST_F(JsonDeserializerTest, registerFactory)
{
    ErrCode errCode = daqRegisterSerializerFactory(factoryId, serializedObjectFactory);

    ASSERT_EQ(errCode, OPENDAQ_SUCCESS);
}

TEST_F(JsonDeserializerTest, registerExistingFactory)
{
    registerFactory(serializedObjectFactory);

    ErrCode errCode = daqRegisterSerializerFactory(factoryId, serializedObjectFactory);

    ASSERT_EQ(errCode, OPENDAQ_SUCCESS);
}

TEST_F(JsonDeserializerTest, getFactory)
{
    registerFactory(serializedObjectFactory);

    daqDeserializerFactory factory;
    ErrCode errCode = daqGetSerializerFactory(factoryId, &factory);

    ASSERT_EQ(errCode, OPENDAQ_SUCCESS);
}

TEST_F(JsonDeserializerTest, unregisterFactory)
{
    registerFactory(serializedObjectFactory);
    ErrCode errCode = daqUnregisterSerializerFactory(factoryId);

    ASSERT_EQ(errCode, OPENDAQ_SUCCESS);
}

TEST_F(JsonDeserializerTest, unregisterNonExistingFactory)
{
    ErrCode errCode = daqUnregisterSerializerFactory(factoryId);

    ASSERT_EQ(errCode, OPENDAQ_ERR_FACTORY_NOT_REGISTERED);
}

TEST_F(JsonDeserializerTest, getNonExistingFactory)
{
    daqDeserializerFactory factory;
    ErrCode errCode = daqGetSerializerFactory(factoryId, &factory);

    ASSERT_EQ(errCode, OPENDAQ_ERR_FACTORY_NOT_REGISTERED);
}

TEST_F(JsonDeserializerTest, factoryReturnsError)
{
    registerFactory(errorFactory);

    std::string json = R"({"__type":")" + std::string(factoryId) + "\"}";

    ASSERT_THROW(deserializer.deserialize(json.data()), GeneralErrorException);
}

TEST_F(JsonDeserializerTest, createToNull)
{
    ErrCode errCode = createJsonDeserializer(nullptr);

    ASSERT_EQ(errCode, OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(JsonDeserializerTest, deserializerGuid)
{
    auto expected = OPENDAQ_INTFID("IDeserializer");
    ASSERT_EQ(IDeserializer::Id, expected);
}

TEST_F(JsonDeserializerTest, serializedListGuid)
{
    auto expected = OPENDAQ_INTFID("ISerializedList");
    ASSERT_EQ(ISerializedList::Id, expected);
}

TEST_F(JsonDeserializerTest, serializedObjectGuid)
{
    auto expected = OPENDAQ_INTFID("ISerializedObject");
    ASSERT_EQ(ISerializedObject::Id, expected);
}

TEST_F(JsonDeserializerTest, Inspectable)
{
    auto ids = deserializer.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IDeserializer::Id);
}

TEST_F(JsonDeserializerTest, ImplementationName)
{
    StringPtr className = deserializer.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::JsonDeserializerImpl");
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IDeserializer", "daq");

TEST_F(JsonDeserializerTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IDeserializer::Id);
}

TEST_F(JsonDeserializerTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IDeserializer>(), "{66DEEEF9-2B0D-5A49-A050-2820C4738AE7}");
}
