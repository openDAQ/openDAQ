#include <coreobjects/property_object_class_factory.h>
#include "coreobjects/property_object_factory.h"
#include "gtest/gtest.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuatms_client/objects/tms_client_property_object_factory.h"
#include "opcuatms_client/objects/tms_client_property_object_impl.h"
#include "opcuatms_server/objects/tms_server_property_object.h"
#include "tms_object_integration_test.h"
#include "coreobjects/argument_info_factory.h"
#include "coreobjects/callable_info_factory.h"
#include "coreobjects/coercer_factory.h"
#include "coreobjects/validator_factory.h"
#include "opendaq/instance_factory.h"
#include "coreobjects/unit_factory.h"
#include "coretypes/struct_factory.h"
#include "coretypes/type_manager_factory.h"
#include <coreobjects/property_object_protected_ptr.h>
#include <gmock/gmock.h>
#include "opendaq/device_type_factory.h"
#include "opendaq/server_type_factory.h"

using namespace daq;
using namespace opcua::tms;
using namespace opcua;
using namespace std::chrono_literals;

struct RegisteredPropertyObject
{
    TmsServerPropertyObjectPtr serverProp;
    PropertyObjectPtr clientProp;
};

// TODO: Add complex number type property test cases once implemented

class TmsPropertyObjectAdvancedTest : public TmsObjectIntegrationTest
{
public:
    TypeManagerPtr manager;

    void SetUp() override
    {
        TmsObjectIntegrationTest::SetUp();

        manager = TypeManager();

        manager.addType(StructType("ListRuleDescriptionStructure",
                                   List<IString>("Type", "Elements"),
                                   List<IType>(SimpleType(ctString), SimpleType(ctList))));

        manager.addType(StructType("CustomRuleDescriptionStructure",
                                   List<IString>("Type", "Parameters"),
                                   List<IType>(SimpleType(ctString), SimpleType(ctDict))));

        manager.addType(StructType("AdditionalParametersType",
                                   List<IString>("Parameters"),
                                   List<IType>(SimpleType(ctList))));

        manager.addType(StructType("KeyValuePair",
                                   List<IString>("Key", "Value"),
                                   List<IType>(SimpleType(ctString), SimpleType(ctUndefined))));


        auto functionProp = FunctionProperty(
            "function", FunctionInfo(ctString, List<IArgumentInfo>(ArgumentInfo("int", ctInt), ArgumentInfo("float", ctFloat))));
        FunctionPtr funcCallback = Function(
            [](ListPtr<IBaseObject> args)
            {
                int intVal = args[0];
                double floatVal = args[1];

                if (floatVal > intVal)
                    return String("Float is greater.");

                return String("Int is greater or equal.");
            });

        auto procProp =
            FunctionProperty("procedure",
                             ProcedureInfo(List<IArgumentInfo>(
                                 ArgumentInfo("ratio", ctRatio),
                                 ArgumentInfo("string", ctString),
                                 ArgumentInfo("bool", ctBool))));
        ProcedurePtr procCallback = Procedure(
            [&](ListPtr<IBaseObject> args)
            {
                testRatio = args[0];
                testString = args[1];
                testBool = args[2];
            });

        const auto obj = PropertyObject();
        obj.addProperty(IntProperty("ObjNumber", 0));
        obj.addProperty(functionProp);
        obj.setPropertyValue("function", funcCallback);
        obj.addProperty(procProp);
        obj.setPropertyValue("procedure", procCallback);

        auto obj1 = PropertyObject();
        obj1.addProperty(StringProperty("foo", "bar"));

        auto objClass = PropertyObjectClassBuilder("TestClass")
                            .addProperty(IntPropertyBuilder("Integer", 1)
                                             .setDescription("MyInteger")
                                             .setMaxValue(10)
                                             .setMinValue(0)
                                             .setSuggestedValues(List<IInteger>(1, 3, 5, 7, 10))
                                             .setUnit(EvalValue("Unit(%IntegerUnit:SelectedValue)"))
                                             .build())
                            .addProperty(FloatPropertyBuilder("Float", EvalValue("$Integer - 0.123"))
                                             .setDescription("MyFloat")
                                             .setMaxValue(EvalValue("$Integer + 1"))
                                             .setMinValue(0)
                                             .setSuggestedValues(EvalValue("[1.0, 3.0, 5.0, 7.0, 10.0] * if($Integer < 5, 1.0, 0.5)"))
                                             .build())
                            .addProperty(ListProperty("IntList", List<Int>(1, 2, 3, 4, 5)))
                            .addProperty(ListProperty("StringList", List<IString>("foo", "bar")))
                            .addProperty(BoolPropertyBuilder("BoolReadOnly", False).setReadOnly(True).build())
                            .addProperty(DictProperty("IntFloatDict", Dict<IInteger, IFloat>({{0, 1.123}, {2, 2.345}, {4, 3.456}})))
                            .addProperty(SelectionProperty("IntegerUnit", List<IString>("s", "ms"), 0, false))
                            .addProperty(SelectionProperty("Range", EvalValue("[50.0, 10.0, 1.0, 0.1] * if($Integer < 5, 1.0, 1.123)"), 0))
                            .addProperty(SparseSelectionProperty(
                                "StringSparseSelection", Dict<IInteger, IString>({{0, "foo"}, {10, "bar"}}), 10, EvalValue("$Integer < 5")))
                            .addProperty(ReferenceProperty("IntOrFloat", EvalValue("if($Float < 1, %Integer, %Float)")))
                            .addProperty(ObjectProperty("Object", obj))
                            .addProperty(StringPropertyBuilder("String", "foo").setReadOnly(EvalValue("$Float < 1.0")).build())
                            .addProperty(ReferenceProperty("ObjectOrString", EvalValue("if($Integer < 5, %Object, %String)")))
                            .addProperty(IntPropertyBuilder("ValidatedInt", 5).setValidator(Validator("Value < 10")).build())
                            .addProperty(IntPropertyBuilder("CoercedInt", 10).setCoercer(Coercer("if(Value > 10, 10, Value)")).build())
                            .addProperty(RatioProperty("Ratio", Ratio(1, 1000)))
                            .addProperty(ObjectPropertyBuilder("ObjectWithMetadata", obj1).setReadOnly(true).setVisible(false).build())
                            .addProperty(StructProperty("UnitStruct", Unit("s", -1, "second", "time")))
                            .addProperty(StructProperty("ArgumentStruct", ArgumentInfo("Arg", ctInt)))
                            .addProperty(StructProperty("RangeStruct", Range(1, 2)))
                            .addProperty(StructProperty("ComplexNumberStruct", ComplexNumber(1, 2)))
                            .addProperty(StructProperty("FunctionBlockTypeStruct", FunctionBlockType("id", "name", "desc")))
                            .addProperty(StructProperty("DeviceDomainStructure",
                                                        Struct("DeviceDomainStructure",
                                                               Dict<IString, IBaseObject>({{"Resolution", Ratio(10, 20)},
                                                                                           {"TicksSinceOrigin", 1000},
                                                                                           {"Origin", "origin"},
                                                                                           {"Unit",
                                                                                            Unit("symbol", -1, "name", "quantity")}}),
                                                               manager)))
                            .addProperty(StructProperty("ListRuleDescriptionStructure",
                                                        Struct("ListRuleDescriptionStructure",
                                                               Dict<IString, IBaseObject>(
                                                                   {{"Type", "list"}, {"Elements", List<IString>("foo", "bar")}}),
                                                               manager)))
                            .addProperty(StructProperty("CustomRuleDescriptionStructure",
                                                        Struct("CustomRuleDescriptionStructure",
                                                               Dict<IString, IBaseObject>(
                                                               {{"Type", "list"},
                                                                {"Parameters",
                                                                 Dict<IString, IBaseObject>({{"foo", "bar"}, {"foo1", "bar1"}})}}),
                                                               manager)))
                            .addProperty(StructProperty("AdditionalParametersType",
                                                        Struct("AdditionalParametersType",
                                                               Dict<IString, IBaseObject>({{"Parameters", List<IStruct>(
                                                                                                Struct("KeyValuePair",
                                                                                                    Dict<IString, IBaseObject>(
                                                                                                    {{"Key", "key1"},
                                                                                                        {"Value", "value1"}}),
                                                                                                    manager),
                                                                                                Struct("KeyValuePair",
                                                                                                    Dict<IString, IBaseObject>(
                                                                                                    {{"Key", "key1"},
                                                                                                        {"Value", "value1"}}),
                                                                                                    manager))}}),
                                                               manager)))
                            .build();

        manager.addType(objClass);
    }

    void TearDown() override
    {
        TmsObjectIntegrationTest::TearDown();
    }

    RegisteredPropertyObject registerPropertyObject(const PropertyObjectPtr& prop)
    {
        const auto context = Context(nullptr, logger, manager, nullptr);
        const auto serverProp =
            std::make_shared<TmsServerPropertyObject>(prop, server, context, std::make_shared<TmsServerContext>(context, nullptr));
        const auto nodeId = serverProp->registerOpcUaNode();
        const auto clientProp = TmsClientPropertyObject(Context(nullptr, logger, manager,nullptr), clientContext, nodeId);
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

    RatioPtr testRatio;
    StringPtr testString;
    BoolPtr testBool;
};

TEST_F(TmsPropertyObjectAdvancedTest, SetUpServer)
{
    const auto obj = PropertyObject(manager, "TestClass");

    const auto serverProp = std::make_shared<TmsServerPropertyObject>(obj, server, ctx, serverContext);
    const auto nodeId = serverProp->registerOpcUaNode();
}

TEST_F(TmsPropertyObjectAdvancedTest, SetUp)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);
}

TEST_F(TmsPropertyObjectAdvancedTest, PropertyCount)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    ASSERT_EQ(obj.getAllProperties().getCount(), clientObj.getAllProperties().getCount());
    ASSERT_EQ(obj.getVisibleProperties().getCount(), clientObj.getVisibleProperties().getCount());
}

TEST_F(TmsPropertyObjectAdvancedTest, MinMax)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    const auto serverIntProp = obj.getProperty("Integer");
    const auto clientIntProp = clientObj.getProperty("Integer");
    
    const auto serverFloatProp = obj.getProperty("Float");
    const auto clientFloatProp = clientObj.getProperty("Float");

    ASSERT_EQ(serverIntProp.getMinValue(), clientIntProp.getMinValue());
    ASSERT_EQ(serverIntProp.getMaxValue(), clientIntProp.getMaxValue());

    ASSERT_EQ(serverFloatProp.getMinValue(), clientFloatProp.getMinValue());
    ASSERT_EQ(serverFloatProp.getMaxValue(), clientFloatProp.getMaxValue());

    obj.setPropertyValue("Integer", 8);
    ASSERT_EQ(serverFloatProp.getMaxValue(), 9);
    ASSERT_EQ(clientFloatProp.getMaxValue(), 9);
}

TEST_F(TmsPropertyObjectAdvancedTest, SuggestedValues)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    const auto serverIntProp = obj.getProperty("Integer");
    const auto clientIntProp = clientObj.getProperty("Integer");

    const auto serverFloatProp = obj.getProperty("Float");
    const auto clientFloatProp = clientObj.getProperty("Float");

    ASSERT_EQ(serverIntProp.getSuggestedValues(), clientIntProp.getSuggestedValues());

    ListPtr<IFloat> serverFloatSuggestedValues = serverFloatProp.getSuggestedValues();
    ListPtr<IFloat> clientFloatSuggestedValues = clientFloatProp.getSuggestedValues();

    ASSERT_EQ(serverFloatSuggestedValues, clientFloatSuggestedValues);

    obj.setPropertyValue("Integer", 8);

    serverFloatSuggestedValues = serverFloatProp.getSuggestedValues();
    clientFloatSuggestedValues = clientFloatProp.getSuggestedValues();

    ASSERT_EQ(serverFloatSuggestedValues, clientFloatSuggestedValues);
}

TEST_F(TmsPropertyObjectAdvancedTest, Unit)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    const auto serverIntProp = obj.getProperty("Integer");
    const auto clientIntProp = clientObj.getProperty("Integer");

    UnitPtr serverUnit = serverIntProp.getUnit();
    UnitPtr clientUnit = clientIntProp.getUnit();

    ASSERT_EQ(serverUnit, clientUnit);

    obj.setPropertyValue("IntegerUnit", 1);

    serverUnit = serverIntProp.getUnit();
    clientUnit = clientIntProp.getUnit();

    ASSERT_EQ(serverUnit, clientUnit);
}

TEST_F(TmsPropertyObjectAdvancedTest, Description)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    const auto serverIntProp = obj.getProperty("Integer");
    const auto clientIntProp = clientObj.getProperty("Integer");

    const StringPtr serverDesc = serverIntProp.getDescription();
    const StringPtr clientDesc = clientIntProp.getDescription();

    ASSERT_EQ(serverDesc, clientDesc);
}

TEST_F(TmsPropertyObjectAdvancedTest, Visible)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    //const auto serverIntUnitProp = obj.getProperty("IntegerUnit");
    //const auto clientIntUnitProp = clientObj.getProperty("IntegerUnit");

    const auto serverSparseSelectionProp = obj.getProperty("StringSparseSelection");
    const auto clientSparseSelectionProp = clientObj.getProperty("StringSparseSelection");

    //ASSERT_EQ(serverIntUnitProp.getVisible(), clientIntUnitProp.getVisible());
    ASSERT_EQ(serverSparseSelectionProp.getVisible(), clientSparseSelectionProp.getVisible());

    obj.setPropertyValue("Integer", 9);

    ASSERT_EQ(serverSparseSelectionProp.getVisible(), false);
    ASSERT_EQ(clientSparseSelectionProp.getVisible(), false);
}

TEST_F(TmsPropertyObjectAdvancedTest, ReadOnly)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    const auto serverStringProp = obj.getProperty("String");
    const auto clientStringProp = clientObj.getProperty("String");

    ASSERT_EQ(serverStringProp.getReadOnly(), clientStringProp.getReadOnly());

    obj.setPropertyValue("Float", 3.0);
    ASSERT_EQ(serverStringProp.getReadOnly(), false);
    ASSERT_EQ(serverStringProp.getReadOnly(), false);
}

TEST_F(TmsPropertyObjectAdvancedTest, ReadOnlyPropValue)
{
    const auto obj = PropertyObject(manager, "TestClass");
    obj.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("BoolReadOnly", True);

    auto [serverObj, clientObj] = registerPropertyObject(obj);

    const bool value = clientObj.getPropertyValue("BoolReadOnly");
    ASSERT_TRUE(value);
}

TEST_F(TmsPropertyObjectAdvancedTest, DefaultValues)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    for (auto serverProp : obj.getAllProperties())
    {
        auto name = serverProp.getName();
        auto clientProp = clientObj.getProperty(name);

        if (serverProp.getValueType() != ctObject && serverProp.getValueType() != ctUndefined)
            ASSERT_EQ(serverProp.getDefaultValue(), clientProp.getDefaultValue());
    }

    obj.setPropertyValue("Integer", 5);
    ASSERT_DOUBLE_EQ(obj.getProperty("Float").getDefaultValue(), 4.877);
    ASSERT_DOUBLE_EQ(clientObj.getProperty("Float").getDefaultValue(), 4.877);
}

TEST_F(TmsPropertyObjectAdvancedTest, IntFloatGetSet)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    ASSERT_EQ(obj.getPropertyValue("Integer"), clientObj.getPropertyValue("Integer"));
    ASSERT_DOUBLE_EQ(obj.getPropertyValue("Float"), clientObj.getPropertyValue("Float"));

    ASSERT_NO_THROW(clientObj.setPropertyValue("Integer", 5));
    ASSERT_NO_THROW(clientObj.setPropertyValue("Integer", -1));
    ASSERT_NO_THROW(clientObj.setPropertyValue("Integer", 11));

    ASSERT_NO_THROW(clientObj.setPropertyValue("Float", 2.5));
    ASSERT_NO_THROW(clientObj.setPropertyValue("Float", 8.8));
    ASSERT_NO_THROW(clientObj.setPropertyValue("Float", -1.2));

    ASSERT_EQ(obj.getPropertyValue("Integer"), clientObj.getPropertyValue("Integer"));
    ASSERT_EQ(10, clientObj.getPropertyValue("Integer"));
    ASSERT_DOUBLE_EQ(obj.getPropertyValue("Float"), clientObj.getPropertyValue("Float"));
    ASSERT_DOUBLE_EQ(0, clientObj.getPropertyValue("Float"));
}

TEST_F(TmsPropertyObjectAdvancedTest, StringGetSet)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    ASSERT_EQ(obj.getPropertyValue("String"), clientObj.getPropertyValue("String"));

    ASSERT_NO_THROW(clientObj.setPropertyValue("Float", 5.123));
    ASSERT_NO_THROW(clientObj.setPropertyValue("String", "bar"));

    ASSERT_EQ(obj.getPropertyValue("String"), "bar");
    ASSERT_EQ(clientObj.getPropertyValue("String"), "bar");
}

TEST_F(TmsPropertyObjectAdvancedTest, IntListGetSet)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    ListPtr<Int> serverListObj = obj.getPropertyValue("IntList");
    ListPtr<Int> clientListObj = clientObj.getPropertyValue("IntList");

    for (SizeT i = 0; i < serverListObj.getCount(); ++i)
        ASSERT_EQ(serverListObj[i], clientListObj[i]);

    clientObj.setPropertyValue("IntList", List<Int>(3, 4, 5, 6, 7));

    serverListObj = obj.getPropertyValue("IntList");
    clientListObj = clientObj.getPropertyValue("IntList");

    for (SizeT i = 0; i < serverListObj.getCount(); ++i)
        ASSERT_EQ(serverListObj[i], clientListObj[i]);

}

TEST_F(TmsPropertyObjectAdvancedTest, RatioGetSet)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    ASSERT_EQ(obj.getPropertyValue("Ratio"), clientObj.getPropertyValue("Ratio"));

    ASSERT_NO_THROW(clientObj.setPropertyValue("Ratio", Ratio(1, 2000)));

    ASSERT_EQ(obj.getPropertyValue("Ratio"), Ratio(1, 2000));
    ASSERT_EQ(clientObj.getPropertyValue("Ratio"), Ratio(1, 2000));
}

TEST_F(TmsPropertyObjectAdvancedTest, StringListGetSet)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    ListPtr<IString> serverListObj = obj.getPropertyValue("StringList");
    ListPtr<IString> clientListObj = clientObj.getPropertyValue("StringList");

    ASSERT_EQ(serverListObj[0], clientListObj[0]);
    ASSERT_EQ(serverListObj[1], clientListObj[1]);
    
    const auto list = List<IString>("bar", "foo");
    clientObj.setPropertyValue("StringList", list);

    serverListObj = obj.getPropertyValue("StringList");
    clientListObj = clientObj.getPropertyValue("StringList");
    
    ASSERT_EQ(serverListObj[0], clientListObj[0]);
    ASSERT_EQ(serverListObj[1], clientListObj[1]);
}

TEST_F(TmsPropertyObjectAdvancedTest, IntFloatDictGetSet)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    DictPtr<Int, Float> serverDict = obj.getPropertyValue("IntFloatDict");
    DictPtr<Int, Float> clientDict = clientObj.getPropertyValue("IntFloatDict");

    for (auto key : serverDict.getKeyList())
        ASSERT_EQ(serverDict.get(key), clientDict.get(key));

    auto dict = Dict<Int, Float>();
    dict.set(10, 5.678);
    clientObj.setPropertyValue("IntFloatDict", dict);

    serverDict = obj.getPropertyValue("IntFloatDict");
    clientDict = clientObj.getPropertyValue("IntFloatDict");

    for (auto key : serverDict.getKeyList())
        ASSERT_EQ(serverDict.get(key), clientDict.get(key));
}


TEST_F(TmsPropertyObjectAdvancedTest, ObjectGetSet)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto nonClassObj = PropertyObject();
    nonClassObj.addProperty(IntProperty("test", 0));
    obj.addProperty(ObjectProperty("NonClassObj", nonClassObj));

    auto [serverObj, clientObj] = registerPropertyObject(obj);

    PropertyObjectPtr serverChildObj = obj.getPropertyValue("Object");
    PropertyObjectPtr clientChildObj = clientObj.getPropertyValue("Object");
    ASSERT_EQ(serverChildObj.getPropertyValue("ObjNumber"), clientChildObj.getPropertyValue("ObjNumber"));

    auto testObj = PropertyObject();
    testObj.addProperty(IntProperty("test", 0));

    ASSERT_NO_THROW(clientObj.setPropertyValue("Object", testObj));
    ASSERT_EQ(getLastMessage(), "Failed to set value for property \"Object\" on OpcUA client property object: Object type properties cannot be set over OpcUA");

    ASSERT_NO_THROW(clientChildObj.setPropertyValue("ObjNumber", 1));

    ASSERT_EQ(clientChildObj.getPropertyValue("ObjNumber"), 1);
    PropertyObjectPtr serverNonClassObj = obj.getPropertyValue("NonClassObj");
    PropertyObjectPtr clientNonClassObj = clientObj.getPropertyValue("NonClassObj");
    ASSERT_EQ(serverNonClassObj.getPropertyValue("test"), clientNonClassObj.getPropertyValue("test"));

    clientNonClassObj.setPropertyValue("test", 1);
    ASSERT_EQ(serverNonClassObj.getPropertyValue("test"), clientNonClassObj.getPropertyValue("test"));

    serverNonClassObj.setPropertyValue("test", 2);
    ASSERT_EQ(serverNonClassObj.getPropertyValue("test"), clientNonClassObj.getPropertyValue("test"));
}

TEST_F(TmsPropertyObjectAdvancedTest, ReferencedGetSet)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    auto serverReferenceProp = obj.getProperty("IntOrFloat");
    auto clientReferenceProp = clientObj.getProperty("IntOrFloat");

    auto defValue1 = serverReferenceProp.getDefaultValue();
    auto defValue2 = clientReferenceProp.getDefaultValue();
    ASSERT_EQ(serverReferenceProp.getDefaultValue(), clientReferenceProp.getDefaultValue());

    ASSERT_EQ(obj.getPropertyValue("IntOrFloat"), 1);
    ASSERT_EQ(clientObj.getPropertyValue("IntOrFloat"), 1);

    clientObj.setPropertyValue("Integer", 5);
    clientObj.setPropertyValue("Float", 1.123);
    
    ASSERT_DOUBLE_EQ(serverReferenceProp.getDefaultValue(), 4.877);
    ASSERT_DOUBLE_EQ(clientReferenceProp.getDefaultValue(), 4.877);

    clientObj.setPropertyValue("IntOrFloat", 2.345);
    ASSERT_DOUBLE_EQ(obj.getPropertyValue("Float"), 2.345);
    ASSERT_DOUBLE_EQ(clientObj.getPropertyValue("Float"), 2.345);

    ASSERT_EQ(obj.getPropertyValue("IntOrFloat"), 2.345);
    ASSERT_EQ(clientObj.getPropertyValue("IntOrFloat"), 2.345);
}

TEST_F(TmsPropertyObjectAdvancedTest, ReferencedGetSetObj)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    auto serverReferenceProp = obj.getProperty("ObjectOrString");
    auto clientReferenceProp = clientObj.getProperty("ObjectOrString");

    PropertyObjectPtr serverDefaultValue = serverReferenceProp.getDefaultValue();
    PropertyObjectPtr clientDefaultValue = clientReferenceProp.getDefaultValue();

    ASSERT_EQ(serverDefaultValue.getPropertyValue("ObjNumber"), clientDefaultValue.getPropertyValue("ObjNumber"));

    clientObj.setPropertyValue("Integer", 7);

    ASSERT_EQ(serverReferenceProp.getDefaultValue(), clientReferenceProp.getDefaultValue());

    clientObj.setPropertyValue("ObjectOrString", "foobar");
    ASSERT_EQ(clientObj.getPropertyValue("String"), "foobar");
    ASSERT_EQ(obj.getPropertyValue("String"), "foobar");
}

TEST_F(TmsPropertyObjectAdvancedTest, SelectionListGetSet)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    auto serverIntegerUnitProp = obj.getProperty("IntegerUnit");
    auto clientIntegerUnitProp = clientObj.getProperty("IntegerUnit");

    auto serverRangeProp = obj.getProperty("Range");
    auto clientRangeProp = clientObj.getProperty("Range");

    ListPtr<Float> serverRangeSelectionValues = serverRangeProp.getSelectionValues();
    ListPtr<Float> clientRangeSelectionValues = clientRangeProp.getSelectionValues();

    for (SizeT i = 0; i < serverRangeSelectionValues.getCount(); ++i)
        ASSERT_DOUBLE_EQ(serverRangeSelectionValues[i], clientRangeSelectionValues[i]);

    ListPtr<IString> serverIntegerUnitSelectionValues = serverIntegerUnitProp.getSelectionValues();
    ListPtr<IString> clientIntegerUnitSelectionValues = clientIntegerUnitProp.getSelectionValues();

    for (SizeT i = 0; i < serverIntegerUnitSelectionValues.getCount(); ++i)
        ASSERT_EQ(serverIntegerUnitSelectionValues[i], clientIntegerUnitSelectionValues[i]);

    ASSERT_DOUBLE_EQ(clientObj.getPropertySelectionValue("Range"), obj.getPropertySelectionValue("Range"));
    clientObj.setPropertyValue("Range", 1);
    ASSERT_DOUBLE_EQ(clientObj.getPropertySelectionValue("Range"), obj.getPropertySelectionValue("Range"));

    ASSERT_EQ(clientObj.getPropertySelectionValue("IntegerUnit"), obj.getPropertySelectionValue("IntegerUnit"));
    clientObj.setPropertyValue("IntegerUnit", 1);
    ASSERT_EQ(clientObj.getPropertySelectionValue("IntegerUnit"), obj.getPropertySelectionValue("IntegerUnit"));
}

TEST_F(TmsPropertyObjectAdvancedTest, SelectionDictGetSet)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    auto serverStringSparseSelectionProp = obj.getProperty("StringSparseSelection");
    auto clientStringSparseSelectionProp = clientObj.getProperty("StringSparseSelection");

    ASSERT_EQ(serverStringSparseSelectionProp.getSelectionValues(), clientStringSparseSelectionProp.getSelectionValues());
}

TEST_F(TmsPropertyObjectAdvancedTest, ValidationCoercion)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    ASSERT_EQ(obj.getPropertyValue("ValidatedInt"), clientObj.getPropertyValue("ValidatedInt"));
    ASSERT_EQ(obj.getPropertyValue("CoercedInt"), clientObj.getPropertyValue("CoercedInt"));
    
    ASSERT_NO_THROW(clientObj.setPropertyValue("ValidatedInt", 11));
    ASSERT_EQ(getLastMessage(), "Failed to set value for property \"ValidatedInt\" on OpcUA client property object: Writing property value");
    ASSERT_EQ(obj.getPropertyValue("ValidatedInt"), 5);
    ASSERT_EQ(clientObj.getPropertyValue("ValidatedInt"), 5);

    ASSERT_NO_THROW(clientObj.setPropertyValue("CoercedInt", 15));
    ASSERT_EQ(obj.getPropertyValue("CoercedInt"), 10);
    ASSERT_EQ(clientObj.getPropertyValue("CoercedInt"), 10);
}

TEST_F(TmsPropertyObjectAdvancedTest, NestedObjTest)
{
    const auto obj = PropertyObject(manager, "TestClass");

    auto obj1 = PropertyObject();
    auto obj2 = PropertyObject();
    obj2.addProperty(IntProperty("foo", 10));
    obj1.addProperty(ObjectProperty("child", obj2));
    obj.addProperty(ObjectProperty("child", obj1));

    auto [serverObj, clientObj] = registerPropertyObject(obj);

    ASSERT_EQ(clientObj.getPropertyValue("child.child.foo"), 10);
}

TEST_F(TmsPropertyObjectAdvancedTest, ObjectPropWithMetadata)
{
    const auto obj = PropertyObject(manager, "TestClass");

    const auto propObj = PropertyObject();
    propObj.addProperty(StringProperty("foo", "bar"));
    const auto newObjProp = ObjectPropertyBuilder("LocalObjectWithMetadata", propObj).setReadOnly(true).setVisible(false).build();
    obj.addProperty(newObjProp);

    auto [serverObj, clientObj] = registerPropertyObject(obj);

    PropertyObjectPtr clientObjWithMetadata = clientObj.getPropertyValue("ObjectWithMetadata");
    ASSERT_NO_THROW(clientObjWithMetadata.setPropertyValue("foo", "notbar"));
    ASSERT_EQ(clientObjWithMetadata.getPropertyValue("foo"), "notbar");

    PropertyObjectPtr clientLocalObjWithMetadata = clientObj.getPropertyValue("LocalObjectWithMetadata");
    ASSERT_NO_THROW(clientLocalObjWithMetadata.setPropertyValue("foo", "notbar"));
    ASSERT_EQ(clientLocalObjWithMetadata.getPropertyValue("foo"), "notbar");

    PropertyPtr clientObjectWithMetadataProp = clientObj.getProperty("ObjectWithMetadata");
    PropertyPtr serverObjectWithMetadataProp = obj.getProperty("ObjectWithMetadata");
    ASSERT_EQ(clientObjectWithMetadataProp.getVisible(), serverObjectWithMetadataProp.getVisible());
    ASSERT_EQ(clientObjectWithMetadataProp.getReadOnly(), serverObjectWithMetadataProp.getReadOnly());

    PropertyPtr clientLocalObjectWithMetadataProp = clientObj.getProperty("LocalObjectWithMetadata");
    PropertyPtr serverLocalObjectWithMetadataProp = obj.getProperty("LocalObjectWithMetadata");
    ASSERT_EQ(clientLocalObjectWithMetadataProp.getVisible(), serverLocalObjectWithMetadataProp.getVisible());
    ASSERT_EQ(clientLocalObjectWithMetadataProp.getReadOnly(), serverLocalObjectWithMetadataProp.getReadOnly());
}

TEST_F(TmsPropertyObjectAdvancedTest, FunctionCall)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    PropertyObjectPtr serverChildObj = obj.getPropertyValue("Object");
    PropertyObjectPtr clientChildObj = clientObj.getPropertyValue("Object");

    ListPtr<IBaseObject> inputArgs = List<IBaseObject>(1, 1.123);

    FunctionPtr serverFunc = serverChildObj.getPropertyValue("function");
    FunctionPtr clientFunc = clientChildObj.getPropertyValue("function");
    
    ASSERT_EQ(serverFunc(inputArgs), clientFunc(inputArgs));

    inputArgs = List<IBaseObject>(-5, -7.23);
    ASSERT_EQ(serverFunc(inputArgs), clientFunc(inputArgs));
}

TEST_F(TmsPropertyObjectAdvancedTest, ProcedureCall)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);
    
    PropertyObjectPtr clientChildObj = clientObj.getPropertyValue("Object");

    ListPtr<IBaseObject> inputArgs = List<IBaseObject>(Ratio(1, 2), "foo", true);

    ProcedurePtr clientFunc = clientChildObj.getPropertyValue("procedure");
    ASSERT_NO_THROW(clientFunc(inputArgs));

    ASSERT_EQ(testRatio, Ratio(1, 2));
    ASSERT_EQ(testString, "foo");
    ASSERT_EQ(testBool, true);

    testRatio = nullptr;
    testString = nullptr;
    testBool = nullptr;
}

TEST_F(TmsPropertyObjectAdvancedTest, StructureGet)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    //ASSERT_EQ(clientObj.getPropertyValue("UnitStruct"), obj.getPropertyValue("UnitStruct"));
    //ASSERT_EQ(clientObj.getPropertyValue("ArgumentStruct"), obj.getPropertyValue("ArgumentStruct"));
    ASSERT_EQ(clientObj.getPropertyValue("DeviceDomainStructure"), obj.getPropertyValue("DeviceDomainStructure"));
    ASSERT_EQ(clientObj.getPropertyValue("ListRuleDescriptionStructure"), obj.getPropertyValue("ListRuleDescriptionStructure"));
    ASSERT_EQ(clientObj.getPropertyValue("CustomRuleDescriptionStructure"), obj.getPropertyValue("CustomRuleDescriptionStructure"));
    ASSERT_EQ(clientObj.getPropertyValue("AdditionalParametersType"), obj.getPropertyValue("AdditionalParametersType"));
}

TEST_F(TmsPropertyObjectAdvancedTest, StructureSet)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    //clientObj.setPropertyValue("UnitStruct", Unit("V", 5, "volt", "voltage"));
    //clientObj.setPropertyValue("ArgumentStruct", ArgumentInfo("test", ctFloat));
    clientObj.setPropertyValue("DeviceDomainStructure",
                               Struct("DeviceDomainStructure",
                                      Dict<IString, IBaseObject>({{"Resolution", Ratio(20, 30)},
                                                                  {"TicksSinceOrigin", 5000},
                                                                  {"Origin", "origin1"},
                                                                  {"Unit", Unit("symbol1", 2, "name1", "quantity1")}}),
                                      manager));
    clientObj.setPropertyValue("ListRuleDescriptionStructure",
                               Struct("ListRuleDescriptionStructure",
                                      Dict<IString, IBaseObject>({{"Type", "list"}, {"Elements", List<IString>("foo1", "bar1")}}),
                                      manager));
    clientObj.setPropertyValue("CustomRuleDescriptionStructure",
                               Struct("CustomRuleDescriptionStructure",
                                      Dict<IString, IBaseObject>({{"Type", "list"},
                                                                  {"Parameters",
                                                                   Dict<IString, IBaseObject>({{"foo", "bar"}, {"foo1", "bar1"}})}}),
                                      manager));

    const auto keyValuePairList = List<IStruct>(
        Struct("KeyValuePair", Dict<IString, IBaseObject>({{"Key", "key2"}, {"Value", "value2"}}), manager),
        Struct("KeyValuePair", Dict<IString, IBaseObject>({{"Key", "key2"}, {"Value", "value2"}}), manager));

    clientObj.setPropertyValue("AdditionalParametersType",
                               Struct("AdditionalParametersType",
                                      Dict<IString, IBaseObject>({{"Parameters", keyValuePairList}}),
                                      manager));

    //ASSERT_EQ(clientObj.getPropertyValue("UnitStruct"), obj.getPropertyValue("UnitStruct"));
    //ASSERT_EQ(clientObj.getPropertyValue("ArgumentStruct"), obj.getPropertyValue("ArgumentStruct"));
    ASSERT_EQ(clientObj.getPropertyValue("DeviceDomainStructure"), obj.getPropertyValue("DeviceDomainStructure"));
    ASSERT_EQ(clientObj.getPropertyValue("ListRuleDescriptionStructure"), obj.getPropertyValue("ListRuleDescriptionStructure"));
    ASSERT_EQ(clientObj.getPropertyValue("CustomRuleDescriptionStructure"), obj.getPropertyValue("CustomRuleDescriptionStructure"));
    ASSERT_EQ(clientObj.getPropertyValue("AdditionalParametersType"), obj.getPropertyValue("AdditionalParametersType"));
}

TEST_F(TmsPropertyObjectAdvancedTest, PropertyOrder)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    auto serverProps = obj.getAllProperties();
    auto clientProps = clientObj.getAllProperties();

    ASSERT_EQ(serverProps.getCount(), clientProps.getCount());

    for (SizeT i = 0; i < serverProps.getCount(); ++i)
        ASSERT_EQ(serverProps[i].getName(), clientProps[i].getName());
}

TEST_F(TmsPropertyObjectAdvancedTest, GainScalingStructure)
{
    const auto typeManager = TypeManager();
    const auto type = StructType("GainScalingStructure", List<IString>("Factor", "Offset"), List<IType>(SimpleType(ctFloat), SimpleType(ctFloat)));
    typeManager.addType(type);

    const auto obj = PropertyObject();
    const auto structBuilder = StructBuilder("GainScalingStructure", typeManager).set("Factor", 0.5).set("Offset", 2.5);
    obj.addProperty(StructProperty("Gain", structBuilder.build()));
    

    const auto logger = Logger();
    const auto serverProp =
    std::make_shared<TmsServerPropertyObject>(obj, server, Context(nullptr, logger, manager, nullptr), serverContext);
    const auto nodeId = serverProp->registerOpcUaNode();

    auto [serverObj, clientObj] = registerPropertyObject(obj);
    
    const auto structObj = obj.getPropertyValue("Gain");
    const auto newBuilder = StructBuilder(structObj);
    clientObj.setPropertyValue("Gain", newBuilder.set("Factor", 2.0).set("Offset", 10.0).build());
}

TEST_F(TmsPropertyObjectAdvancedTest, BeginEndUpdate)
{
    bool eventTriggered = false;

    const auto obj = PropertyObject(manager, "TestClass");
    obj.addProperty(IntProperty("Prop1", 1, true));
    obj.addProperty(IntProperty("Prop2", 2, true));
    obj.addProperty(IntProperty("Prop3", 3, true));

    obj.getOnEndUpdate() += [&eventTriggered](PropertyObjectPtr&, EndUpdateEventArgsPtr& args)
    {
        ASSERT_EQ(args.getEventName(), "EndUpdateEvent");

        auto properties = args.getProperties();
        testing::ElementsAre("Prop1", "Prop2", "Prop3");

        eventTriggered = true;
    };

    auto [serverObj, clientObj] = registerPropertyObject(obj);

    clientObj.beginUpdate();
    clientObj.setPropertyValue("Prop1", 10);
    clientObj.setPropertyValue("Prop2", 20);
    clientObj.setPropertyValue("Prop3", 30);
    clientObj.endUpdate();

    ASSERT_TRUE(eventTriggered);
}

TEST_F(TmsPropertyObjectAdvancedTest, NativeStructTypes)
{
    const auto obj = PropertyObject(manager, "TestClass");
    auto [serverObj, clientObj] = registerPropertyObject(obj);

    ASSERT_EQ(obj.getPropertyValue("UnitStruct"), clientObj.getPropertyValue("UnitStruct"));
    ASSERT_EQ(obj.getPropertyValue("ArgumentStruct"), clientObj.getPropertyValue("ArgumentStruct"));
    ASSERT_EQ(obj.getPropertyValue("RangeStruct"), clientObj.getPropertyValue("RangeStruct"));
    ASSERT_EQ(obj.getPropertyValue("ComplexNumberStruct"), clientObj.getPropertyValue("ComplexNumberStruct"));
    ASSERT_EQ(obj.getPropertyValue("FunctionBlockTypeStruct"), clientObj.getPropertyValue("FunctionBlockTypeStruct"));

    clientObj.setPropertyValue("UnitStruct", Unit("new_symbol", 2, "new_name", "new_quantity"));
    clientObj.setPropertyValue("ArgumentStruct", ArgumentInfo("new_name", CoreType::ctFloat));
    clientObj.setPropertyValue("RangeStruct", Range(100, 2000));
    clientObj.setPropertyValue("ComplexNumberStruct", ComplexNumber(100, 2000));
    clientObj.setPropertyValue("FunctionBlockTypeStruct", FunctionBlockType("new_id", "new_name", "new_desc"));
    
    ASSERT_EQ(obj.getPropertyValue("UnitStruct"), Unit("new_symbol", 2, "new_name", "new_quantity"));
    ASSERT_EQ(obj.getPropertyValue("ArgumentStruct"), ArgumentInfo("new_name", CoreType::ctFloat));
    ASSERT_EQ(obj.getPropertyValue("RangeStruct"), Range(100, 2000));
    ASSERT_EQ(obj.getPropertyValue("ComplexNumberStruct"), ComplexNumber(100, 2000));
    ASSERT_EQ(obj.getPropertyValue("FunctionBlockTypeStruct"), FunctionBlockType("new_id", "new_name", "new_desc"));

    ASSERT_EQ(obj.getPropertyValue("UnitStruct"), clientObj.getPropertyValue("UnitStruct"));
    ASSERT_EQ(obj.getPropertyValue("ArgumentStruct"), clientObj.getPropertyValue("ArgumentStruct"));
    ASSERT_EQ(obj.getPropertyValue("RangeStruct"), clientObj.getPropertyValue("RangeStruct"));
    ASSERT_EQ(obj.getPropertyValue("ComplexNumberStruct"), clientObj.getPropertyValue("ComplexNumberStruct"));
    ASSERT_EQ(obj.getPropertyValue("FunctionBlockTypeStruct"), clientObj.getPropertyValue("FunctionBlockTypeStruct"));
}

