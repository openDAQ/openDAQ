#include <testutils/testutils.h>
#include <future>
#include "opcuaclient/opcuaclient.h"
#include "opcuaservertesthelper.h"
#include "opcuashared/opcuacommon.h"
#include <opcuaclient/attribute_reader.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using AttributeReaderTest = BaseClientTest;

TEST_F(AttributeReaderTest, TwoAttributes)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    const auto attr1 = OpcUaAttribute(OpcUaNodeId(1, ".i64"), UA_ATTRIBUTEID_VALUE);
    const auto attr2 = OpcUaAttribute(OpcUaNodeId(1, ".i32"), UA_ATTRIBUTEID_VALUE);

    auto reader = AttributeReader(client);
    reader.addAttribute(attr1);
    reader.addAttribute(attr2);
    reader.read();

    auto i64 = reader.getValue(attr1).toInteger();
    ASSERT_EQ(i64, 64);

    auto i32 = reader.getValue(attr2).toInteger();
    ASSERT_EQ(i32, 41);
}

TEST_F(AttributeReaderTest, NotRead)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    const auto attr = OpcUaAttribute(OpcUaNodeId(1, ".i64"), UA_ATTRIBUTEID_VALUE);

    auto reader = AttributeReader(client);
    reader.addAttribute(attr);

    ASSERT_THROW(reader.getValue(attr), OpcUaException);

    reader.read();
    ASSERT_NO_THROW(reader.getValue(attr));
}

TEST_F(AttributeReaderTest, Missing)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    const auto idI64 = OpcUaNodeId(1, ".i64");
    const auto idI32 = OpcUaNodeId(1, ".i32");

    auto reader = AttributeReader(client);
    reader.addAttribute({idI64, UA_ATTRIBUTEID_VALUE});
    reader.read();

    ASSERT_NO_THROW(reader.getValue(idI64, UA_ATTRIBUTEID_VALUE));
    ASSERT_THROW(reader.getValue(idI32, UA_ATTRIBUTEID_VALUE), OpcUaException);
    ASSERT_THROW(reader.getValue(idI64, UA_ATTRIBUTEID_DISPLAYNAME), OpcUaException);
}

TEST_F(AttributeReaderTest, NoAttributes)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());

    auto reader = AttributeReader(client);
    ASSERT_NO_THROW(reader.read());
}

TEST_F(AttributeReaderTest, Clear)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    const auto idI64 = OpcUaNodeId(1, ".i64");

    auto reader = AttributeReader(client);
    reader.addAttribute({idI64, UA_ATTRIBUTEID_VALUE});
    reader.addAttribute({idI64, UA_ATTRIBUTEID_VALUE});
    reader.read();

    auto i64 = reader.getValue(idI64, UA_ATTRIBUTEID_VALUE).toInteger();
    ASSERT_EQ(i64, 64);
}

TEST_F(AttributeReaderTest, FailedRequest)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    ASSERT_FALSE(client->isConnected());
    
    const auto idI64 = OpcUaNodeId(1, ".i64");

    auto reader = AttributeReader(client);
    reader.addAttribute({idI64, UA_ATTRIBUTEID_VALUE});
    ASSERT_THROW(reader.read(), OpcUaException);
}

TEST_F(AttributeReaderTest, SameAttribute)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    const auto attr = OpcUaAttribute(OpcUaNodeId(1, ".i64"), UA_ATTRIBUTEID_VALUE);

    auto reader = AttributeReader(client);
    reader.addAttribute(attr);
    reader.addAttribute(attr);
    reader.read();

    auto i64 = reader.getValue(attr).toInteger();
    ASSERT_EQ(i64, 64);
}

TEST_F(AttributeReaderTest, MaxNodesPerRead)
{
    const size_t maxNodesPerRead = 3;

    testHelper.stop();
    testHelper.onConfigure([&](UA_ServerConfig* config) { config->maxNodesPerRead = maxNodesPerRead; });
    testHelper.startServer();

    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    const size_t maxBatchSize = client->readValue(OpcUaNodeId(UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERREAD)).toInteger();
    ASSERT_EQ(maxBatchSize, maxNodesPerRead);

    const auto idI64 = OpcUaNodeId(1, ".i64");
    const auto idI32 = OpcUaNodeId(1, ".i32");
    const auto idI16 = OpcUaNodeId(1, ".i16");
    const auto idProductUri = OpcUaNodeId(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI);

    auto reader = AttributeReader(client, maxBatchSize);
    reader.addAttribute({idI64, UA_ATTRIBUTEID_VALUE});
    reader.addAttribute({idI64, UA_ATTRIBUTEID_DISPLAYNAME});
    reader.addAttribute({idI32, UA_ATTRIBUTEID_VALUE});
    reader.addAttribute({idI32, UA_ATTRIBUTEID_DISPLAYNAME});
    reader.addAttribute({idProductUri, UA_ATTRIBUTEID_VALUE});
    reader.addAttribute({idProductUri, UA_ATTRIBUTEID_BROWSENAME});
    reader.addAttribute({idI16, UA_ATTRIBUTEID_VALUE});
    reader.read();

    OpcUaVariant variant;

    variant = reader.getValue(idI64, UA_ATTRIBUTEID_VALUE);
    ASSERT_EQ(64, variant.toInteger());

    variant = reader.getValue(idI64, UA_ATTRIBUTEID_DISPLAYNAME);
    ASSERT_EQ(".i64", variant.toString());

    variant = reader.getValue(idI32, UA_ATTRIBUTEID_VALUE);
    ASSERT_EQ(41, variant.toInteger());

    variant = reader.getValue(idI32, UA_ATTRIBUTEID_DISPLAYNAME);
    ASSERT_EQ(".i32", variant.toString());

    variant = reader.getValue(idProductUri, UA_ATTRIBUTEID_VALUE);
    ASSERT_EQ("http://open62541.org", variant.toString());

    variant = reader.getValue(idProductUri, UA_ATTRIBUTEID_BROWSENAME);
    ASSERT_EQ("ProductUri", variant.toString());

    variant = reader.getValue(idI16, UA_ATTRIBUTEID_VALUE);
    ASSERT_EQ(16, variant.toInteger());
}

TEST_F(AttributeReaderTest, MultipleReads)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    const auto idI64 = OpcUaNodeId(1, ".i64");
    const auto idI32 = OpcUaNodeId(1, ".i32");

    auto reader = AttributeReader(client);
    reader.addAttribute({idI64, UA_ATTRIBUTEID_VALUE});
    reader.addAttribute({idI64, UA_ATTRIBUTEID_BROWSENAME});
    reader.read();

    reader.addAttribute({idI32, UA_ATTRIBUTEID_VALUE});
    reader.addAttribute({idI32, UA_ATTRIBUTEID_BROWSENAME});
    reader.read();

    ASSERT_NO_THROW(reader.getValue({idI64, UA_ATTRIBUTEID_VALUE}));
    ASSERT_NO_THROW(reader.getValue({idI64, UA_ATTRIBUTEID_BROWSENAME}));
    ASSERT_NO_THROW(reader.getValue({idI32, UA_ATTRIBUTEID_VALUE}));
    ASSERT_NO_THROW(reader.getValue({idI32, UA_ATTRIBUTEID_BROWSENAME}));
}

TEST_F(AttributeReaderTest, ClearAttributes)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    const auto idI64 = OpcUaNodeId(1, ".i64");
    const auto idI32 = OpcUaNodeId(1, ".i32");

    auto reader = AttributeReader(client);
    reader.addAttribute({idI64, UA_ATTRIBUTEID_VALUE});
    reader.clearAttributes();
    reader.addAttribute({idI64, UA_ATTRIBUTEID_BROWSENAME});
    reader.read();

    ASSERT_NO_THROW(reader.getValue({idI64, UA_ATTRIBUTEID_BROWSENAME}));
    ASSERT_THROW(reader.getValue({idI64, UA_ATTRIBUTEID_VALUE}), OpcUaException);
}

TEST_F(AttributeReaderTest, ClearResults)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    const auto idI64 = OpcUaNodeId(1, ".i64");
    const auto idI32 = OpcUaNodeId(1, ".i32");

    auto reader = AttributeReader(client);
    reader.addAttribute({idI64, UA_ATTRIBUTEID_VALUE});
    reader.clearAttributes();
    reader.addAttribute({idI64, UA_ATTRIBUTEID_BROWSENAME});
    reader.read();

    ASSERT_NO_THROW(reader.getValue({idI64, UA_ATTRIBUTEID_BROWSENAME}));
    ASSERT_THROW(reader.getValue({idI64, UA_ATTRIBUTEID_VALUE}), OpcUaException);
}



END_NAMESPACE_OPENDAQ_OPCUA
