#include <coreobjects/property_object_class_factory.h>
#include "coreobjects/callable_info_factory.h"
#include "coreobjects/property_object_factory.h"
#include "gtest/gtest.h"
#include "opcuatms_client/objects/tms_client_property_object_factory.h"
#include "opcuatms_client/objects/tms_client_property_object_impl.h"
#include "opcuatms_server/objects/tms_server_property_object.h"
#include "tms_object_integration_test.h"
#include "coreobjects/argument_info_factory.h"
#include "opendaq/context_factory.h"

using namespace daq;
using namespace opcua::tms;
using namespace opcua;

struct RegisteredPropertyObject
{
    TmsServerPropertyObjectPtr serverProp;
    PropertyObjectPtr clientProp;
};

class TmsFunctionTest: public TmsObjectIntegrationTest
{
public:
    RegisteredPropertyObject registerPropertyObject(const PropertyObjectPtr& obj)
    {
        const auto serverProp = std::make_shared<TmsServerPropertyObject>(obj, server, ctx, serverContext);
        const auto nodeId = serverProp->registerOpcUaNode();
        const auto clientProp = TmsClientPropertyObject(NullContext(logger), clientContext, nodeId);
        return {serverProp, clientProp};
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
};

TEST_F(TmsFunctionTest, ProcedureNoArgs)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty("proc", ProcedureInfo()));
    bool called = false;
    auto proc = Procedure([&called]() { called = true; });
    obj.setPropertyValue("proc", proc);

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    ProcedurePtr clientProc = clientObj.getPropertyValue("proc");
    ASSERT_NO_THROW(clientProc());
    ASSERT_EQ(called, true);
}

TEST_F(TmsFunctionTest, FunctionNoArgs)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty("func", FunctionInfo(ctBool)));
    auto func = Function([]() { return true; });
    obj.setPropertyValue("func", func);

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    FunctionPtr clientFunc = clientObj.getPropertyValue("func");
    ASSERT_TRUE(clientFunc());
}

TEST_F(TmsFunctionTest, ProcedureOneArg)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty("proc", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("int", ctInt)))));
    Int callValue;
    auto proc = Procedure([&callValue](const IntegerPtr& arg) { callValue = arg; });
    obj.setPropertyValue("proc", proc);

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    ProcedurePtr clientProc = clientObj.getPropertyValue("proc");

    ASSERT_NO_THROW(clientProc(10));
    ASSERT_EQ(callValue, 10);

    ASSERT_NO_THROW(clientProc(100));
    ASSERT_EQ(callValue, 100);
}

TEST_F(TmsFunctionTest, FunctionOneArg)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty("func", FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("int", ctInt)))));
    Int callValue;
    auto func = Function([&callValue](const IntegerPtr& arg) { return arg; });
    obj.setPropertyValue("func", func);

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    FunctionPtr clientFunc = clientObj.getPropertyValue("func");
    
    ASSERT_EQ(clientFunc(10), 10);
    ASSERT_EQ(clientFunc(100), 100);
}

TEST_F(TmsFunctionTest, ProcedureMultipleArgs)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty(
        "proc", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("int", ctInt), ArgumentInfo("ratio", ctRatio), ArgumentInfo("string", ctString)))));

    Int intArg;
    RatioPtr ratioArg;
    StringPtr stringArg;
    auto proc = Procedure([&intArg, &ratioArg, &stringArg](const ListPtr<IBaseObject>& args)
        {
            intArg = args[0];
            ratioArg = args[1];
            stringArg = args[2];
        });
    obj.setPropertyValue("proc", proc);

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    ProcedurePtr clientProc = clientObj.getPropertyValue("proc");

    clientProc(10, Ratio(1, 20), "foo");

    ASSERT_EQ(intArg, 10);
    ASSERT_EQ(ratioArg, Ratio(1, 20));
    ASSERT_EQ(stringArg, "foo");
}

TEST_F(TmsFunctionTest, FunctionMultipleArgs)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty(
        "func",
        FunctionInfo(ctBool, List<IArgumentInfo>(ArgumentInfo("int", ctInt), ArgumentInfo("ratio", ctRatio), ArgumentInfo("string", ctString)))));

    auto func = Function([](const ListPtr<IBaseObject>& args)
        {
            bool valid = args[0] == 10;
            valid = valid && args[1] == Ratio(1, 20);
            valid = valid && args[2] == "foo";
            return valid;
        });
    obj.setPropertyValue("func", func);

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    FunctionPtr clientProc = clientObj.getPropertyValue("func");

    ASSERT_EQ(clientProc(10, Ratio(1, 20), "foo"), true);
    ASSERT_EQ(clientProc(10, Ratio(1, 20), "bar"), false);
}

TEST_F(TmsFunctionTest, AllArgTypes)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty(
        "proc",
        ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("int", ctInt),
                                          ArgumentInfo("ratio", ctRatio),
                                          ArgumentInfo("string", ctString),
                                          ArgumentInfo("bool", ctBool),
                                          ArgumentInfo("float", ctFloat)))));

    Int intArg;
    RatioPtr ratioArg;
    StringPtr stringArg;
    Bool boolArg;
    Float floatArg;
    auto proc = Procedure(
        [&intArg, &ratioArg, &stringArg, &boolArg, &floatArg](const ListPtr<IBaseObject>& args)
        {
            intArg = args[0];
            ratioArg = args[1];
            stringArg = args[2];
            boolArg = args[3];
            floatArg = args[4];
        });
    obj.setPropertyValue("proc", proc);

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    ProcedurePtr clientProc = clientObj.getPropertyValue("proc");

    clientProc(10, Ratio(1, 20), "foo", false, 1.123);

    ASSERT_EQ(intArg, 10);
    ASSERT_EQ(ratioArg, Ratio(1, 20));
    ASSERT_EQ(stringArg, "foo");
    ASSERT_EQ(boolArg, false);
    ASSERT_EQ(floatArg, 1.123);
}

TEST_F(TmsFunctionTest, InvalidArgTypes)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty("proc", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("int", ctInt)))));
    Int callValue;
    auto proc = Procedure([&callValue](const IntegerPtr& arg) { callValue = arg; });
    obj.setPropertyValue("proc", proc);

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    ProcedurePtr clientProc = clientObj.getPropertyValue("proc");

    ASSERT_NO_THROW(clientProc("foo"));
    ASSERT_EQ(getLastMessage(), "Failed to call procedure on OpcUA client. Error: \"Calling procedure\""); 
}

// NOTE: Should this throw an error?
TEST_F(TmsFunctionTest, InvalidReturnType)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty("func", FunctionInfo(ctBool)));
    auto func = Function([]() { return "str"; });
    obj.setPropertyValue("func", func);

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    FunctionPtr clientFunc = clientObj.getPropertyValue("func");
    
    ASSERT_EQ(clientFunc(), "str");
}

TEST_F(TmsFunctionTest, InvalidArgCount)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty("proc", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("int", ctInt)))));
    Int callValue;
    auto proc = Procedure([&callValue](const IntegerPtr& arg) { callValue = arg; });
    obj.setPropertyValue("proc", proc);

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    ProcedurePtr clientProc = clientObj.getPropertyValue("proc");

    ASSERT_NO_THROW(clientProc());
    ASSERT_EQ(getLastMessage(), "Failed to call procedure on OpcUA client. Error: \"Calling procedure\""); 

    ASSERT_NO_THROW(clientProc(1, 2));
    ASSERT_EQ(getLastMessage(), "Failed to call procedure on OpcUA client. Error: \"Calling procedure\""); 
}

TEST_F(TmsFunctionTest, ProcedureArgumentInfo)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty(
        "proc",
        ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("int", ctInt),
                                          ArgumentInfo("ratio", ctRatio),
                                          ArgumentInfo("string", ctString),
                                          ArgumentInfo("bool", ctBool),
                                          ArgumentInfo("float", ctFloat)))));
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    ASSERT_EQ(obj.getProperty("proc").getCallableInfo(), clientObj.getProperty("proc").getCallableInfo());
}

TEST_F(TmsFunctionTest, FunctionArgumentInfo)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty(
        "func",
        FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("int", ctInt),
                                                ArgumentInfo("ratio", ctRatio),
                                                ArgumentInfo("string", ctString),
                                                ArgumentInfo("bool", ctBool),
                                                ArgumentInfo("float", ctFloat)))));
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    ASSERT_EQ(obj.getProperty("func").getCallableInfo(), clientObj.getProperty("func").getCallableInfo());
}

TEST_F(TmsFunctionTest, UnsupportedArgumentInfo)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty("proc1", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("list", ctList)))));
    obj.addProperty(FunctionProperty("proc2", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("dict", ctDict)))));
    obj.addProperty(FunctionProperty("proc3", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("proc", ctProc)))));
    obj.addProperty(FunctionProperty("proc4", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("object", ctObject)))));
    obj.addProperty(FunctionProperty("proc5", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("binary_data", ctBinaryData)))));
    obj.addProperty(FunctionProperty("proc6", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("func", ctFunc)))));
    obj.addProperty(FunctionProperty("proc7", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("complex_number", ctComplexNumber)))));
    obj.addProperty(FunctionProperty("proc8", ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("undefined", ctUndefined)))));
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    ASSERT_EQ(clientObj.getAllProperties().getCount(), 0u);
}

TEST_F(TmsFunctionTest, UnsupportedReturnType)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty("func1", FunctionInfo(ctList)));
    obj.addProperty(FunctionProperty("func2", FunctionInfo(ctDict)));
    obj.addProperty(FunctionProperty("func3", FunctionInfo(ctProc)));
    obj.addProperty(FunctionProperty("func4", FunctionInfo(ctObject)));
    obj.addProperty(FunctionProperty("func5", FunctionInfo(ctBinaryData)));
    obj.addProperty(FunctionProperty("func6", FunctionInfo(ctFunc)));
    obj.addProperty(FunctionProperty("func7", FunctionInfo(ctComplexNumber)));
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    ASSERT_EQ(clientObj.getAllProperties().getCount(), 0u);
}


TEST_F(TmsFunctionTest, ServerThrow)
{
    auto obj = PropertyObject();
    obj.addProperty(FunctionProperty("func", FunctionInfo(ctBool)));
    auto func = Function([]()
    {
        throw GeneralErrorException{};
        return false;
    });
    obj.setPropertyValue("func", func);

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    FunctionPtr clientFunc = clientObj.getPropertyValue("func");
    ASSERT_NO_THROW(clientFunc());
    ASSERT_EQ(getLastMessage(), "Failed to call function on OpcUA client. Error in \"Calling function\""); 
}
