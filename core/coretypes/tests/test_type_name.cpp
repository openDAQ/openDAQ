#include <gtest/gtest.h>
#include <coretypes/common.h>

using TypeNameTest = testing::Test;

using namespace daq;

namespace daq
{
    template <std::size_t N>
    std::ostream& operator<<(std::ostream& os, const ConstexprString<N>& bar)
    {
        return os << std::string(bar.data(), N);
    }
}

TEST_F(TypeNameTest, Char)
{
    constexpr auto intType = typeName<char>();
    ASSERT_EQ(intType, "char");
}

TEST_F(TypeNameTest, SignedChar)
{
    constexpr auto intType = typeName<signed char>();
    ASSERT_EQ(intType, "int8");
}

TEST_F(TypeNameTest, UnsignedChar)
{
    constexpr auto intType = typeName<unsigned char>();
    ASSERT_EQ(intType, "uint8");
}

TEST_F(TypeNameTest, Float)
{
    constexpr auto floatType = typeName<float>();
    ASSERT_EQ(floatType, "float");
}

TEST_F(TypeNameTest, Double)
{
    constexpr auto doubleType = typeName<double>();
    ASSERT_EQ(doubleType, "double");
}

TEST_F(TypeNameTest, Bool)
{
    constexpr auto boolType = typeName<bool>();
    ASSERT_EQ(boolType, "bool");
}

////////////////////////////
//// Fundamental integers
////////////////////////////

TEST_F(TypeNameTest, ShortInt)
{
    constexpr auto intType = typeName<short int>();
    ASSERT_EQ(intType, "int16");
}

TEST_F(TypeNameTest, UnsignedShortInt)
{
    constexpr auto uintType = typeName<unsigned short int>();
    ASSERT_EQ(uintType, "uint16");
}

TEST_F(TypeNameTest, Int)
{
    constexpr auto intType = typeName<int>();
    ASSERT_EQ(intType, "int32");
}

TEST_F(TypeNameTest, UnsignedInt)
{
    constexpr auto uintType = typeName<unsigned int>();
    ASSERT_EQ(uintType, "uint32");
}

TEST_F(TypeNameTest, LongInt)
{
    constexpr auto intType = typeName<long int>();

#if defined(__LP64__)
    ASSERT_EQ(intType, "int64");
#else
    ASSERT_EQ(intType, "int32");
#endif
}

TEST_F(TypeNameTest, UnsignedLongInt)
{
    constexpr auto uintType = typeName<unsigned long int>();

#if defined(__LP64__)
    ASSERT_EQ(uintType, "uint64");
#else
    ASSERT_EQ(uintType, "uint32");
#endif
}

TEST_F(TypeNameTest, LongLongInt)
{
    constexpr auto intType = typeName<long long int>();
    ASSERT_EQ(intType, "int64");
}

TEST_F(TypeNameTest, UnsignedLongLongInt)
{
    constexpr auto uintType = typeName<unsigned long long int>();
    ASSERT_EQ(uintType, "uint64");
}

///////////////////
//// Fixed types
///////////////////

TEST_F(TypeNameTest, Int32)
{
    constexpr auto int32Type = typeName<int32_t>();
    ASSERT_EQ(int32Type, "int32");
}


TEST_F(TypeNameTest, UInt32)
{
    constexpr auto uint32Type = typeName<uint32_t>();
    ASSERT_EQ(uint32Type, "uint32");
}

TEST_F(TypeNameTest, Int64)
{
    constexpr auto int64Type = typeName<int64_t>();
    ASSERT_EQ(int64Type, "int64");
}

TEST_F(TypeNameTest, UInt64)
{
    constexpr auto uint64Type = typeName<uint64_t>();
    ASSERT_EQ(uint64Type, "uint64");
}

TEST_F(TypeNameTest, Int8)
{
    constexpr auto int8Type = typeName<int8_t>();
    ASSERT_EQ(int8Type, "int8");
}

TEST_F(TypeNameTest, UInt8)
{
    constexpr auto uint8Type = typeName<uint8_t>();
    ASSERT_EQ(uint8Type, "uint8");
}

TEST_F(TypeNameTest, Int16)
{
    constexpr auto int16Type = typeName<int16_t>();
    ASSERT_EQ(int16Type, "int16");
}

TEST_F(TypeNameTest, UInt16)
{
    constexpr auto uint16Type = typeName<uint16_t>();
    ASSERT_EQ(uint16Type, "uint16");
}
