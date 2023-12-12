#include <gtest/gtest.h>
#include <math.h>
#include <coretypes/coretypes.h>

using namespace daq;

class JsonSerializerTest : public testing::Test
{
public:
    explicit JsonSerializerTest() = default;

protected:
    void SetUp() override
    {
        serializer = JsonSerializer();
    }

    void TearDown() override
    {
        serializer.release();
    }

    SerializerPtr serializer;
    Float floatMin = std::numeric_limits<Float>::min();
    Float floatMax = std::numeric_limits<Float>::max();
    Int intMin = std::numeric_limits<Int>::min();
    Int intMax = std::numeric_limits<Int>::max();
};

TEST_F(JsonSerializerTest, boolTrue)
{
    auto boolObj = Boolean(true);

    boolObj.serialize(serializer);
    StringPtr serializedBool = serializer.getOutput();

    ASSERT_EQ(serializedBool.toStdString(), "true");
}

TEST_F(JsonSerializerTest, boolFalse)
{
    auto boolObj = Boolean(false);

    boolObj.serialize(serializer);
    StringPtr serializedBool = serializer.getOutput();

    ASSERT_EQ(serializedBool.toStdString(), "false");
}

TEST_F(JsonSerializerTest, floatZero)
{
    auto float0 = Floating(0);
    float0.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "0.0");
}

TEST_F(JsonSerializerTest, floatMax)
{
    auto max = Floating(floatMax);
    max.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "1.7976931348623157e308");
}

TEST_F(JsonSerializerTest, floatMin)
{
    auto min = Floating(floatMin);
    min.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "2.2250738585072014e-308");
}

TEST_F(JsonSerializerTest, floatNaN)
{
    auto nan = Floating(NAN);
    nan.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "NaN");
}

TEST_F(JsonSerializerTest, floatInf)
{
    auto nan = Floating(INFINITY);
    nan.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "Infinity");
}

TEST_F(JsonSerializerTest, intZero)
{
    auto int0 = Integer(0);
    int0.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "0");
}

TEST_F(JsonSerializerTest, intMax)
{
    auto max = Integer(intMax);
    max.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "9223372036854775807");
}

TEST_F(JsonSerializerTest, intMin)
{
    auto min = Integer(intMin);
    min.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "-9223372036854775808");
}

TEST_F(JsonSerializerTest, asciiStr)
{
    const auto ascii = R"( !"#$%&'()*+'-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)";
    const auto quotedAndEscaped = R"(" !\"#$%&'()*+'-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~")";

    auto plainStr = String(ascii);
    plainStr.serialize(serializer);

    StringPtr ptr = serializer.getOutput();
    std::string str = ptr.toStdString();

    ASSERT_EQ(str, quotedAndEscaped);
}

TEST_F(JsonSerializerTest, nullString)
{
    auto nullStr = String(nullptr);
    nullStr.serialize(serializer);

    std::string str = serializer.getOutput();

    ASSERT_EQ(str, R"("")");
}

TEST_F(JsonSerializerTest, emptyList)
{
    auto emptyList = List<IBaseObject>();
    emptyList.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "[]");
}

TEST_F(JsonSerializerTest, stringListOne)
{
    auto arrayOneItem = List<IString>("Item1");
    arrayOneItem.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, R"(["Item1"])");
}

TEST_F(JsonSerializerTest, stringListMultiple)
{
    auto arrayMultipleItems = List<IString>("Item1", "Item2");
    arrayMultipleItems.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, R"(["Item1","Item2"])");
}

TEST_F(JsonSerializerTest, floatListOne)
{
    auto arrayOneItem = List<IFloat>(0.0);
    arrayOneItem.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "[0.0]");
}

TEST_F(JsonSerializerTest, floatListMultiple)
{
    auto arrayMultipleItems = List<IFloat>(0.0, std::numeric_limits<Float>::min(), std::numeric_limits<Float>::max());
    arrayMultipleItems.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "[0.0,2.2250738585072014e-308,1.7976931348623157e308]");
}

TEST_F(JsonSerializerTest, intListOne)
{
    auto arrayOnItem = List<IInteger>(0);
    arrayOnItem.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "[0]");
}

TEST_F(JsonSerializerTest, intListMultiple)
{
    auto arrayMultipleItems = List<IInteger>(0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    arrayMultipleItems.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "[0,-9223372036854775808,9223372036854775807]");
}

TEST_F(JsonSerializerTest, intFloatList)
{
    ListPtr<IBaseObject> mixedArray = List<IBaseObject>(0.0, 0, -2.5, 1.5, 1, -2, floatMin, floatMax, intMin, intMax);
    mixedArray.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized, "[0.0,0,-2.5,1.5,1,-2,2.2250738585072014e-308,1.7976931348623157e308,-9223372036854775808,9223372036854775807]");
}

TEST_F(JsonSerializerTest, mixedList)
{
    ListPtr<IBaseObject> mixedArray = List<IBaseObject>(0.0, 0, -2.5, 1.5, 1, -2, floatMin, floatMax, intMin, intMax, "Test1");
    mixedArray.serialize(serializer);

    std::string serialized = serializer.getOutput().toStdString();

    ASSERT_EQ(serialized,
              R"([0.0,0,-2.5,1.5,1,-2,2.2250738585072014e-308,1.7976931348623157e308,-9223372036854775808,9223372036854775807,"Test1"])");
}

TEST_F(JsonSerializerTest, startTaggedNull)
{
    ASSERT_THROW(serializer.startTaggedObject(nullptr), ArgumentNullException);
}

TEST_F(JsonSerializerTest, isCompleteFalse)
{
    serializer.startObject();

    ASSERT_FALSE(serializer.isComplete());
}

TEST_F(JsonSerializerTest, isCompleteEmpty)
{
    ASSERT_FALSE(serializer.isComplete());
}

TEST_F(JsonSerializerTest, isCompleteEmptyObject)
{
    serializer.startObject();
    serializer.endObject();
    ASSERT_TRUE(serializer.isComplete());
}

TEST_F(JsonSerializerTest, reset)
{
    serializer.startObject();
    serializer.endObject();

    ASSERT_TRUE(serializer.isComplete());

    serializer.reset();
    ASSERT_FALSE(serializer.isComplete());
}

TEST_F(JsonSerializerTest, simpleObject)
{
    serializer.startObject();
    serializer.key("test");
    serializer.writeString("success");
    serializer.endObject();

    StringPtr json = serializer.getOutput();

    ASSERT_EQ(json.toStdString(), R"({"test":"success"})");
}

TEST_F(JsonSerializerTest, simpleObjectKeyLenght)
{
    serializer.startObject();
    serializer->keyRaw("test", 4);
    serializer.writeString("success");
    serializer.endObject();

    StringPtr json = serializer.getOutput();

    ASSERT_EQ(json.toStdString(), R"({"test":"success"})");
}

TEST_F(JsonSerializerTest, objectZeroLengthKey)
{
    serializer.startObject();
    ASSERT_THROW(serializer.key(""), InvalidParameterException);
}

TEST_F(JsonSerializerTest, objectNullKey)
{
    serializer.startObject();
    ASSERT_THROW(serializer.key(String(nullptr)), ArgumentNullException);
}

TEST_F(JsonSerializerTest, objectNullKeyPtr)
{
    serializer.startObject();
    ASSERT_THROW(serializer.key(nullptr), ArgumentNullException);
}

TEST_F(JsonSerializerTest, objectNullKeyInterface)
{
    serializer.startObject();

    ErrCode errCode = serializer->keyStr(static_cast<IString*>(nullptr));

    ASSERT_EQ(errCode, OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(JsonSerializerTest, objectNullKeyPtrInterface)
{
    serializer.startObject();
    ErrCode errCode = serializer->key(static_cast<ConstCharPtr>(nullptr));

    ASSERT_EQ(errCode, OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(JsonSerializerTest, objectZeroLengthKeyInterface)
{
    serializer.startObject();
    ErrCode errCode = serializer->keyRaw("", 0);

    ASSERT_EQ(errCode, OPENDAQ_ERR_INVALIDPARAMETER);
}

TEST_F(JsonSerializerTest, objectKeyNullSizeInterface)
{
    serializer.startObject();
    ErrCode errCode = serializer->keyRaw(nullptr, 0);

    ASSERT_EQ(errCode, OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(JsonSerializerTest, createToNull)
{
    ErrCode errCode = createJsonSerializer(nullptr);

    ASSERT_EQ(errCode, OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(JsonSerializerTest, Inspectable)
{
    auto ids = serializer.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], ISerializer::Id);
}

TEST_F(JsonSerializerTest, ImplementationName)
{
    std::string className = serializer.asPtr<IInspectable>(true).getRuntimeClassName();

    auto prefix = className.find("daq::JsonSerializerImpl<");
    ASSERT_EQ(prefix, 0u);
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("ISerializer", "daq");

TEST_F(JsonSerializerTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, ISerializer::Id);
}

TEST_F(JsonSerializerTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<ISerializer>(), "{4230318E-85D5-5BB6-80C8-10CC47B7A3D2}");
}

static constexpr auto SERIALIZABLE_ID = FromTemplatedTypeName("ISerializable", "daq");

TEST_F(JsonSerializerTest, SerializableId)
{
    ASSERT_EQ(SERIALIZABLE_ID, ISerializable::Id);
}

TEST_F(JsonSerializerTest, SerializableString)
{
    ASSERT_EQ(daqInterfaceIdString<ISerializable>(), "{831915F2-C42F-5520-A420-56524D2AC552}");
}
