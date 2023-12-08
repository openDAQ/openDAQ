#include <gtest/gtest.h>
#include <coretypes/boolean_factory.h>
#include <coretypes/binarydata_factory.h>
#include <coretypes/inspectable_ptr.h>

using namespace daq;

using BinaryDataTest = testing::Test;

TEST_F(BinaryDataTest, Create)
{
    ASSERT_NO_THROW(BinaryData(5));
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IBinaryData", "daq");

TEST_F(BinaryDataTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IBinaryData::Id);
}

TEST_F(BinaryDataTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IBinaryData>(), "{778DCE96-1ECD-59C0-B066-FD47BAF07789}");
}

TEST_F(BinaryDataTest, CreateInvalid)
{
    ASSERT_THROW(BinaryData(0), InvalidParameterException);
}

TEST_F(BinaryDataTest, NullParameter)
{
    auto binObj = BinaryData(5);

    ASSERT_EQ(binObj->getSize(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    ASSERT_EQ(binObj->getAddress(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(BinaryDataTest, WriteReadBufferSomeData)
{
    auto binaryDataObj = BinaryData(sizeof(int));

    void* data = binaryDataObj.getAddress();
    ASSERT_NE(data, nullptr);

    int* typedData = static_cast<int*>(data);
    *typedData = 123456789;

    ASSERT_EQ(typedData[0], 123456789);
}

TEST_F(BinaryDataTest, CoreType)
{
    auto binObj = BinaryData(5);

    ASSERT_EQ(binObj.getCoreType(), ctBinaryData);
}

TEST_F(BinaryDataTest, FromVector)
{
    std::vector<char> bytes;
    for (size_t i = 0; i < 100; i++)
        bytes.push_back((char) i);

    auto binObject = BinaryData(bytes.data(), bytes.size());
    ASSERT_EQ(binObject.getSize(), bytes.size());
    ASSERT_EQ(memcmp(binObject.getAddress(), bytes.data(), bytes.size()), 0);

    bytes[0] = 99;
    ASSERT_NE(memcmp(binObject.getAddress(), bytes.data(), bytes.size()), 0);
}

TEST_F(BinaryDataTest, ToString)
{
    auto obj = BinaryData(5);

    CharPtr str;
    ASSERT_EQ(obj->toString(&str), OPENDAQ_SUCCESS);

    ASSERT_STREQ(str, "daq::IBinaryData");
    daqFreeMemory(str);
}

TEST_F(BinaryDataTest, Inspectable)
{
    auto obj = BinaryData(5);

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IBinaryData::Id);
}

TEST_F(BinaryDataTest, ImplementationName)
{
    auto obj = BinaryData(5);

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::BinaryDataImpl");
}
