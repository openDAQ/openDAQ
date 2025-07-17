#include <coreobjects/property_object_factory.h>
#include <gtest/gtest.h>
#include <opcuaclient/opcuaclient.h>
#include <opcuatms_client/objects/tms_client_property_object_impl.h>
#include <opcuaclient/monitored_item_create_request.h>
#include <opcuaclient/subscriptions.h>
#include <future>
#include <coreobjects/property_object_class_ptr.h>
#include <opcuatms_server/objects/tms_server_property_object.h>
#include "tms_object_integration_test.h"
#include <opcuatms_client/objects/tms_client_property_object_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <opendaq/mock/advanced_components_setup_utils.h>

#include <coreobjects/callable_info_factory.h>
#include <opendaq/context_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>

using namespace daq;
using namespace opcua::tms;
using namespace opcua;
using namespace std::chrono_literals;

struct RegisteredPropertyObject
{
    TmsServerPropertyObjectPtr serverProp;
    PropertyObjectPtr clientProp;
};

class TmsPropertyObjectTest : public TmsObjectIntegrationTest
{
public:

    TypeManagerPtr manager;

    void SetUp() override
    {
        TmsObjectIntegrationTest::SetUp();
        manager = TypeManager();
        auto personClass = PropertyObjectClassBuilder("PersonObject").build();
        manager.addType(personClass);
    }

    void TearDown() override
    {
        TmsObjectIntegrationTest::TearDown();
    }

    PropertyObjectPtr createPropertyObject()
    {
        GenericPropertyObjectPtr object = PropertyObject(manager, "PersonObject");

        auto roles = List<IString>();
        roles.pushBack("Software");
        roles.pushBack("Hardware");

        object.addProperty(SelectionProperty("Role", roles, 0));
        object.addProperty(IntProperty("Height", 180));
        object.addProperty(IntProperty("Age", 0));
        object.setPropertyValue("Age", 999);

        // Is not registered
        //object.addProperty(PropertyObjectClassPtr::CreateStringPropInfo("Name"));
        //object.setPropertyValue("Name", "Glados");

        return object;
    }

    StringPtr getLastMessage()
    {
        logger.flush();
        auto sink = getPrivateSink();
        auto newMessage = sink.waitForMessage(2000);
        if (newMessage == 0)
            return StringPtr("");
        auto logMessage = sink.getLastMessage();
        return logMessage;
    }

    RegisteredPropertyObject registerPropertyObject(const PropertyObjectPtr& prop)
    {
        auto serverProp = std::make_shared<TmsServerPropertyObject>(prop, server, ctx, serverContext);
        auto nodeId = serverProp->registerOpcUaNode();
        auto clientProp = TmsClientPropertyObject(NullContext(logger), clientContext, nodeId);
        return {serverProp, clientProp};
    }
};

TEST_F(TmsPropertyObjectTest, Create)
{
    auto prop = createPropertyObject();

    auto serverProp = TmsServerPropertyObject(prop, server, ctx, serverContext);
    auto nodeId = serverProp.registerOpcUaNode();
    ASSERT_TRUE(server->nodeExists(nodeId));

    auto clientProp = TmsClientPropertyObject(NullContext(), clientContext, nodeId);
    ASSERT_TRUE(clientProp.assigned());
}

// TODO: Class names cannot be transferred as of now
TEST_F(TmsPropertyObjectTest, DISABLED_ClassName)
{
    auto prop = createPropertyObject();

    auto propClass = PropertyObjectClassBuilder("Chell").setParentName("PersonObject").build();
    manager.addType(propClass);

    auto [serverProp, clientProp] = registerPropertyObject(prop);

    ASSERT_EQ(clientProp.getClassName(), prop.getClassName());

    manager.removeType("Chell");
}

TEST_F(TmsPropertyObjectTest, PropertyValue)
{
    auto prop = createPropertyObject();
    auto [serverProp, clientProp] = registerPropertyObject(prop);

    ASSERT_EQ(clientProp.getPropertyValue("Height"), prop.getPropertyValue("Height"));
    clientProp.setPropertyValue("Height", 100);
    ASSERT_EQ(clientProp.getPropertyValue("Height"), 100);
    ASSERT_EQ(prop.getPropertyValue("Height"), 100);
    
    ASSERT_ANY_THROW(clientProp.setPropertyValue("Missing", 100));
    ASSERT_EQ(getLastMessage(), "Failed to set value for property \"Missing\" on OpcUA client property object: Property not found");
}

TEST_F(TmsPropertyObjectTest, PropertyValueRole)
{
    auto prop = createPropertyObject();
    auto [serverProp, clientProp] = registerPropertyObject(prop);

    ASSERT_EQ(clientProp.getPropertyValue("Role"), prop.getPropertyValue("Role"));
    clientProp.setPropertyValue("Role", 1);
    ASSERT_EQ(clientProp.getPropertyValue("Role"), 1);
    ASSERT_EQ(prop.getPropertyValue("Role"), 1);
    
    ASSERT_NO_THROW(clientProp.setPropertyValue("Role", 2));
    ASSERT_EQ(getLastMessage(), "Failed to set value for property \"Role\" on OpcUA client property object: Writing property value");
}

TEST_F(TmsPropertyObjectTest, getPropertySelectionValue)
{
    auto prop = createPropertyObject();
    auto [serverProp, clientProp] = registerPropertyObject(prop);
    const auto targetRole = "Hardware";

    ASSERT_EQ(clientProp.getPropertySelectionValue("Role"), prop.getPropertySelectionValue("Role"));
    clientProp.setPropertyValue("Role", 1);
    ASSERT_EQ(prop.getPropertySelectionValue("Role"), targetRole);
    ASSERT_EQ(clientProp.getPropertySelectionValue("Role"), targetRole);
}

TEST_F(TmsPropertyObjectTest, GetProperty)
{
    auto prop = createPropertyObject();
    auto [serverProp, clientProp] = registerPropertyObject(prop);
    
    auto height = clientProp.getProperty("Height");
    ASSERT_TRUE(height.assigned());
}

TEST_F(TmsPropertyObjectTest, EnumVisibleProperties)
{
    auto prop = createPropertyObject();
    auto [serverProp, clientProp] = registerPropertyObject(prop);

    auto infos = prop.getVisibleProperties();
    auto clientInfos = clientProp.getVisibleProperties();
    ASSERT_EQ(infos.getCount(), clientInfos.getCount());
}

// TODO: hasPropertyValue should only return true of local value is set
TEST_F(TmsPropertyObjectTest, HasProperty)
{
    auto prop = createPropertyObject();
    auto [serverProp, clientProp] = registerPropertyObject(prop);

    ASSERT_TRUE(clientProp.hasProperty("Age"));
    ASSERT_TRUE(clientProp.hasProperty("Role"));
    ASSERT_TRUE(clientProp.hasProperty("Height"));
    ASSERT_FALSE(clientProp.hasProperty("Missing"));
}

// TODO: Dictionary properties are not yet available on OpcUa TMS
TEST_F(TmsPropertyObjectTest, TestDictProperty)
{
    auto dict = Dict<IString, IInteger>();
    dict.set("BananaCount", 10);
    dict.set("AppleCount", 5);
    dict.set("BlueberryCount", 999);

    auto prop = DictProperty("DictProp", dict);
    auto propObj = PropertyObject();
    propObj.addProperty(prop);

    auto [serverProp, clientProp] = registerPropertyObject(propObj);
    auto clientDict = clientProp.getPropertyValue("DictProp");
    ASSERT_EQ(dict, clientDict);
}

// TODO: List properties are not yet available on OpcUa TMS
TEST_F(TmsPropertyObjectTest, TestListProperty)
{
    auto list = List<IString>("Banana", "Apple", "Blueberry");

    auto prop = ListProperty("ListProp", list);
    auto propObj = PropertyObject();
    propObj.addProperty(prop);

    auto [serverProp, clientProp] = registerPropertyObject(propObj);
    auto clientDict = clientProp.getPropertyValue("ListProp");
    ASSERT_EQ(list, clientDict);
}

TEST_F(TmsPropertyObjectTest, TestPropertyOrder)
{
    auto obj = PropertyObject();
    for (SizeT i = 0; i < 200; ++i)
        obj.addProperty(BoolProperty("Bool" + std::to_string(i), true));
    for (SizeT i = 0; i < 200; ++i)
        obj.addProperty(StringProperty("String" + std::to_string(i), "test"));
    for (SizeT i = 0; i < 200; ++i)
    {
        obj.addProperty(FunctionProperty("Func" + std::to_string(i), ProcedureInfo()));
        ProcedurePtr test = Procedure([](){});
        obj.setPropertyValue("Func" + std::to_string(i), test);
    }

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    auto serverProps = obj.getAllProperties();
    auto clientProps = clientObj.getAllProperties();

    ASSERT_EQ(serverProps.getCount(), clientProps.getCount());

    for (SizeT i = 0; i < serverProps.getCount(); ++i)
        ASSERT_EQ(serverProps[i].getName(), clientProps[i].getName());

}

TEST_F(TmsPropertyObjectTest, TestReadOnlyWrite)
{
    auto obj = PropertyObject();
    obj.addProperty(IntPropertyBuilder("ReadOnly", 0).setReadOnly(true).build());
    auto [serverObj, clientObj] = registerPropertyObject(obj);
    auto serverProps = obj.getAllProperties();
    auto clientProps = clientObj.getAllProperties();

    ASSERT_EQ(clientObj.getPropertyValue("ReadOnly"), 0);
    clientObj.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("ReadOnly", 100);
    ASSERT_EQ(clientObj.getPropertyValue("ReadOnly"), 100);
}

TEST_F(TmsPropertyObjectTest, TestReadOnlyWriteFail)
{
    auto obj = PropertyObject();
    obj.addProperty(IntPropertyBuilder("ReadOnly", 0).setReadOnly(true).build());
    auto [serverObj, clientObj] = registerPropertyObject(obj);
    auto serverProps = obj.getAllProperties();
    auto clientProps = clientObj.getAllProperties();

    ASSERT_EQ(clientObj.getPropertyValue("ReadOnly"), 0);
    ASSERT_THROW(clientObj.setPropertyValue("ReadOnly", 100), AccessDeniedException);
}

TEST_F(TmsPropertyObjectTest, DotAccessClient)
{
    PropertyObjectPtr obj = PropertyObject();
    PropertyObjectPtr child1 = PropertyObject();
    PropertyObjectPtr child2 = PropertyObject();

    child2.addProperty(StringProperty("foo", "bar"));
    child1.addProperty(ObjectProperty("child", child2));
    obj.addProperty(ObjectProperty("child", child1));

    auto [serverObj, clientObj] = registerPropertyObject(obj);

    auto prop = clientObj.getProperty("child.child.foo");

    ASSERT_EQ(clientObj.getPropertyValue("child.child.foo"), "bar");
    ASSERT_EQ(prop.getValue(), "bar");

    ASSERT_NO_THROW(clientObj.setPropertyValue("child.child.foo", "test"));
    ASSERT_EQ(obj.getPropertyValue("child.child.foo"), "test");
    ASSERT_EQ(prop.getValue(), "test");
    ASSERT_EQ(clientObj.getPropertyValue("child.child.foo"), "test");

    ASSERT_NO_THROW(prop.setValue("bar"));
    ASSERT_EQ(obj.getPropertyValue("child.child.foo"), "bar");
    ASSERT_EQ(prop.getValue(), "bar");
    ASSERT_EQ(clientObj.getPropertyValue("child.child.foo"), "bar");
}

TEST_F(TmsPropertyObjectTest, DotAccessClientSelection)
{
    PropertyObjectPtr obj = PropertyObject();
    PropertyObjectPtr child1 = PropertyObject();
    PropertyObjectPtr child2 = PropertyObject();

    child2.addProperty(SelectionProperty("fruit", List<IString>("apple", "orange", "banana"), 0));
    child1.addProperty(ObjectProperty("child", child2));
    obj.addProperty(ObjectProperty("child", child1));

    auto [serverObj, clientObj] = registerPropertyObject(obj);

    auto prop = clientObj.getProperty("child.child.fruit");

    ASSERT_EQ(clientObj.getPropertyValue("child.child.fruit"), 0);
    ASSERT_EQ(prop.getValue(), 0);
    ASSERT_EQ(clientObj.getPropertySelectionValue("child.child.fruit"), "apple");
    ASSERT_EQ(obj.getPropertySelectionValue("child.child.fruit"), "apple");

    ASSERT_NO_THROW(clientObj.setPropertyValue("child.child.fruit", 1));
    ASSERT_EQ(clientObj.getPropertyValue("child.child.fruit"), 1);
    ASSERT_EQ(prop.getValue(), 1);
    ASSERT_EQ(clientObj.getPropertySelectionValue("child.child.fruit"), "orange");
    ASSERT_EQ(obj.getPropertySelectionValue("child.child.fruit"), "orange");

    ASSERT_NO_THROW(prop.setValue(2));
    ASSERT_EQ(clientObj.getPropertyValue("child.child.fruit"), 2);
    ASSERT_EQ(prop.getValue(), 2);
    ASSERT_EQ(clientObj.getPropertySelectionValue("child.child.fruit"), "banana");
    ASSERT_EQ(obj.getPropertySelectionValue("child.child.fruit"), "banana");
}

TEST_F(TmsPropertyObjectTest, DotAccessClientServerChange)
{
    PropertyObjectPtr obj = PropertyObject();
    PropertyObjectPtr child1 = PropertyObject();
    PropertyObjectPtr child2 = PropertyObject();

    child2.addProperty(StringProperty("foo", "bar"));
    child1.addProperty(ObjectProperty("child", child2));
    obj.addProperty(ObjectProperty("child", child1));

    auto [serverObj, clientObj] = registerPropertyObject(obj);

    auto prop = clientObj.getProperty("child.child.foo");
    auto serverProp = obj.getProperty("child.child.foo");

    ASSERT_EQ(clientObj.getPropertyValue("child.child.foo"), "bar");
    ASSERT_EQ(prop.getValue(), "bar");

    ASSERT_NO_THROW(obj.setPropertyValue("child.child.foo", "test"));
    ASSERT_EQ(clientObj.getPropertyValue("child.child.foo"), "test");
    ASSERT_EQ(prop.getValue(), "test");

    ASSERT_NO_THROW(serverProp.setValue("bar"));
    ASSERT_EQ(clientObj.getPropertyValue("child.child.foo"), "bar");
    ASSERT_EQ(prop.getValue(), "bar");
}

TEST_F(TmsPropertyObjectTest, ProcedurePropWithListArg)
{
    PropertyObjectPtr obj = PropertyObject();
    auto argInfo = ListArgumentInfo("Int", ctInt);

    auto prop = FunctionProperty("ProcedureProp", ProcedureInfo(List<IArgumentInfo>(argInfo)));
    obj.addProperty(prop);
    auto proc = Procedure(
        [](const ListPtr<IBaseObject>& list)
        {
            for (const auto& val : list)
                ASSERT_EQ(val.getCoreType(), ctInt);
    });

    obj.setPropertyValue("ProcedureProp", proc);
    auto [serverObj, clientObj] = registerPropertyObject(obj);
    
    ASSERT_FALSE(clientObj.hasProperty("ProcedureProp"));
    
    // TODO: Procedure/Function properties with list/dictionary types are not yet supported over OPC UA!
    //proc = clientObj.getPropertyValue("ProcedureProp");

    //ASSERT_EQ(clientObj.getProperty("ProcedureProp").getCallableInfo().getArguments()[0], argInfo);

    //auto listArg = List<IBaseObject>(Integer(1), Integer(2));
    //proc(listArg);
}
TEST_F(TmsPropertyObjectTest, ProcedurePropWithDictArg)
{
    PropertyObjectPtr obj = PropertyObject();
    auto argInfo = DictArgumentInfo("Int", ctInt, ctString);

    auto prop = FunctionProperty("ProcedureProp", ProcedureInfo(List<IArgumentInfo>(argInfo)));
    obj.addProperty(prop);
    auto proc = Procedure(
        [](const DictPtr<IInteger, IString>& dict)
        {
            for (const auto& [key, val] : dict)
            {
                ASSERT_EQ(key.getCoreType(), ctInt);
                ASSERT_EQ(val.getCoreType(), ctString);
            }
        });

    obj.setPropertyValue("ProcedureProp", proc);

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    
    ASSERT_FALSE(clientObj.hasProperty("ProcedureProp"));

    // TODO: Procedure/Function properties with list/dictionary types are not yet supported over OPC UA!
    //proc = clientObj.getPropertyValue("ProcedureProp");

    //ASSERT_EQ(clientObj.getProperty("ProcedureProp").getCallableInfo().getArguments()[0], argInfo);

    //auto dictArg = Dict<IInteger, IString>({{0, "foo"}, {1, "bar"}});
    //proc(dictArg);
}

class TmsNestedPropertyObjectTest : public TmsObjectIntegrationTest
{
public:

    void SetUp() override
    {
        TmsObjectIntegrationTest::SetUp();
        serverObj = test_utils::createMockNestedPropertyObject();
        auto registeredObj = registerPropertyObject(serverObj);
        serverTMSObj = registeredObj.serverProp;
        clientObj = registeredObj.clientProp;
    }

    void TearDown() override
    {
        TmsObjectIntegrationTest::TearDown();
    }

    RegisteredPropertyObject registerPropertyObject(const PropertyObjectPtr& prop)
    {
        auto serverProp = std::make_shared<TmsServerPropertyObject>(prop, server, ctx, serverContext);
        auto nodeId = serverProp->registerOpcUaNode();
        auto clientProp = TmsClientPropertyObject(NullContext(logger), clientContext, nodeId);
        return {serverProp, clientProp};
    }

    PropertyObjectPtr serverObj;
    TmsServerPropertyObjectPtr serverTMSObj;
    PropertyObjectPtr clientObj;
};

TEST_F(TmsNestedPropertyObjectTest, TestNestedObjectClientGet)
{
    const PropertyObjectPtr child1 = clientObj.getPropertyValue("child1");
    const PropertyObjectPtr child2 = clientObj.getPropertyValue("child2");
    const PropertyObjectPtr child1_1 = child1.getPropertyValue("child1_1");
    const PropertyObjectPtr child1_2 = child1.getPropertyValue("child1_2");
    const PropertyObjectPtr child1_2_1 = child1_2.getPropertyValue("child1_2_1");
    const PropertyObjectPtr child2_1 = child2.getPropertyValue("child2_1");

    ASSERT_EQ(child1_2_1.getPropertyValue("String"), "String");
    ASSERT_DOUBLE_EQ(child1_1.getPropertyValue("Float"), 1.1);
    ASSERT_EQ(child1_2.getPropertyValue("Int"), 1);
    ASSERT_EQ(child2_1.getPropertyValue("Ratio"), Ratio(1,2));
}


TEST_F(TmsNestedPropertyObjectTest, TestNestedObjectClientGetDotAccess)
{
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(clientObj.getPropertyValue("child1.child1_1.Float"), 1.1);
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.Int"), 1);
    ASSERT_EQ(clientObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 2));
}

TEST_F(TmsNestedPropertyObjectTest, TestNestedObjectClientGetSelectionValueDotAccess)
{
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.Selection"), 0);
    ASSERT_EQ(clientObj.getPropertySelectionValue("child1.child1_2.child1_2_1.Selection"), "a");
    clientObj.setPropertyValue("child1.child1_2.child1_2_1.Selection", 1);
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.Selection"), 1);
    ASSERT_EQ(clientObj.getPropertySelectionValue("child1.child1_2.child1_2_1.Selection"), "b");
}

TEST_F(TmsNestedPropertyObjectTest, TestNestedObjectClientSet)
{
    const PropertyObjectPtr child1 = clientObj.getPropertyValue("child1");
    const PropertyObjectPtr child2 = clientObj.getPropertyValue("child2");
    const PropertyObjectPtr child1_1 = child1.getPropertyValue("child1_1");
    const PropertyObjectPtr child1_2 = child1.getPropertyValue("child1_2");
    const PropertyObjectPtr child1_2_1 = child1_2.getPropertyValue("child1_2_1");
    const PropertyObjectPtr child2_1 = child2.getPropertyValue("child2_1");
    
    child1_2_1.setPropertyValue("String", "new_string");
    child1_1.setPropertyValue("Float", 2.1);
    child1_2.setPropertyValue("Int", 2);
    child2_1.setPropertyValue("Ratio", Ratio(1, 5));

    ASSERT_EQ(serverObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_DOUBLE_EQ(serverObj.getPropertyValue("child1.child1_1.Float"), 2.1);
    ASSERT_EQ(serverObj.getPropertyValue("child1.child1_2.Int"), 2);
    ASSERT_EQ(serverObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 5));

    ASSERT_EQ(child1_2_1.getPropertyValue("String"), "new_string");
    ASSERT_DOUBLE_EQ(child1_1.getPropertyValue("Float"), 2.1);
    ASSERT_EQ(child1_2.getPropertyValue("Int"), 2);
    ASSERT_EQ(child2_1.getPropertyValue("Ratio"), Ratio(1, 5));
}

TEST_F(TmsNestedPropertyObjectTest, TestNestedObjectClientSetDotAccess)
{
    clientObj.setPropertyValue("child1.child1_2.child1_2_1.String", "new_string");
    clientObj.setPropertyValue("child1.child1_1.Float", 2.1);
    clientObj.setPropertyValue("child1.child1_2.Int", 2);
    clientObj.setPropertyValue("child2.child2_1.Ratio", Ratio(1, 5));
    
    ASSERT_EQ(serverObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_DOUBLE_EQ(serverObj.getPropertyValue("child1.child1_1.Float"), 2.1);
    ASSERT_EQ(serverObj.getPropertyValue("child1.child1_2.Int"), 2);
    ASSERT_EQ(serverObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 5));
    
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_DOUBLE_EQ(clientObj.getPropertyValue("child1.child1_1.Float"), 2.1);
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.Int"), 2);
    ASSERT_EQ(clientObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 5));
}

TEST_F(TmsNestedPropertyObjectTest, TestNestedObjectClientProtectedSet)
{
    ASSERT_THROW(clientObj.setPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString", "new_string"), AccessDeniedException);
    ASSERT_EQ(serverObj.getPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString"), "String");
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString"), "String");

    ASSERT_NO_THROW(clientObj.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString", "new_string"));
    ASSERT_EQ(serverObj.getPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString"), "new_string");
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.ReadOnlyString"), "new_string");
}

// TODO: Enable once clearing property values is supported via OPC UA
TEST_F(TmsNestedPropertyObjectTest, DISABLED_TestNestedObjectClientClear)
{
    clientObj.setPropertyValue("child1.child1_2.child1_2_1.String", "new_string");
    clientObj.setPropertyValue("child1.child1_1.Float", 2.1);
    clientObj.setPropertyValue("child1.child1_2.Int", 2);
    clientObj.setPropertyValue("child2.child2_1.Ratio", Ratio(1, 5));

    clientObj.clearPropertyValue("ObjectProperty");

    ASSERT_EQ(serverObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(serverObj.getPropertyValue("child1.child1_1.Float"), 1.1);
    ASSERT_EQ(serverObj.getPropertyValue("child1.child1_2.Int"), 1);
    ASSERT_EQ(serverObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 2));

    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(clientObj.getPropertyValue("child1.child1_1.Float"), 1.1);
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.Int"), 1);
    ASSERT_EQ(clientObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 2));

    clientObj.setPropertyValue("child1.child1_2.child1_2_1.String", "new_string");
    clientObj.setPropertyValue("child1.child1_1.Float", 2.1);
    clientObj.setPropertyValue("child1.child1_2.Int", 2);
    clientObj.setPropertyValue("child2.child2_1.Ratio", Ratio(1, 5));
    
    clientObj.clearPropertyValue("child1");

    ASSERT_EQ(serverObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(serverObj.getPropertyValue("child1.child1_1.Float"), 1.1);
    ASSERT_EQ(serverObj.getPropertyValue("child1.child1_2.Int"), 1);
    ASSERT_EQ(serverObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 5));

    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(clientObj.getPropertyValue("child1.child1_1.Float"), 1.1);
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.Int"), 1);
    ASSERT_EQ(clientObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 5));
}

TEST_F(TmsNestedPropertyObjectTest, TestNestedObjectServerSet)
{
    serverObj.setPropertyValue("child1.child1_2.child1_2_1.String", "new_string");
    serverObj.setPropertyValue("child1.child1_1.Float", 2.1);
    serverObj.setPropertyValue("child1.child1_2.Int", 2);
    serverObj.setPropertyValue("child2.child2_1.Ratio", Ratio(1, 5));
    
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_DOUBLE_EQ(clientObj.getPropertyValue("child1.child1_1.Float"), 2.1);
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.Int"), 2);
    ASSERT_EQ(clientObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 5));
}

TEST_F(TmsNestedPropertyObjectTest, TestNestedObjectServerClearIndividual)
{
    serverObj.setPropertyValue("child1.child1_2.child1_2_1.String", "new_string");
    serverObj.setPropertyValue("child1.child1_1.Float", 2.1);
    serverObj.setPropertyValue("child1.child1_2.Int", 2);
    serverObj.setPropertyValue("child2.child2_1.Ratio", Ratio(1, 5));

    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_DOUBLE_EQ(clientObj.getPropertyValue("child1.child1_1.Float"), 2.1);
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.Int"), 2);
    ASSERT_EQ(clientObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 5));
    
    serverObj.clearPropertyValue("child1.child1_2.child1_2_1.String");
    serverObj.clearPropertyValue("child1.child1_1.Float");
    serverObj.clearPropertyValue("child1.child1_2.Int");
    serverObj.clearPropertyValue("child2.child2_1.Ratio");

    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(clientObj.getPropertyValue("child1.child1_1.Float"), 1.1);
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.Int"), 1);
    ASSERT_EQ(clientObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 2));
}

TEST_F(TmsNestedPropertyObjectTest, TestNestedObjectServerClearObject)
{
    serverObj.setPropertyValue("child1.child1_2.child1_2_1.String", "new_string");
    serverObj.setPropertyValue("child1.child1_1.Float", 2.1);
    serverObj.setPropertyValue("child1.child1_2.Int", 2);
    serverObj.setPropertyValue("child2.child2_1.Ratio", Ratio(1, 5));
    
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "new_string");
    ASSERT_DOUBLE_EQ(clientObj.getPropertyValue("child1.child1_1.Float"), 2.1);
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.Int"), 2);
    ASSERT_EQ(clientObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 5));

    for (const auto& prop : serverObj.getAllProperties())
        serverObj.asPtr<IPropertyObjectProtected>().clearProtectedPropertyValue(prop.getName());
    
    ASSERT_EQ(serverObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(serverObj.getPropertyValue("child1.child1_1.Float"), 1.1);
    ASSERT_EQ(serverObj.getPropertyValue("child1.child1_2.Int"), 1);
    ASSERT_EQ(serverObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 2));
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "String");

    ASSERT_DOUBLE_EQ(clientObj.getPropertyValue("child1.child1_1.Float"), 1.1);
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.Int"), 1);
    ASSERT_EQ(clientObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 2));
    
    serverObj.setPropertyValue("child1.child1_2.child1_2_1.String", "new_string");
    serverObj.setPropertyValue("child1.child1_1.Float", 2.1);
    serverObj.setPropertyValue("child1.child1_2.Int", 2);
    serverObj.setPropertyValue("child2.child2_1.Ratio", Ratio(1, 5));

    serverObj.asPtr<IPropertyObjectProtected>().clearProtectedPropertyValue("child1");
    
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.child1_2_1.String"), "String");
    ASSERT_DOUBLE_EQ(clientObj.getPropertyValue("child1.child1_1.Float"), 1.1);
    ASSERT_EQ(clientObj.getPropertyValue("child1.child1_2.Int"), 1);
    ASSERT_EQ(clientObj.getPropertyValue("child2.child2_1.Ratio"), Ratio(1, 5));
}

TEST_F(TmsNestedPropertyObjectTest, TestNestedObjectClientFunctionCall)
{
    const PropertyObjectPtr child = clientObj.getPropertyValue("child1.child1_2.child1_2_1");
    FunctionPtr func1 = clientObj.getPropertyValue("child1.child1_2.child1_2_1.Function");
    FunctionPtr func2 = child.getPropertyValue("Function");

    ASSERT_EQ(func1(1), 1);
    ASSERT_EQ(func2(5), 5);
}

// NOTE: OPC UA does not propagate error codes.
TEST_F(TmsNestedPropertyObjectTest, TestNestedObjectClientProcedureCall)
{
    const PropertyObjectPtr child = clientObj.getPropertyValue("child1.child1_2.child1_2_1");
    ProcedurePtr proc1 = clientObj.getPropertyValue("child1.child1_2.child1_2_1.Procedure");
    ProcedurePtr proc2 = child.getPropertyValue("Procedure");

    ASSERT_NO_THROW(proc1(5));
    ASSERT_NO_THROW(proc1(0)); // Outputs warning
    ASSERT_NO_THROW(proc2(5));
    ASSERT_NO_THROW(proc2(0)); // Outputs warning
}
