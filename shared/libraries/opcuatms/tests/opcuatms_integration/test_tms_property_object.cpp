#include "coreobjects/property_object_factory.h"
#include "gtest/gtest.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuatms_client/objects/tms_client_property_object_impl.h"
#include <opcuaclient/monitored_item_create_request.h>
#include <opcuaclient/subscriptions.h>
#include <future>
#include "coreobjects/property_object_class_ptr.h"
#include "opcuatms_server/objects/tms_server_property_object.h"
#include "tms_object_integration_test.h"
#include "opcuatms_client/objects/tms_client_property_object_factory.h"
#include <coreobjects/property_object_class_factory.h>

#include <coreobjects/callable_info_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/logger_sink_last_message_private_ptr.h>

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

    static LoggerPtr createLoggerWithDebugSink(const LoggerSinkPtr& sink)
    {
        sink.setLevel(LogLevel::Debug);
        auto sinks = DefaultSinks(nullptr);
        sinks.pushBack(sink);
        return LoggerWithSinks(sinks);
    }

    StringPtr getLastMessage(const LoggerSinkPtr& sink) 
    {
        // return StringPtr();
        if(!sink.assigned())
        {
            throw ArgumentNullException("Sink must not be null");
        }
        auto sinkPtr =  sink.asPtrOrNull<ILastMessageLoggerSinkPrivate>();
        if (sinkPtr == nullptr)
        {
            throw InvalidTypeException("Wrong sink. GetLastMessage supports only by LastMessageLoggerSink");
        }

        return sinkPtr.getLastMessage();
    }

    Bool waitForMessage(const LoggerSinkPtr& sink, SizeT timeoutMs)
    {
        // return false;
        if(!sink.assigned())
        {
            throw ArgumentNullException("Sink must not be null");
        }
        auto sinkPtr =  sink.asPtrOrNull<ILastMessageLoggerSinkPrivate>();
        if (sinkPtr == nullptr)
        {
            throw InvalidTypeException("Wrong sink. GetLastMessage supports only by LastMessageLoggerSink");
        }

        return sinkPtr.waitForMessage(timeoutMs);
    }

    StringPtr getClientLastMessage()
    {
        clientLogger.flush();
        auto newMessage = waitForMessage(clientDebugSink, 2000);
        if (newMessage == 0)
            return StringPtr("");
        auto logMessage = getLastMessage(clientDebugSink);
        return logMessage;
    }

    RegisteredPropertyObject registerPropertyObject(const PropertyObjectPtr& prop)
    {
        auto serverProp = std::make_shared<TmsServerPropertyObject>(prop, server, NullContext(serverLogger));
        auto nodeId = serverProp->registerOpcUaNode();
        auto clientProp = TmsClientPropertyObject(NullContext(clientLogger), clientContext, nodeId);
        return {serverProp, clientProp};
    }

  
    LoggerSinkPtr serverDebugSink = LastMessageLoggerSink();
    LoggerSinkPtr clientDebugSink = LastMessageLoggerSink();
    LoggerPtr serverLogger = createLoggerWithDebugSink(serverDebugSink);
    LoggerPtr clientLogger = createLoggerWithDebugSink(clientDebugSink);
};

TEST_F(TmsPropertyObjectTest, Create)
{
    auto prop = createPropertyObject();

    auto serverProp = TmsServerPropertyObject(prop, server, NullContext());
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
    
    ASSERT_NO_THROW(clientProp.setPropertyValue("Missing", 100));
    ASSERT_EQ(getClientLastMessage(), "Property with name \"Missing\" is not found");
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
    ASSERT_EQ(getClientLastMessage(), "Failed to set value for existing property \"Role\" on OPC UA client");
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
        obj.addProperty(BoolProperty("bool" + std::to_string(i), true));
    for (SizeT i = 0; i < 200; ++i)
        obj.addProperty(StringProperty("string" + std::to_string(i), "test"));
    for (SizeT i = 0; i < 200; ++i)
    {
        obj.addProperty(FunctionProperty("func" + std::to_string(i), ProcedureInfo()));
        ProcedurePtr test = Procedure([](){});
        obj.setPropertyValue("func" + std::to_string(i), test);
    }

    auto [serverObj, clientObj] = registerPropertyObject(obj);
        auto serverProps = obj.getAllProperties();
    auto clientProps = clientObj.getAllProperties();

    ASSERT_EQ(serverProps.getCount(), clientProps.getCount());

    for (SizeT i = 0; i < serverProps.getCount(); ++i)
        ASSERT_EQ(serverProps[i].getName(), clientProps[i].getName());

}
