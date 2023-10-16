#include <gtest/gtest.h>
#include <opcuashared/opcuavector.h>
#include <opcuashared/opcuacommon.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaVectorTest = testing::Test;

TEST_F(OpcUaVectorTest, ScopeTest)
{
    OpcUaObject<UA_String> a = OpcUaObject<UA_String>(UA_STRING_ALLOC("Hello One"));
    OpcUaObject<UA_String> b = OpcUaObject<UA_String>(UA_STRING_ALLOC("Hello Two"));

    {
        OpcUaVector<UA_String> vect;
        vect.push_back(a.getValue());
        vect.push_back(a.getValue());
        ASSERT_EQ(utils::ToStdString(vect[1]), utils::ToStdString(a.getValue()));
        vect[1] = b.getValue();
        ASSERT_EQ(utils::ToStdString(vect[1]), utils::ToStdString(b.getValue()));
    }
}

TEST_F(OpcUaVectorTest, CopyTest)
{
    OpcUaVector<UA_Int32> a = {1, 2, 3, 4, 5};
    OpcUaVector<UA_Int32> b = {10, 20, 30};

    a = b;
    ASSERT_EQ(a.size(), b.size());
    ASSERT_EQ(a[0], b[0]);
    ASSERT_EQ(a[1], b[1]);
    ASSERT_EQ(a[2], b[2]);

    UA_Int32 b0 = 10;
    UA_Int32 a0 = 99;
    a[0] = a0;
    ASSERT_EQ(a[0], a0);
    ASSERT_EQ(b[0], b0);
}

TEST_F(OpcUaVectorTest, ResizeTest)
{
    const size_t sizeA = 5;
    OpcUaVector<UA_Int32> a = {1, 2, 3, 4, 5};
    const size_t sizeB = 3;
    OpcUaVector<UA_Int32> b = {10, 20, 30};

    size_t newSizeA = 3;
    ASSERT_EQ(a.size(), sizeA);
    a.resize(newSizeA);
    ASSERT_EQ(a.size(), newSizeA);

    size_t newSizeB = 10;
    ASSERT_EQ(b.size(), sizeB);
    b.resize(newSizeB);
    ASSERT_EQ(b.size(), newSizeB);

    OpcUaVector<UA_Int32> c;
    c.resize(10);
}

TEST_F(OpcUaVectorTest, AppendTest)
{
    std::vector<UA_Int32> a = {1, 2, 3, 4, 5};
    OpcUaVector<UA_Int32> b;
    for (size_t i = 0; i < a.size(); i++)
        b.push_back(a[i]);

    ASSERT_EQ(a.size(), b.size());
    bool areEqual = memcmp(a.data(), b.data(), sizeof(UA_Int32) * a.size()) == 0;
    ASSERT_TRUE(areEqual);

    UA_Int32 last = 99;
    b[b.size() - 1] = last;
    areEqual = memcmp(a.data(), b.data(), sizeof(UA_Int32) * a.size()) == 0;
    ASSERT_FALSE(areEqual);
}

TEST_F(OpcUaVectorTest, SetGetTest)
{
    UA_Int32 a = 1;
    UA_Int32 b = 2;
    UA_Int32 c = 3;

    OpcUaVector<UA_Int32> vectA;
    vectA.resize(10);

    vectA[0] = a;
    ASSERT_EQ(vectA[0], a);
    vectA[1] = b;
    ASSERT_EQ(vectA[1], b);
    vectA[1] = c;
    ASSERT_EQ(vectA[1], c);

    OpcUaVector<UA_Int32> vectB;
    vectB.resize(10);

    vectB.set(0, a);
    ASSERT_EQ(vectB.get(0), a);
    vectB.set(1, b);
    ASSERT_EQ(vectB.get(1), b);
    vectB.set(1, c);
    ASSERT_EQ(vectB.get(1), c);
}

END_NAMESPACE_OPENDAQ_OPCUA
