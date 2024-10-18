#include <gtest/gtest.h>
#include "tms_object_integration_test.h"
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_class_ptr.h>
#include <opcuatms_server/objects/tms_server_property.h>
#include <opcuatms_client/objects/tms_client_property_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/context_factory.h>

using namespace daq;
using namespace opcua::tms;
using namespace opcua;
using namespace std::chrono_literals;

struct RegisteredProperty
{
    TmsServerPropertyPtr serverProp;
    PropertyPtr clientProp;
};

class TmsPropertyTest : public TmsObjectIntegrationTest
{
public:

    PropertyBuilderPtr createIntPropertyBuilder()
    {
        return IntPropertyBuilder("Age", 10).setDescription("Hello world.").setMinValue(-100).setMaxValue(100);
    }

    PropertyPtr createIntProperty()
    {
        return createIntPropertyBuilder().build();
    }

    PropertyBuilderPtr createEnumPropertyBuilder()
    {
        auto values = List<IString>();
        values.pushBack("banana");
        values.pushBack("apple");
        values.pushBack("blueberry");
        
        return SelectionPropertyBuilder("Fruits", values, 0).setDescription("Hello world.");
    }

    PropertyPtr createEnumProperty()
    {
        return createEnumPropertyBuilder().build();
    }

    PropertyBuilderPtr createDictEnumPropertyBuilder()
    {
        auto values = Dict<Int, IString>();
        values.set(0, "banana");
        values.set(2, "apple");
        values.set(5, "blueberry");
        
        return SparseSelectionPropertyBuilder("FruitsDict", values, 0).setDescription("Hello world.");
    }

    PropertyPtr createDictEnumProperty()
    {
        return createDictEnumPropertyBuilder().build();
    }

    RegisteredProperty registerProperty(const PropertyPtr& prop)
    {
        auto serverProp = std::make_shared<TmsServerProperty>(prop, server, ctx, serverContext);
        auto nodeId = serverProp->registerOpcUaNode();
        auto clientProp = TmsClientProperty(NullContext(), clientContext, nodeId);
        return {serverProp, clientProp};
    }
};

TEST_F(TmsPropertyTest, Create)
{
    auto prop = createIntProperty();

    auto serverProp = TmsServerProperty(prop, server, ctx, serverContext);
    auto nodeId = serverProp.registerOpcUaNode();
    ASSERT_TRUE(server->nodeExists(nodeId));

    auto clientProp = TmsClientProperty(NullContext(), clientContext, nodeId);
    ASSERT_TRUE(clientProp.assigned());
}

TEST_F(TmsPropertyTest, Name)
{
    auto prop = createIntProperty();
    auto [serverProp, clientProp] = registerProperty(prop);

    ASSERT_EQ(clientProp.getName(), prop.getName());
    ASSERT_THROW(client->writeDisplayName(serverProp->getNodeId(), "Name"), OpcUaException);
}

TEST_F(TmsPropertyTest, Description)
{
    auto prop = createIntProperty();
    auto [serverProp, clientProp] = registerProperty(prop);

    ASSERT_EQ(clientProp.getDescription(), prop.getDescription());
    ASSERT_THROW(client->writeDescription(serverProp->getNodeId(), "New description."), OpcUaException);
}

TEST_F(TmsPropertyTest, Unit)
{
    auto prop = createIntPropertyBuilder().setUnit(Unit("V", 1, "voltage")).build();

    auto [serverProp, clientProp] = registerProperty(prop);

    ASSERT_TRUE(BaseObjectPtr::Equals(clientProp.getUnit(), prop.getUnit()));
    ASSERT_THROW(writeChildNode(serverProp->getNodeId(), "Unit", OpcUaVariant()), OpcUaException);
}

TEST_F(TmsPropertyTest, UnitEval)
{
    auto prop = createIntPropertyBuilder().setUnit(EvalValue("Unit('V')")).build();

    auto [serverProp, clientProp] = registerProperty(prop);
    ASSERT_TRUE(BaseObjectPtr::Equals(clientProp.getUnit(), prop.getUnit()));
}

TEST_F(TmsPropertyTest, IsEnumEnum)
{
    auto prop = createEnumProperty();
    auto [serverProp, clientProp] = registerProperty(prop);
    ASSERT_TRUE(clientProp.getSelectionValues().assigned());
}

TEST_F(TmsPropertyTest, IsEnumInt)
{
    auto prop = createIntProperty();
    auto [serverProp, clientProp] = registerProperty(prop);
    ASSERT_FALSE(clientProp.getSelectionValues().assigned());
}

TEST_F(TmsPropertyTest, MinValue)
{
    auto prop = createIntProperty();
    auto [serverProp, clientProp] = registerProperty(prop);

    ASSERT_EQ(clientProp.getMinValue(), prop.getMinValue());
    ASSERT_THROW(writeChildNode(serverProp->getNodeId(), "MinValue", OpcUaVariant()), OpcUaException);
}

TEST_F(TmsPropertyTest, MinValueEnum)
{
    auto prop = createEnumProperty();
    auto [serverProp, clientProp] = registerProperty(prop);

    ASSERT_FALSE(clientProp.getMinValue().assigned());
}

TEST_F(TmsPropertyTest, MaxValue)
{
    auto prop = createIntProperty();
    auto [serverProp, clientProp] = registerProperty(prop);

    ASSERT_EQ(clientProp.getMaxValue(), prop.getMaxValue());
    ASSERT_THROW(writeChildNode(serverProp->getNodeId(), "MaxValue", OpcUaVariant()), OpcUaException);
}

TEST_F(TmsPropertyTest, MaxValueEnum)
{
    auto prop = createEnumProperty();
    auto [serverProp, clientProp] = registerProperty(prop);

    ASSERT_FALSE(clientProp.getMaxValue().assigned());
}

TEST_F(TmsPropertyTest, DefaultValue)
{
    auto prop = createEnumProperty();
    auto [serverProp, clientProp] = registerProperty(prop);

    ASSERT_EQ(clientProp.getDefaultValue(), prop.getDefaultValue());
    ASSERT_THROW(writeChildNode(serverProp->getNodeId(), "DefaultValue", OpcUaVariant()), OpcUaException);
}

TEST_F(TmsPropertyTest, IsVisible)
{
    auto prop = createIntPropertyBuilder().setVisible(true).build();
    auto [serverProp, clientProp] = registerProperty(prop);

    ASSERT_EQ(clientProp.getVisible(), prop.getVisible());
    ASSERT_THROW(writeChildNode(serverProp->getNodeId(), "IsVisible", OpcUaVariant()), OpcUaException);
}

TEST_F(TmsPropertyTest, IsVisibleEval)
{
    auto prop = createIntPropertyBuilder().setVisible(EvalValue("3 > 0")).build();
    auto [serverProp, clientProp] = registerProperty(prop);

    ASSERT_EQ(clientProp.getVisible(), prop.getVisible());
}

TEST_F(TmsPropertyTest, IsReadOnly)
{
    auto prop = createIntPropertyBuilder().setReadOnly(true).build();
    auto [serverProp, clientProp] = registerProperty(prop);
    
    ASSERT_EQ(clientProp.getReadOnly(), prop.getReadOnly());
    ASSERT_THROW(writeChildNode(serverProp->getNodeId(), "IsReadOnly", OpcUaVariant()), OpcUaException);
}

TEST_F(TmsPropertyTest, IsReadOnlyEval)
{
    auto prop = createIntPropertyBuilder().setReadOnly(EvalValue("3 < 0")).build();
    auto [serverProp, clientProp] = registerProperty(prop);

    ASSERT_EQ(clientProp.getReadOnly(), prop.getReadOnly());
}

TEST_F(TmsPropertyTest, EnumValues)
{
    auto prop = createEnumProperty();
    auto [serverProp, clientProp] = registerProperty(prop);

    auto sel1 = clientProp.getSelectionValues();
    auto sel2 = prop.getSelectionValues();

    ASSERT_EQ(clientProp.getSelectionValues(), prop.getSelectionValues());
    ASSERT_THROW(writeChildNode(serverProp->getNodeId(), "EnumValues", OpcUaVariant()), OpcUaException);
}

TEST_F(TmsPropertyTest, DictEnumValues)
{
    auto prop = createDictEnumProperty();
    auto [serverProp, clientProp] = registerProperty(prop);

    ASSERT_EQ(clientProp.getSelectionValues(), prop.getSelectionValues());
    ASSERT_THROW(writeChildNode(serverProp->getNodeId(), "EnumValues", OpcUaVariant()), OpcUaException);
}
