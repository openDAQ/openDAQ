#include "gtest/gtest.h"
#include "tms_object_integration_test.h"
#include <opendaq/function_block_type_factory.h>
#include <opcuatms_server/objects/tms_server_function_block_type.h>
#include <opcuatms_client/objects/tms_client_function_block_type_factory.h>
#include <coreobjects/property_factory.h>
#include <testutils/test_comparators.h>
#include <opcuatms/converters/property_object_conversion_utils.h>

using namespace daq;
using namespace opcua::tms;
using namespace opcua;

class TmsFunctionBlockTypeTest : public TmsObjectIntegrationTest
{
public:
    FunctionBlockTypePtr createFunctionBlockType()
    {
        auto defaultConfig = PropertyObject();
        defaultConfig.addProperty(IntProperty("port", 1000));
        defaultConfig.addProperty(StringProperty("name", "vlado"));
        defaultConfig.addProperty(ListProperty("scaling", List<IFloat>(1.0, 2.0, 3.0)));

        return FunctionBlockType("ref_fb", "Reference function block", "Description", defaultConfig);
    }
};

TEST_F(TmsFunctionBlockTypeTest, Create)
{
    auto fbType = createFunctionBlockType();

    auto serverFbType = std::make_shared<TmsServerFunctionBlockType>(fbType, server, ctx, serverContext);
    auto nodeId = serverFbType->registerOpcUaNode();
    ASSERT_TRUE(server->nodeExists(nodeId));

    auto clientFbType = TmsClientFunctionBlockType(ctx, clientContext, nodeId);
    ASSERT_TRUE(clientFbType.assigned());
}

TEST_F(TmsFunctionBlockTypeTest, Values)
{
    auto fbType = createFunctionBlockType();
    auto serverFbType = std::make_shared<TmsServerFunctionBlockType>(fbType, server, ctx, serverContext);
    auto nodeId = serverFbType->registerOpcUaNode();
    auto clientFbType = TmsClientFunctionBlockType(ctx, clientContext, nodeId);

    ASSERT_EQ(fbType.getId(), clientFbType.getId());
    ASSERT_EQ(fbType.getName(), clientFbType.getName());
    ASSERT_EQ(fbType.getDescription(), clientFbType.getDescription());

    auto serverConfig = fbType.createDefaultConfig();
    auto clientConfig = clientFbType.createDefaultConfig();
    ASSERT_TRUE(TestComparators::PropertyObjectEquals(serverConfig, clientConfig));

    ASSERT_TRUE(TestComparators::FunctionBlockTypeEquals(fbType, clientFbType));
}

TEST_F(TmsFunctionBlockTypeTest, DefaultConfig)
{
    auto fbType = FunctionBlockType("id", "", "");
    auto serverFbType = std::make_shared<TmsServerFunctionBlockType>(fbType, server, ctx, serverContext);
    auto nodeId = serverFbType->registerOpcUaNode();
    auto clientFbType = TmsClientFunctionBlockType(ctx, clientContext, nodeId);

    ASSERT_TRUE(TestComparators::FunctionBlockTypeEquals(fbType, clientFbType));
}

TEST_F(TmsFunctionBlockTypeTest, CloneConfig)
{
    auto fbType = createFunctionBlockType();
    auto serverFbType = std::make_shared<TmsServerFunctionBlockType>(fbType, server, ctx, serverContext);
    auto nodeId = serverFbType->registerOpcUaNode();
    auto clientFbType = TmsClientFunctionBlockType(ctx, clientContext, nodeId);

    auto clientConfig1 = clientFbType.createDefaultConfig();
    auto clientConfig2 = clientFbType.createDefaultConfig();
    clientConfig1.setPropertyValue("name", "name1");
    clientConfig2.setPropertyValue("name", "name2");

    ASSERT_EQ(clientConfig1.getPropertyValue("name"), "name1");
    ASSERT_EQ(clientConfig2.getPropertyValue("name"), "name2");
}

TEST_F(TmsFunctionBlockTypeTest, ReadOnly)
{
    auto fbType = createFunctionBlockType();
    auto serverFbType = std::make_shared<TmsServerFunctionBlockType>(fbType, server, ctx, serverContext);
    auto nodeId = serverFbType->registerOpcUaNode();
    auto clientFbType = TmsClientFunctionBlockType(ctx, clientContext, nodeId);

    auto fbTypeVariant = VariantConverter<IFunctionBlockType>::ToVariant(fbType);
    ASSERT_THROW(client->writeValue(nodeId, fbTypeVariant), OpcUaException);
    ASSERT_THROW(client->writeDisplayName(nodeId, "Display name"), OpcUaException);
    ASSERT_THROW(client->writeDescription(nodeId, "Description"), OpcUaException);

    auto browser = CachedReferenceBrowser(client);
    const auto defaultConfigId = browser.getChildNodeId(nodeId, "DefaultConfig");
    const auto nameId = browser.getChildNodeId(defaultConfigId, "name");
    const auto portId = browser.getChildNodeId(defaultConfigId, "port");
    const auto scalingId = browser.getChildNodeId(defaultConfigId, "scaling");

    ASSERT_THROW(client->writeValue(nameId, OpcUaVariant("value")), OpcUaException);
    ASSERT_THROW(client->writeValue(portId, OpcUaVariant(1001)), OpcUaException);
    ASSERT_THROW(client->writeValue(scalingId, OpcUaVariant()), OpcUaException);
}

TEST_F(TmsFunctionBlockTypeTest, DISABLED_NonDefaultValues)
{
    // This should work, but it doesnt, because of an error in client property object.

    auto defaultConfig = PropertyObject();
    defaultConfig.addProperty(IntProperty("port", 0));
    defaultConfig.addProperty(StringProperty("name", ""));

    defaultConfig.setPropertyValue("port", 1000);
    defaultConfig.setPropertyValue("name", "vlado");

    auto fbType = FunctionBlockType("ref_fb", "Reference function block", "Description", defaultConfig);

    auto serverFbType = std::make_shared<TmsServerFunctionBlockType>(fbType, server, ctx, serverContext);
    auto nodeId = serverFbType->registerOpcUaNode();
    auto clientFbType = TmsClientFunctionBlockType(ctx, clientContext, nodeId);

    ASSERT_TRUE(TestComparators::FunctionBlockTypeEquals(fbType, clientFbType));
}

