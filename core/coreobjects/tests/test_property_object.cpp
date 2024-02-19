#include <property_value_event_args_ptr.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <testutils/testutils.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coretypes/type_manager_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/ownable_ptr.h>
#include <coreobjects/coercer_factory.h>
#include <coreobjects/validator_factory.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <coretypes/inspectable_ptr.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/eval_value_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/argument_info_factory.h>
#include <coreobjects/property_object_internal_ptr.h>

using namespace daq;

static constexpr SizeT NumVisibleProperties = 12u;
static constexpr SizeT NumAllProperties = 14u;

class PropertyObjectTest : public testing::Test
{
protected:
    PropertyObjectClassPtr testPropClass;
    PropertyObjectClassBuilderPtr testPropClassBuilder;
    BaseObjectPtr propValue;
    BaseObjectPtr propName;
    TypeManagerPtr objManager;

    void SetUp() override
    {
        auto floatReadOnlyPropAssigned = FloatPropertyBuilder("FloatReadOnlyPropAssigned", 1.0)
                                         .setReadOnly(true)
                                         .build();

        auto atomicObj = PropertyObject();
        auto objProp = ObjectProperty("AtomicObject", atomicObj);

        auto dict = Dict<IInteger, IString>();
        dict.set(0, "a");
        dict.set(1, "b");
        dict.set(2, "c");
        dict.set(5, "d");
        dict.set(8, "e");
        
        auto referencedProp = IntProperty("Referenced", 10);
        referencedProp.getOnPropertyValueWrite() += ::event(this, &PropertyObjectTest::intPropertyValueEvent);

        testPropClassBuilder = PropertyObjectClassBuilder(objManager, "Test")
                               .addProperty(FunctionProperty("Function", FunctionInfo(ctObject)))
                               .addProperty(FunctionProperty("Procedure", ProcedureInfo()))
                               .addProperty(floatReadOnlyPropAssigned)
                               .addProperty(FloatProperty("FloatProperty", 1.0))
                               .addProperty(ListProperty("ListProperty", List<Int>(1, 2, 3, 4)))
                               .addProperty(objProp)
                               .addProperty(ReferenceProperty("IntProperty", EvalValue("%TwoHopReference")))
                               .addProperty(ReferenceProperty("TwoHopReference", EvalValue("%Referenced")))
                               .addProperty(SelectionProperty("SelectionProp", List<IString>("a", "b", "c"), 0))
                               .addProperty(SelectionProperty("SelectionPropNoList", nullptr, 0))
                               .addProperty(SparseSelectionProperty("SparseSelectionProp", dict, 5))
                               .addProperty(DictProperty("DictProp", dict))
                               .addProperty(ReferenceProperty("Kind", EvalValue("%Child")))
                               .addProperty(referencedProp);
        testPropClass = testPropClassBuilder.build();
        objManager = TypeManager();
        objManager.addType(testPropClass);

        // DerivedClass

        PropertyObjectClassPtr derivedClass = PropertyObjectClassBuilder(objManager, "DerivedClass")
                                              .setParentName("Test")
                                              .addProperty(IntProperty("AdditionalProp", 1))
                                              .build();
        objManager.addType(derivedClass);

        // BaseClass

        PropertyObjectClassPtr baseClass = PropertyObjectClassBuilder(objManager, "BaseClass").build();
        objManager.addType(baseClass);

        // SpecificClass

        PropertyObjectClassPtr specificClass = PropertyObjectClassBuilder(objManager, "SpecificClass")
                                               .setParentName("BaseClass")
                                               .build();
        objManager.addType(specificClass);

        const auto defaultObj1 = PropertyObject();
        defaultObj1.addProperty(StringProperty("MyString", "foo"));
        PropertyObjectClassPtr objectClass = PropertyObjectClassBuilder(objManager, "ObjectClass")
                                                    .addProperty(ObjectProperty("Child", defaultObj1))
                                                    .build();
         objManager.addType(objectClass);

        const auto defaultObj2 = PropertyObject();
        defaultObj2.addProperty(ObjectProperty("Child", defaultObj1));
        PropertyObjectClassPtr nestedObjectClass = PropertyObjectClassBuilder(objManager, "NestedObjectClass")
                                                    .addProperty(ObjectProperty("Child", PropertyObject(objManager, "ObjectClass")))
                                                    .build();
        objManager.addType(nestedObjectClass);
    }

    void TearDown() override
    {
        objManager.removeType("DerivedClass");
        objManager.removeType("SpecificClass");
        objManager.removeType("BaseClass");
        objManager.removeType("Test");

        objManager->removeType(String("Parent"));
        objManager->removeType(String("Base"));

        propValue.release();
        propName.release();
        testPropClass.release();
    }

    void intPropertyValueEvent(PropertyObjectPtr& propObj, PropertyValueEventArgsPtr& args)
    {
        PropertyPtr prop = args.getProperty();

        propValue = propObj.getPropertyValue("IntProperty");
        propName = prop.getName();
    }

public:
    void onPropertyChanged(PropertyObjectPtr& /*sender*/, PropertyValueEventArgsPtr& /*args*/)
    {
        ASSERT_FALSE(true) << "Should never be called!";
    }

    template <typename TReturn, typename TInstance, typename... TArgs>
    auto event(TReturn (TInstance::*func)(TArgs...))
    {
        return delegate<TReturn(TArgs...)>(static_cast<TInstance* const>(this), func);
    }

    template <typename TReturn, typename TInstance, typename... TArgs>
    auto event(TReturn (TInstance::*func)(TArgs...) const)
    {
        return delegate<TReturn(TArgs...)>(static_cast<TInstance* const>(this), func);
    }
};

static void eventHandlerThrows(PropertyObjectPtr& /*sender*/, PropertyValueEventArgsPtr& /*args*/)
{
    ASSERT_FALSE(true) << "Should never be called!";
}

static void freeF(PropertyObjectPtr& prop, PropertyValueEventArgsPtr& args)
{
    IntPtr value = prop.getPropertyValue("callCount");
    if (!value.assigned())
    {
        value = 0;
    }

    // mute to prevent recursive call
    prop.getOnPropertyValueWrite(args.getProperty().getName()) |= freeF;
    prop.setPropertyValue("callCount", value + 1);
    prop.getOnPropertyValueWrite(args.getProperty().getName()) &= freeF;
}

static void freeF2(PropertyObjectPtr& /*prop*/, PropertyValueEventArgsPtr& /*args*/)
{
    // ignore
}

TEST_F(PropertyObjectTest, Create)
{
    auto propObj = PropertyObject(objManager, "Test");
}

TEST_F(PropertyObjectTest, EmptyClass)
{
    auto propObj = PropertyObject();
    ASSERT_EQ(propObj.getClassName(), "");

    propObj = PropertyObject(nullptr, nullptr);
    ASSERT_EQ(propObj.getClassName(), "");

    propObj = PropertyObject(nullptr, "");
    ASSERT_EQ(propObj.getClassName(), "");

    propObj = PropertyObject(objManager, nullptr);
    ASSERT_EQ(propObj.getClassName(), "");

    propObj = PropertyObject(objManager, "");
    ASSERT_EQ(propObj.getClassName(), "");
}

TEST_F(PropertyObjectTest, ClassName)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_EQ(propObj.getClassName(), "Test");
}

TEST_F(PropertyObjectTest, SimpleProperty)
{
    auto propObj = PropertyObject(objManager, "Test");

    auto info = PropertyBuilder("Name")
                .setValueType(ctString)
                .setDefaultValue("")
                .build();
    propObj.addProperty(info);

    propObj.setPropertyValue("Name", "Unknown");
    ASSERT_EQ(propObj.getPropertyValue("Name"), "Unknown");
}

TEST_F(PropertyObjectTest, Ownership)
{
    auto parent = PropertyObject(objManager, "Test");
    auto child = PropertyObject(objManager, "Test");

    const auto childProp = ObjectProperty("Child", child);
    parent.addProperty(childProp);

    parent.dispose();
}

TEST_F(PropertyObjectTest, GetValueType)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_EQ(propObj.getProperty("FloatProperty").getValueType(), ctFloat);
}

TEST_F(PropertyObjectTest, GetValueTypeReferenceProperty)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_EQ(propObj.getProperty("IntProperty").getValueType(), ctInt);
}

TEST_F(PropertyObjectTest, SerializeJsonSimple)
{
    const std::string expectedJson = R"({"__type":"PropertyObject","className":"Test","propValues":{"AtomicObject":{"__type":"PropertyObject"},"Referenced":12}})";

    PropertyObjectPtr propObj = PropertyObject(objManager, "Test");
    propObj.setPropertyValue("IntProperty", "12");
    propObj.setPropertyValue("Function", Function([](IBaseObject*, IBaseObject**)
    {
        return OPENDAQ_SUCCESS;
    }));

    propObj.setPropertyValue("Procedure", Procedure([]()
    {
        // nothing
    }));

    auto serializer = JsonSerializer();
    propObj.serialize(serializer);

    std::string json = serializer.getOutput().toStdString();

    ASSERT_EQ(json, expectedJson);
}

TEST_F(PropertyObjectTest, SerializeJsonSimpleWithLocalProperty)
{
    const std::string expectedJson = R"({"__type":"PropertyObject","className":"Test","propValues":{"AtomicObject":{"__type":"PropertyObject"},"Referenced":12},"properties":[{"__type":"Property","name":"LocalProp","valueType":3,"defaultValue":"-","readOnly":false,"visible":true}]})";

    PropertyObjectPtr propObj = PropertyObject(objManager, "Test");
    propObj.addProperty(StringPropertyBuilder("LocalProp", "-").build());

    propObj.setPropertyValue("IntProperty", "12");
    propObj.setPropertyValue("Function", Function([](IBaseObject*, IBaseObject**) { return OPENDAQ_SUCCESS; }));

    propObj.setPropertyValue("Procedure",
                             Procedure(
                                 []()
                                 {
                                     // nothing
                                 }));

    auto serializer = JsonSerializer();
    propObj.serialize(serializer);

    std::string json = serializer.getOutput().toStdString();

    ASSERT_EQ(json, expectedJson);
}

TEST_F(PropertyObjectTest, DeserializeJsonSimple)
{
    const std::string json = R"({"__type":"PropertyObject","className":"Test","propValues":{"AtomicObject":{"__type":"PropertyObject"},"Referenced":12}})";

    auto deserializer = JsonDeserializer();

    PropertyObjectPtr ptr = deserializer.deserialize(String(json.data()), objManager);

    auto serializer = JsonSerializer();
    ptr.serialize(serializer);

    std::string deserializedJson = serializer.getOutput().toStdString();

    ASSERT_EQ(json, deserializedJson);
}

TEST_F(PropertyObjectTest, DeserializeJsonSimpleWithLocalProperty)
{
    const std::string json =
        R"({"__type":"PropertyObject","className":"Test","propValues":{"AtomicObject":{"__type":"PropertyObject"},"Referenced":12},"properties":[{"__type":"Property","name":"LocalProp","valueType":3,"defaultValue":"-","readOnly":false,"visible":true}]})";

    const auto deserializer = JsonDeserializer();

    const PropertyObjectPtr ptr = deserializer.deserialize(String(json.data()), objManager);

    const auto serializer = JsonSerializer();
    ptr.serialize(serializer);

    const std::string deserializedJson = serializer.getOutput().toStdString();

    ASSERT_EQ(json, deserializedJson);
}

TEST_F(PropertyObjectTest, SetNullPropertyPtr)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_THROW(propObj.setPropertyValue(nullptr, nullptr), ArgumentNullException);
}

TEST_F(PropertyObjectTest, GetNullProperty)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_THROW(propObj.getPropertyValue(nullptr), ArgumentNullException);
}

TEST_F(PropertyObjectTest, ClearNullProperty)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_THROW(propObj.clearPropertyValue(nullptr), ArgumentNullException);
}

TEST_F(PropertyObjectTest, ClearPropertyValue)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_NO_THROW(propObj.clearPropertyValue("Function"));
}

TEST_F(PropertyObjectTest, ClearNonExistentProperty)
{
    auto propObj = PropertyObject(objManager, "Test");

    ASSERT_THROW(propObj.clearPropertyValue("doesNotExist"), NotFoundException);
}

TEST_F(PropertyObjectTest, SetSameValueFloat)
{
    auto propObj = PropertyObject(objManager, "Test");
    int callCount = 0;
    propObj.getOnPropertyValueWrite("FloatProperty") += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { callCount++; };
    propObj.setPropertyValue("FloatProperty", 1.0);
    propObj.setPropertyValue("FloatProperty", 2.0);
    propObj.setPropertyValue("FloatProperty", 2.0);

    ASSERT_EQ(callCount, 1);
}

TEST_F(PropertyObjectTest, SetSameValueReferenced)
{
    auto propObj = PropertyObject(objManager, "Test");
    int callCount = 0;
    propObj.getOnPropertyValueWrite("Referenced") += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { callCount++; };
    propObj.setPropertyValue("IntProperty", 10);
    propObj.setPropertyValue("IntProperty", 12);
    propObj.setPropertyValue("TwoHopReference", 12);
    propObj.setPropertyValue("TwoHopReference", 10);
    propObj.setPropertyValue("IntProperty", 10);
    propObj.setPropertyValue("IntProperty", 12);

    ASSERT_EQ(callCount, 3);
}

TEST_F(PropertyObjectTest, SetSameValueBeginEndUpdate)
{
    auto propObj = PropertyObject(objManager, "Test");
    int callCount = 0;
    
    propObj.getOnPropertyValueWrite("Referenced") += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { callCount++; };
    propObj.getOnPropertyValueWrite("FloatProperty") += [&](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { callCount++; };

    propObj.beginUpdate();

    propObj.setPropertyValue("IntProperty", 10);
    propObj.setPropertyValue("FloatProperty", 1.0);

    propObj.endUpdate();

    ASSERT_EQ(callCount, 0);
    
    propObj.beginUpdate();

    propObj.setPropertyValue("IntProperty", 12);
    propObj.setPropertyValue("FloatProperty", 2.0);

    propObj.endUpdate();

    ASSERT_EQ(callCount, 2);
}

TEST_F(PropertyObjectTest, GetNonExistentPropertyObject)
{
    auto propObj = PropertyObject(objManager, String("Test"));
    ASSERT_THROW(propObj.getProperty("doesNotExist"), NotFoundException);
}

TEST_F(PropertyObjectTest, GetPropertyObjectParamNull)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_THROW(propObj.getProperty(nullptr), ArgumentNullException);
}

TEST_F(PropertyObjectTest, EnumVisiblePropertyListNull)
{
    auto propObj = PropertyObject(objManager, "Test");
    ErrCode errCode = propObj->getVisibleProperties(nullptr);

    ASSERT_EQ(errCode, OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(PropertyObjectTest, EnumVisiblePropertyWhenClassNull)
{
    auto propObj = PropertyObject();
    ASSERT_EQ(propObj.getVisibleProperties().getCount(), 0u);
}

TEST_F(PropertyObjectTest, GetPropertyObjectWhenClassNull)
{
    auto propObj = PropertyObject();

    ASSERT_THROW(propObj.getProperty("test"), NotFoundException);
}

TEST_F(PropertyObjectTest, SelectionPropertiesInsertionOrder)
{
    auto propObj = PropertyObject(objManager, "DerivedClass");

    const auto childProp = ObjectProperty("Child", PropertyObject());
    propObj.addProperty(childProp);

    auto props = propObj.getAllProperties();
    ASSERT_EQ(props.getCount(), NumAllProperties + 2);

    size_t order = 0;
    ASSERT_EQ(props[order++].getName(), "Function");
    ASSERT_EQ(props[order++].getName(), "Procedure");
    ASSERT_EQ(props[order++].getName(), "FloatReadOnlyPropAssigned");
    ASSERT_EQ(props[order++].getName(), "FloatProperty");
    ASSERT_EQ(props[order++].getName(), "ListProperty");
    ASSERT_EQ(props[order++].getName(), "AtomicObject");
    ASSERT_EQ(props[order++].getName(), "IntProperty");
    ASSERT_EQ(props[order++].getName(), "TwoHopReference");
    ASSERT_EQ(props[order++].getName(), "SelectionProp");
    ASSERT_EQ(props[order++].getName(), "SelectionPropNoList");
    ASSERT_EQ(props[order++].getName(), "SparseSelectionProp");
    ASSERT_EQ(props[order++].getName(), "DictProp");
    ASSERT_EQ(props[order++].getName(), "Kind");
    // Derived class properties after base
    ASSERT_EQ(props[order++].getName(), "Referenced");
    ASSERT_EQ(props[order++].getName(), "AdditionalProp");
}

TEST_F(PropertyObjectTest, SelectionPropertiesCustomOrder)
{
    auto propObj = PropertyObject(objManager, "DerivedClass");
    propObj.setPropertyOrder(List<IString>("Kind", "AdditionalProp"));

    const auto childProp = ObjectProperty("Child", PropertyObject());
    propObj.addProperty(childProp);

    auto props = propObj.getAllProperties();
    ASSERT_EQ(props.getCount(), NumAllProperties + 2);

    size_t order = 0;
    ASSERT_EQ(props[order++].getName(), "Kind");
    ASSERT_EQ(props[order++].getName(), "AdditionalProp");
    // Explicit order properties before others
    ASSERT_EQ(props[order++].getName(), "Function");
    ASSERT_EQ(props[order++].getName(), "Procedure");
    ASSERT_EQ(props[order++].getName(), "FloatReadOnlyPropAssigned");
    ASSERT_EQ(props[order++].getName(), "FloatProperty");
    ASSERT_EQ(props[order++].getName(), "ListProperty");
    ASSERT_EQ(props[order++].getName(), "AtomicObject");
    ASSERT_EQ(props[order++].getName(), "IntProperty");
    ASSERT_EQ(props[order++].getName(), "TwoHopReference");
    ASSERT_EQ(props[order++].getName(), "SelectionProp");
    ASSERT_EQ(props[order++].getName(), "SelectionPropNoList");
    ASSERT_EQ(props[order++].getName(), "SparseSelectionProp");
    ASSERT_EQ(props[order++].getName(), "DictProp");
}

// TODO: Enable check once supported on OpcUa
TEST_F(PropertyObjectTest, DISABLED_SelectionPropertyInvalidItemType)
{
    auto propObj = PropertyObject();
    propObj.addProperty(SelectionProperty("selection", List<Float>(1.2, 2.3), 0));

    ASSERT_THROW(propObj.getPropertySelectionValue("selection"), InvalidTypeException);
}

TEST_F(PropertyObjectTest, SparseSelectionPropertyGet)
{
    auto propObj = PropertyObject(objManager, "Test");
    DictPtr<Int, IString> dict = propObj.getProperty("SparseSelectionProp").getSelectionValues();

    ASSERT_EQ(propObj.getPropertySelectionValue("SparseSelectionProp"), dict[5]);
}

TEST_F(PropertyObjectTest, SparseSelectionPropertySet)
{
    auto propObj = PropertyObject(objManager, "Test");
    DictPtr<Int, IString> dict = propObj.getProperty("SparseSelectionProp").getSelectionValues();

    propObj.setPropertyValue("SparseSelectionProp", 8);
    ASSERT_EQ(propObj.getPropertySelectionValue("SparseSelectionProp"), dict[8]);
}

TEST_F(PropertyObjectTest, SparseSelectionPropertySetInvalid)
{
    auto propObj = PropertyObject(objManager, "Test");
    DictPtr<Int, IString> dict = propObj.getProperty("SparseSelectionProp").getSelectionValues();

    ASSERT_ANY_THROW(propObj.setPropertyValue("SparseSelectionProp", 10));
}

TEST_F(PropertyObjectTest, EnumVisibleWithVisibleThroughRefs)
{
    auto propObj = PropertyObject(objManager, "Test");

    const auto childProp = ObjectProperty("Child", PropertyObject());
    propObj.addProperty(childProp);

    ASSERT_EQ(propObj.getAllProperties().getCount(), NumAllProperties + 1);
    ASSERT_EQ(propObj.getVisibleProperties().getCount(), NumVisibleProperties);
}

TEST_F(PropertyObjectTest, TrySetReadOnlyProperty)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_THROW(propObj.setPropertyValue("FloatReadOnlyPropAssigned", 3.33), AccessDeniedException);
}

TEST_F(PropertyObjectTest, SetProtectedReadOnlyProperty)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto protectedPropObj = propObj.asPtrOrNull<IPropertyObjectProtected>(true);

    ASSERT_NO_THROW(protectedPropObj.setProtectedPropertyValue("FloatReadOnlyPropAssigned", 3.33));
    ASSERT_EQ(propObj.getPropertyValue("FloatReadOnlyPropAssigned"), 3.33);
}

TEST_F(PropertyObjectTest, ReadOnlyPropertyAssigned)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_EQ(propObj.getPropertyValue("FloatReadOnlyPropAssigned"), 1);
}

TEST_F(PropertyObjectTest, TryClearReadOnlyProperty)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_THROW(propObj.clearPropertyValue("FloatReadOnlyPropAssigned"), AccessDeniedException);
}

TEST_F(PropertyObjectTest, ClearProtectedReadOnlyProperty)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto protectedPropObj = propObj.asPtrOrNull<IPropertyObjectProtected>(true);

    protectedPropObj.setProtectedPropertyValue("FloatReadOnlyPropAssigned", 3.33);
    ASSERT_NO_THROW(protectedPropObj.clearProtectedPropertyValue("FloatReadOnlyPropAssigned"));
    ASSERT_EQ(propObj.getPropertyValue("FloatReadOnlyPropAssigned"), 1.0);
}

TEST_F(PropertyObjectTest, ValueNotFound)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_THROW(propObj.getPropertyValue("SomeNoneExistingProperty"), NotFoundException);
}

TEST_F(PropertyObjectTest, ConvertToPropertyCoreType)
{
    auto propObj = PropertyObject(objManager, "Test");
    propObj.setPropertyValue("FloatProperty", "1");
    ASSERT_EQ(propObj.getPropertyValue("FloatProperty").getCoreType(), ctFloat);
}

TEST_F(PropertyObjectTest, ConvertToPropertyCoreTypeFails)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_THROW_MSG(propObj.setPropertyValue("FloatProperty", "a"),
                     ConversionFailedException,
                     "Value type is different than Property type and conversion failed")
}

TEST_F(PropertyObjectTest, ConvertToPropertyCoreTypeFails2)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto list = List<IBaseObject>();
    ASSERT_THROW_MSG(propObj.setPropertyValue("FloatProperty", list),
                     NoInterfaceException,
                     "Value type is different than Property type and conversion failed")
}

TEST_F(PropertyObjectTest, SetNullPropertyValue)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_THROW(propObj.setPropertyValue("FloatProperty", nullptr), ArgumentNullException);
}

TEST_F(PropertyObjectTest, SelectionProp)
{
    auto propObj = PropertyObject(objManager, "Test");
    propObj.setPropertyValue("SelectionProp", 1);

    ASSERT_EQ(propObj.getPropertyValue("SelectionProp"), 1);
    ASSERT_EQ(propObj.getPropertySelectionValue("SelectionProp"), "b");
}

TEST_F(PropertyObjectTest, SelectionPropNoEnum)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_THROW_MSG(propObj.getPropertySelectionValue("IntProperty"),
                     InvalidPropertyException,
                     "Selection property \"IntProperty\" has no selection values assigned")
}

TEST_F(PropertyObjectTest, SelectionPropNotRegistered)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_THROW_MSG(propObj.getPropertySelectionValue("TestProp"),
                     NotFoundException, "Selection property \"TestProp\" not found")
}

TEST_F(PropertyObjectTest, SelectionPropNoList)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_THROW_MSG(propObj.getPropertySelectionValue("SelectionPropNoList"),
                     InvalidPropertyException,
                     "Selection property \"SelectionPropNoList\" has no selection values assigned")
}

TEST_F(PropertyObjectTest, DictProp)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_NO_THROW(propObj.getPropertyValue("DictProp"));

    DictPtr<IInteger, IString> val = propObj.getPropertyValue("DictProp");
    ASSERT_EQ(val[0], "a");

    auto dict = Dict<IInteger, IString>();
    dict.set(0, "g");
    propObj.setPropertyValue("DictProp", dict);
    val = propObj.getPropertyValue("DictProp");
    ASSERT_EQ(val[0], "g");
}

// TODO: Enable check once supported on OpcUa
TEST_F(PropertyObjectTest, DISABLED_DictPropInvalidKeyType)
{
    auto propObj = PropertyObject(objManager, "Test");

    auto dict = Dict<IString, IString>();
    dict.set("a", "a");
    ASSERT_THROW(propObj.setPropertyValue("DictProp", dict), InvalidTypeException);
}

// TODO: Enable check once supported on OpcUa
TEST_F(PropertyObjectTest, DISABLED_DictPropInvalidItemType)
{
    auto propObj = PropertyObject(objManager, "Test");

    auto dict = Dict<IInteger, IInteger>();
    dict.set(1, 2);
    ASSERT_THROW(propObj.setPropertyValue("DictProp", dict), InvalidTypeException);
}

TEST_F(PropertyObjectTest, SquareBracketOperator)
{
    /*  auto propObj = PropertyObject(objManager, "Test");
        propObj.setPropertyValue("Value", 1);
        int a = propObj["Value"];
        1 == propObj["Value"];*/

    //  propObj["Values1"] = 2;
    //  ASSERT_EQ(propObj["Value"], 1);
}

TEST_F(PropertyObjectTest, ChildPropSet)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto childObj = PropertyObject(objManager, "Test");

    const auto childProp = ObjectProperty("Child", childObj);
    propObj.addProperty(childProp);

    propObj.setPropertyValue("Child.IntProperty", 1);
    const PropertyObjectPtr childObjCloned = propObj.getPropertyValue("Child");

    ASSERT_EQ(childObjCloned.getPropertyValue("IntProperty"), 1);
}

TEST_F(PropertyObjectTest, NestedChildPropSet)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto defaultObj1 = PropertyObject(objManager, "Test");
    auto defaultObj2 = PropertyObject(objManager, "Test");
    auto defaultObj3 = PropertyObject(objManager, "Test");

    const auto childProp3 = ObjectProperty("Child", defaultObj3);
    defaultObj2.addProperty(childProp3);
    
    const auto childProp2 = ObjectProperty("Child", defaultObj2);
    defaultObj1.addProperty(childProp2);

    const auto childProp1 = ObjectProperty("Child", defaultObj1);
    propObj.addProperty(childProp1);

    propObj.setPropertyValue("Child.IntProperty", 1);
    propObj.setPropertyValue("Child.Child.IntProperty", 2);
    propObj.setPropertyValue("Child.Child.Child.IntProperty", 3);

    const PropertyObjectPtr childObj1 = propObj.getPropertyValue("Child");
    const PropertyObjectPtr childObj2 = childObj1.getPropertyValue("Child");
    const PropertyObjectPtr childObj3 = childObj2.getPropertyValue("Child");

    ASSERT_EQ(childObj1.getPropertyValue("IntProperty"), 1);
    ASSERT_EQ(childObj2.getPropertyValue("IntProperty"), 2);
    ASSERT_EQ(childObj3.getPropertyValue("IntProperty"), 3);
}

TEST_F(PropertyObjectTest, ChildPropGet)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto childObj = PropertyObject(objManager, "Test");

    childObj.setPropertyValue("IntProperty", 2);
    const auto childProp = ObjectProperty("Child", childObj);

    propObj.addProperty(childProp);
    propObj.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("Child", childObj);

    ASSERT_EQ(propObj.getPropertyValue("Child.IntProperty"), 2);
}

TEST_F(PropertyObjectTest, ChildPropClear)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto defaultObj = PropertyObject(objManager, "Test");

    defaultObj.setPropertyValue("IntProperty", 2);
    const auto childProp = ObjectProperty("Child", defaultObj);
    propObj.addProperty(childProp);
    
    const PropertyObjectPtr childObj = propObj.getPropertyValue("Child");
    ASSERT_NO_THROW(propObj.clearPropertyValue("Child.IntProperty"));
    ASSERT_EQ(childObj.getPropertyValue("IntProperty"), 10);
}

TEST_F(PropertyObjectTest, NestedChildPropGet)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto childObj1 = PropertyObject(objManager, "Test");
    auto childObj2 = PropertyObject(objManager, "Test");
    auto childObj3 = PropertyObject(objManager, "Test");

    childObj1.setPropertyValue("IntProperty", 1);
    childObj2.setPropertyValue("IntProperty", 2);
    childObj3.setPropertyValue("IntProperty", 3);

    const auto childProp3 = ObjectProperty("Child", childObj3);
    childObj2.addProperty(childProp3);

    const auto childProp2 = ObjectProperty("Child", childObj2);
    childObj1.addProperty(childProp2);

    const auto childProp1 = ObjectProperty("Child", childObj1);
    propObj.addProperty(childProp1);

    ASSERT_EQ(propObj.getPropertyValue("Child.IntProperty"), 1);
    ASSERT_EQ(propObj.getPropertyValue("Child.Child.IntProperty"), 2);
    ASSERT_EQ(propObj.getPropertyValue("Child.Child.Child.IntProperty"), 3);
}

TEST_F(PropertyObjectTest, NestedChildPropClear)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto defaultObj1 = PropertyObject(objManager, "Test");
    auto defaultObj2 = PropertyObject(objManager, "Test");
    auto defaultObj3 = PropertyObject(objManager, "Test");

    defaultObj1.setPropertyValue("IntProperty", 1);
    defaultObj2.setPropertyValue("IntProperty", 2);
    defaultObj3.setPropertyValue("IntProperty", 3);

    const auto childProp3 = ObjectProperty("Child", defaultObj3);
    defaultObj2.addProperty(childProp3);

    const auto childProp2 = ObjectProperty("Child", defaultObj2);
    defaultObj1.addProperty(childProp2);

    const auto childProp1 = ObjectProperty("Child", defaultObj1);
    propObj.addProperty(childProp1);

    ASSERT_NO_THROW(propObj.clearPropertyValue("Child.IntProperty"));
    ASSERT_NO_THROW(propObj.clearPropertyValue("Child.Child.IntProperty"));
    ASSERT_NO_THROW(propObj.clearPropertyValue("Child.Child.Child.IntProperty"));
    
    const PropertyObjectPtr childObj1 = propObj.getPropertyValue("Child");
    const PropertyObjectPtr childObj2 = childObj1.getPropertyValue("Child");
    const PropertyObjectPtr childObj3 = childObj2.getPropertyValue("Child");

    ASSERT_EQ(childObj1.getPropertyValue("IntProperty"), 10);
    ASSERT_EQ(childObj2.getPropertyValue("IntProperty"), 10);
    ASSERT_EQ(childObj3.getPropertyValue("IntProperty"), 10);
}

TEST_F(PropertyObjectTest, ChildPropSetViaRefProp)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto defaultObj = PropertyObject(objManager, "Test");

    const auto childProp = ObjectProperty("Child", defaultObj);
    propObj.addProperty(childProp);
    propObj.setPropertyValue("Kind.IntProperty", 1);
    
    const PropertyObjectPtr childObj = propObj.getPropertyValue("Child");
    ASSERT_EQ(childObj.getPropertyValue("IntProperty"), 1);
}

TEST_F(PropertyObjectTest, NestedChildPropSetViaRefProp)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto defaultObj1 = PropertyObject(objManager, "Test");
    auto defaultObj2 = PropertyObject(objManager, "Test");
    auto defaultObj3 = PropertyObject(objManager, "Test");
    
    const auto childProp3 = ObjectProperty("Child", defaultObj3);
    defaultObj2.addProperty(childProp3);

    const auto childProp2 = ObjectProperty("Child", defaultObj2);
    defaultObj1.addProperty(childProp2);

    const auto childProp1 = ObjectProperty("Child", defaultObj1);
    propObj.addProperty(childProp1);

    propObj.setPropertyValue("Kind.IntProperty", 1);
    propObj.setPropertyValue("Kind.Kind.IntProperty", 2);
    propObj.setPropertyValue("Kind.Kind.Kind.IntProperty", 3);
    
    const PropertyObjectPtr childObj1 = propObj.getPropertyValue("Child");
    const PropertyObjectPtr childObj2 = childObj1.getPropertyValue("Child");
    const PropertyObjectPtr childObj3 = childObj2.getPropertyValue("Child");

    ASSERT_EQ(childObj1.getPropertyValue("IntProperty"), 1);
    ASSERT_EQ(childObj2.getPropertyValue("IntProperty"), 2);
    ASSERT_EQ(childObj3.getPropertyValue("IntProperty"), 3);
}

TEST_F(PropertyObjectTest, ChildPropGetViaRefProp)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto defaultObj = PropertyObject(objManager, "Test");
    
    defaultObj.setPropertyValue("IntProperty", 2);
    const auto childProp = ObjectProperty("Child", defaultObj);
    propObj.addProperty(childProp);
    
    const PropertyObjectPtr childObj = propObj.getPropertyValue("Child");
    ASSERT_EQ(propObj.getPropertyValue("Kind.IntProperty"), 2);
}

TEST_F(PropertyObjectTest, ChildPropClearViaRefProp)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto defaultObj = PropertyObject(objManager, "Test");
    
    defaultObj.setPropertyValue("IntProperty", 1);
    const auto childProp = ObjectProperty("Child", defaultObj);
    propObj.addProperty(childProp);

    const PropertyObjectPtr childObj = propObj.getPropertyValue("Child");
    ASSERT_NO_THROW(propObj.clearPropertyValue("Kind.IntProperty"));
    ASSERT_EQ(childObj.getPropertyValue("IntProperty"), 10);
}

TEST_F(PropertyObjectTest, NestedChildPropGetViaRefProp)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto defaultObj1 = PropertyObject(objManager, "Test");
    auto defaultObj2 = PropertyObject(objManager, "Test");
    auto defaultObj3 = PropertyObject(objManager, "Test");
    
    defaultObj1.setPropertyValue("IntProperty", 1);
    defaultObj2.setPropertyValue("IntProperty", 2);
    defaultObj3.setPropertyValue("IntProperty", 3);
    
    const auto childProp3 = ObjectProperty("Child", defaultObj3);
    defaultObj2.addProperty(childProp3);

    const auto childProp2 = ObjectProperty("Child", defaultObj2);
    defaultObj1.addProperty(childProp2);

    const auto childProp1 = ObjectProperty("Child", defaultObj1);
    propObj.addProperty(childProp1);

    const PropertyObjectPtr childObj1 = propObj.getPropertyValue("Child");
    const PropertyObjectPtr childObj2 = childObj1.getPropertyValue("Child");
    const PropertyObjectPtr childObj3 = childObj2.getPropertyValue("Child");
    
    ASSERT_EQ(propObj.getPropertyValue("Kind.IntProperty"), 1);
    ASSERT_EQ(propObj.getPropertyValue("Kind.Kind.IntProperty"), 2);
    ASSERT_EQ(propObj.getPropertyValue("Kind.Kind.Kind.IntProperty"), 3);
}

TEST_F(PropertyObjectTest, NestedChildPropClearViaRefProp)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto defaultObj1 = PropertyObject(objManager, "Test");
    auto defaultObj2 = PropertyObject(objManager, "Test");
    auto defaultObj3 = PropertyObject(objManager, "Test");
    

    defaultObj1.setPropertyValue("IntProperty", 1);
    defaultObj2.setPropertyValue("IntProperty", 2);
    defaultObj3.setPropertyValue("IntProperty", 3);

    const auto childProp3 = ObjectProperty("Child", defaultObj3);
    defaultObj2.addProperty(childProp3);

    const auto childProp2 = ObjectProperty("Child", defaultObj2);
    defaultObj1.addProperty(childProp2);

    const auto childProp1 = ObjectProperty("Child", defaultObj1);
    propObj.addProperty(childProp1);

    ASSERT_NO_THROW(propObj.clearPropertyValue("Kind.IntProperty"));
    ASSERT_NO_THROW(propObj.clearPropertyValue("Kind.Kind.IntProperty"));
    ASSERT_NO_THROW(propObj.clearPropertyValue("Kind.Kind.Kind.IntProperty"));
    
    const PropertyObjectPtr childObj1 = propObj.getPropertyValue("Child");
    const PropertyObjectPtr childObj2 = childObj1.getPropertyValue("Child");
    const PropertyObjectPtr childObj3 = childObj2.getPropertyValue("Child");
    ASSERT_EQ(childObj1.getPropertyValue("IntProperty"), 10);
    ASSERT_EQ(childObj2.getPropertyValue("IntProperty"), 10);
    ASSERT_EQ(childObj3.getPropertyValue("IntProperty"), 10);
}

TEST_F(PropertyObjectTest, OnValueChange)
{
    auto propObj = PropertyObject(objManager, "Test");
    BaseObjectPtr propValue;
    StringPtr propName;

    propObj.getOnPropertyValueWrite("Referenced") += [&propValue, &propName](PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args)
    {
        auto prop = args.getProperty();

        propValue = sender.getPropertyValue("IntProperty");
        propName = prop.getName();
    };

    propObj.setPropertyValue("IntProperty", 2);
    ASSERT_EQ(propValue, 2);
    ASSERT_EQ(propName, "Referenced");
}

TEST_F(PropertyObjectTest, OnValueChangeClear)
{
    auto propObj = PropertyObject(objManager, "Test");
    Int numCallbacks = 0;

    propObj.getOnPropertyValueWrite("Referenced") +=
        [&numCallbacks](PropertyObjectPtr& /*sender*/, PropertyValueEventArgsPtr& /*args*/)
    {
        numCallbacks++;
    };

    propObj.setPropertyValue("IntProperty", 2);
    propObj.clearPropertyValue("IntProperty");
    ASSERT_EQ(numCallbacks, 2);
}

TEST_F(PropertyObjectTest, OnValueChangePropertyEvent)
{
    auto propObj = PropertyObject(objManager, "Test");

    propObj.setPropertyValue("Referenced", 2);
    ASSERT_EQ(propValue, 2);
    ASSERT_EQ(propName, "Referenced");
}

TEST_F(PropertyObjectTest, InheritedParent)
{
    auto newTestPropClass = PropertyObjectClassBuilder(objManager, "NewTest")
                            .setParentName("Test")
                            .addProperty(FloatProperty("NewFloatProperty", 1.2))
                            .build();
    objManager.addType(newTestPropClass);

    auto propObj = PropertyObject(objManager, "Test");

    const auto childProp = ObjectProperty("Child", PropertyObject());
    propObj.addProperty(childProp);

    auto props = propObj.getVisibleProperties();
    ASSERT_EQ(NumVisibleProperties, props.getCount());

    propObj.release();
    objManager.removeType("NewTest");
}

TEST_F(PropertyObjectTest, LocalProperties)
{
    auto propObj = PropertyObject(objManager, "Test");

    const auto childProp = ObjectProperty("Child", PropertyObject());
    propObj.addProperty(childProp);

    ASSERT_EQ(propObj.getVisibleProperties().getCount(), NumVisibleProperties);

    auto localProp = PropertyBuilder("LocalProp")
                     .setValueType(ctInt)
                     .setDefaultValue(1)
                     .build();
    propObj.addProperty(localProp);

    ASSERT_EQ(propObj.getVisibleProperties().getCount(), NumVisibleProperties + 1);
    auto value = propObj.getPropertyValue("LocalProp");
    ASSERT_EQ(value, 1);
}

TEST_F(PropertyObjectTest, LocalSelectionProperty)
{
    auto propObj = PropertyObject(objManager, "Test");

    auto localSelectionProp = PropertyBuilder("LocalSelectionProp")
                              .setValueType(ctInt)
                              .setSelectionValues(List<IString>("one", "two", "three"))
                              .setDefaultValue(0)
                              .build();
    propObj.addProperty(localSelectionProp);

    auto value = propObj.getPropertyValue("LocalSelectionProp");
    ASSERT_EQ(value, 0);
    auto valueAsText = propObj.getPropertySelectionValue("LocalSelectionProp");
    ASSERT_EQ(valueAsText, "one");

    propObj.setPropertyValue("LocalSelectionProp", 1);
    value = propObj.getPropertyValue("LocalSelectionProp");
    ASSERT_EQ(value, 1);
    valueAsText = propObj.getPropertySelectionValue("LocalSelectionProp");
    ASSERT_EQ(valueAsText, "two");
}

TEST_F(PropertyObjectTest, NotLocalProperty)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto classProp = propObj.getProperty("FloatProperty");
}

TEST_F(PropertyObjectTest, EventSubscriptionCount)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_EQ(propObj.getOnPropertyValueWrite("FloatProperty").getListenerCount(), 0u);
}

TEST_F(PropertyObjectTest, EventSubscriptionMuteFreeFunction)
{
    auto propObj = PropertyObject(objManager, "Test");
    propObj.addProperty(IntProperty("callCount", 0));

    propObj.setPropertyValue("callCount", 0);
    
    propObj.addProperty(StringProperty("testProp", "test"));

    propObj.getOnPropertyValueWrite("testProp") += [](PropertyObjectPtr& /*prop*/, PropertyValueEventArgsPtr& /*args*/)
    {
        // ignore
    };

    propObj.getOnPropertyValueWrite("testProp") += freeF2;

    propObj.getOnPropertyValueWrite("testProp") += freeF;
    propObj.getOnPropertyValueWrite("testProp") |= freeF;  // mute


    propObj.setPropertyValue("testProp", "testValue");

    Int callCount = propObj.getPropertyValue("callCount");
    ASSERT_EQ(callCount, 0);

    propObj.getOnPropertyValueWrite("testProp") &= freeF;  // unmute

    propObj.setPropertyValue("testProp", "testValue1");

    callCount = propObj.getPropertyValue("callCount");
    ASSERT_EQ(callCount, 1);
}

TEST_F(PropertyObjectTest, EventSubscriptionMute)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto onChange = propObj.getOnPropertyValueWrite("FloatProperty");

    onChange += eventHandlerThrows;
    onChange += event(&PropertyObjectTest::onPropertyChanged);

    onChange += [](PropertyObjectPtr& /*sender*/, PropertyValueEventArgsPtr& /*args*/)
    {
        ASSERT_FALSE(true) << "Should never be called!";
    };

    onChange.mute();

    ASSERT_NO_THROW(propObj.setPropertyValue("FloatProperty", 1.0));
}

TEST_F(PropertyObjectTest, PropertyListIndexerOutOfRange)
{
    auto obj = PropertyObject(objManager, "Test");
    obj.setPropertyValue("ListProperty", List<IBaseObject>());

    ASSERT_THROW(obj.getPropertyValue("ListProperty[3]"), OutOfRangeException);
}

TEST_F(PropertyObjectTest, PropertyIndexer)
{
    auto obj = PropertyObject(objManager, "Test");
    obj.setPropertyValue("ListProperty", List<Int>(1, 2, 3, 4));

    auto prop = obj.getPropertyValue("ListProperty[3]");
}

TEST_F(PropertyObjectTest, PropertyNotListIndexer)
{
    auto obj = PropertyObject(objManager, "Test");
    obj.setPropertyValue("IntProperty", 5);

    ASSERT_THROW(obj.getPropertyValue("IntProperty[3]"), InvalidParameterException);
}

TEST_F(PropertyObjectTest, ChildPropGetArray)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto childObj = PropertyObject(objManager, "Test");
    
    childObj.setPropertyValue("ListProperty", List<Int>(1, 4));
    const auto childProp = ObjectProperty("Child", childObj);
    propObj.addProperty(childProp);

    ASSERT_EQ(propObj.getPropertyValue("Child.ListProperty[1]"), 4);
}

// TODO: Enable check once supported on OpcUa
TEST_F(PropertyObjectTest, DISABLED_ListInvalidItemType)
{
    auto propObj = PropertyObject(objManager, "Test");
    ASSERT_THROW(propObj.setPropertyValue("ListProperty", List<Float>(1.2, 3.4)), InvalidTypeException);
    ASSERT_THROW(propObj.setPropertyValue("ListProperty", List<IString>("foo1", "foo2")), InvalidTypeException);
    ASSERT_THROW(propObj.setPropertyValue("ListProperty", List<Bool>(true, false)), InvalidTypeException);
}

TEST_F(PropertyObjectTest, DefaultPropertyValue)
{
    auto propObjParent = PropertyObject(objManager, "Test");
    auto propObjChild = PropertyObject(objManager, "Test");
    propObjChild.asPtr<IOwnable>().setOwner(propObjParent);

    ASSERT_EQ(propObjChild.getPropertyValue("SelectionProp"), 0);

    propObjParent = nullptr;  // delete parent

    ASSERT_EQ(propObjChild.getPropertyValue("SelectionProp"), 0);
}

TEST_F(PropertyObjectTest, HasPropertyFalse)
{
    auto propObj = PropertyObject(objManager, "Test");

    ASSERT_FALSE(propObj.hasProperty("DoesNotExist"));
}

TEST_F(PropertyObjectTest, HasPropertyPrivate)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto info = PropertyBuilder("SomeProperty")
                .setDefaultValue("foo")
                .build();
    propObj.addProperty(info);

    ASSERT_TRUE(propObj.hasProperty("SomeProperty"));
}

TEST_F(PropertyObjectTest, HasPropertyOnClass)
{
    auto propObj = PropertyObject(objManager, "Test");

    ASSERT_TRUE(propObj.hasProperty("SelectionProp"));
}

TEST_F(PropertyObjectTest, HasPropertyOnClassParent)
{
    auto parentClass = PropertyObjectClassBuilder(objManager, "Parent")
                       .addProperty(BoolProperty("IsTest", true))
                       .build();

    auto baseClass = PropertyObjectClassBuilder(objManager, "Base")
                     .setParentName("Parent")
                     .build();

    objManager.addType(parentClass);
    objManager.addType(baseClass);

    auto propObj = PropertyObject(objManager, "Base");
    ASSERT_TRUE(propObj.hasProperty("IsTest"));
}

TEST_F(PropertyObjectTest, HasPropertyLocal)
{
    auto propObj = PropertyObject(objManager, "Test");
    propObj.addProperty(BoolProperty("BoolProp", false));

    ASSERT_TRUE(propObj.hasProperty("BoolProp"));
}

TEST_F(PropertyObjectTest, ToString)
{
    auto propObj = PropertyObject(objManager, "Test");

    ASSERT_EQ(propObj.toString(), "PropertyObject {Test}");
}

TEST_F(PropertyObjectTest, ToStringWithoutPropertyClass)
{
    auto propObj = PropertyObject();

    ASSERT_EQ(propObj.toString(), "PropertyObject");
}

TEST_F(PropertyObjectTest, ValidatorTestEvalVale)
{
    ValidatorPtr validator = Validator("value > 5");
    ASSERT_NO_THROW(validator.validate(nullptr, 10));
    ASSERT_THROW(validator.validate(nullptr, 0), ValidateFailedException);
}

TEST_F(PropertyObjectTest, CoercerTestEvalValue)
{
    CoercerPtr coercer = Coercer("value + 2");
    ASSERT_EQ(coercer.coerce(nullptr, 10), 12);
}

TEST_F(PropertyObjectTest, PropertyCoerceEvalValue)
{
    std::string propertyName = "CoerceProp";
    Float value = 10.2;

    CoercerPtr coercer = Coercer("value + 2");
    auto ptr = FloatPropertyBuilder(propertyName, value)
               .setCoercer(coercer)
               .build();

    auto obj = PropertyObject(objManager, "Test");

    obj.addProperty(ptr);
    obj.setPropertyValue(propertyName, value);

    Float validatedValue = obj.getPropertyValue(propertyName);

    ASSERT_DOUBLE_EQ(value + 2.0, validatedValue);
}

TEST_F(PropertyObjectTest, PropertyCoerceFailedEvalValue)
{
    std::string propertyName = "CoerceProp";
    Float value = 10.2;
    
    auto obj = PropertyObject(objManager, "Test");
    
    CoercerPtr coercer = Coercer("if(value == foo, 5, value)");
    auto ptr = FloatPropertyBuilder(propertyName, value)
               .setCoercer(coercer)
               .build();

    obj.addProperty(ptr);
    ASSERT_THROW(obj.setPropertyValue(propertyName, value), CoerceFailedException);
}

TEST_F(PropertyObjectTest, PropertyValidateEvalValue)
{
    std::string propertyName = "ValidateProp";
    
    ValidatorPtr validator = Validator("($SelectionProp == 2) && (value == 10.2)");
    auto ptr = FloatPropertyBuilder(propertyName, 1.1)
               .setValidator(validator)
               .build();

    auto obj = PropertyObject(objManager, "Test");
    obj.addProperty(ptr);
    obj.setPropertyValue("SelectionProp", 2);

    ErrCode err = obj->setPropertyValue(String(propertyName), Floating(10.2));
    ASSERT_EQ(err, OPENDAQ_SUCCESS);
    ASSERT_NO_THROW(obj.setPropertyValue(propertyName, 10.2));
}

TEST_F(PropertyObjectTest, PropertyValidateFailedEvalValue)
{
    std::string propertyName = "ValidateProp";
    
    ValidatorPtr validator = Validator("value != 10.2");
    auto ptr = FloatPropertyBuilder(propertyName, 1.1)
               .setValidator(validator)
               .build();

    auto obj = PropertyObject(objManager, "Test");
    obj.addProperty(ptr);
    
    ErrCode err = obj->setPropertyValue(String(propertyName), Floating(10.2));
    ASSERT_EQ(err, OPENDAQ_ERR_VALIDATE_FAILED);

    ASSERT_THROW(obj.setPropertyValue(propertyName, 10.2), ValidateFailedException);
}

TEST_F(PropertyObjectTest, PropertyWriteValidateEvalValueMultipleTimes)
{
    std::string propertyName = "ValidateProp";

    ValidatorPtr validator = Validator("($SelectionProp == 2) && (value > 10)");
    auto ptr = FloatPropertyBuilder(propertyName, 1.1)
               .setValidator(validator)
               .build();

    auto obj = PropertyObject(objManager, "Test");
    obj.addProperty(ptr);
    obj.setPropertyValue("SelectionProp", 2);

    ErrCode err = obj->setPropertyValue(String(propertyName), Floating(10.2));
    ASSERT_EQ(err, OPENDAQ_SUCCESS);
    ASSERT_NO_THROW(obj.setPropertyValue(propertyName, 10.2));

    err = obj->setPropertyValue(String(propertyName), Floating(11.2));
    ASSERT_EQ(err, OPENDAQ_SUCCESS);
    ASSERT_NO_THROW(obj.setPropertyValue(propertyName, 11.2));

    err = obj->setPropertyValue(String(propertyName), Floating(12.2));
    ASSERT_EQ(err, OPENDAQ_SUCCESS);
    ASSERT_NO_THROW(obj.setPropertyValue(propertyName, 12.2));

    err = obj->setPropertyValue(String(propertyName), Floating(5.0));
    ASSERT_EQ(err, OPENDAQ_ERR_VALIDATE_FAILED);
    ASSERT_THROW(obj.setPropertyValue(propertyName, 5), ValidateFailedException);
}


TEST_F(PropertyObjectTest, ValidatorSerialize)
{
    std::string expectedJson = R"({"__type":"Validator","EvalStr":"value == 10"})";
    auto serializer = JsonSerializer();

    ValidatorPtr validator = Validator("value == 10");
    validator.serialize(serializer);
    std::string json = serializer.getOutput();
    ASSERT_EQ(json, expectedJson);
}

TEST_F(PropertyObjectTest, ValidatorDeserialize)
{
    std::string serializedJson = R"({"__type":"Validator","EvalStr":"value == 10"})";

    auto deserializer = JsonDeserializer();
    ValidatorPtr validator = deserializer.deserialize(serializedJson);

    auto serializer = JsonSerializer();
    validator.serialize(serializer);

    std::string deserialized = serializer.getOutput();

    ASSERT_EQ(serializedJson, deserialized);
}

TEST_F(PropertyObjectTest, CoercerSerialize)
{
    std::string expectedJson = R"({"__type":"Coercer","EvalStr":"value == 10"})";
    auto serializer = JsonSerializer();

    CoercerPtr coercer = Coercer("value == 10");
    coercer.serialize(serializer);
    std::string json = serializer.getOutput();
    ASSERT_EQ(json, expectedJson);
}

TEST_F(PropertyObjectTest, CoercerDeserialize)
{
    std::string serializedJson = R"({"__type":"Coercer","EvalStr":"value == 10"})";

    auto deserializer = JsonDeserializer();
    CoercerPtr coercer = deserializer.deserialize(serializedJson);

    auto serializer = JsonSerializer();
    coercer.serialize(serializer);

    std::string deserialized = serializer.getOutput();

    ASSERT_EQ(serializedJson, deserialized);
}

TEST_F(PropertyObjectTest, SetPropertyWhenFrozen)
{
    auto propObj = PropertyObject(objManager, "Test");
    propObj.freeze();

    ASSERT_THROW(propObj.setPropertyValue("test", "testing"), FrozenException);
}

TEST_F(PropertyObjectTest, ClearPropertyWhenFrozen)
{
    auto propObj = PropertyObject(objManager, "Test");
    propObj.freeze();

    ASSERT_THROW(propObj.clearPropertyValue("test"), FrozenException);
}

TEST_F(PropertyObjectTest, SetOwnerWhenFrozen)
{
    auto propObj = PropertyObject(objManager, "Test");
    propObj.freeze();

    auto ownable = propObj.asPtr<IOwnable>(true);
    ASSERT_NO_THROW(ownable.setOwner(nullptr));

    auto newOwner = PropertyObject();
    ASSERT_NO_THROW(ownable.setOwner(newOwner));
}

TEST_F(PropertyObjectTest, AutoCoerceMin)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto intPropInfo = IntPropertyBuilder("IntProperty2", 12)
                       .setMinValue(10)
                       .build();
    propObj.addProperty(intPropInfo);

    auto floatPropInfo = FloatPropertyBuilder("FloatProperty2", 12.1)
                         .setMinValue(Floating(10.0))
                         .build();
    propObj.addProperty(floatPropInfo);

    propObj.setPropertyValue("IntProperty2", 5);
    ASSERT_EQ(propObj.getPropertyValue("IntProperty2"), 10);

    propObj.setPropertyValue("FloatProperty2", 5.0);
    ASSERT_EQ(propObj.getPropertyValue("FloatProperty2"), 10.0);
}

TEST_F(PropertyObjectTest, AutoCoerceMax)
{
    auto propObj = PropertyObject(objManager, "Test");
    auto intPropInfo = IntPropertyBuilder("IntProperty2", 1)
                       .setMaxValue(10)
                       .build();
    propObj.addProperty(intPropInfo);

    auto floatPropInfo = FloatPropertyBuilder("FloatProperty2", 1.1)
                         .setMaxValue(Floating(10.0))
                         .build();
    propObj.addProperty(floatPropInfo);

    propObj.setPropertyValue("IntProperty2", 50);
    ASSERT_EQ(propObj.getPropertyValue("IntProperty2"), 10);

    propObj.setPropertyValue("FloatProperty2", 50.0);
    ASSERT_EQ(propObj.getPropertyValue("FloatProperty2"), 10.0);
}

TEST_F(PropertyObjectTest, AutoCoerceMinEvalValue)
{
    auto propObj = PropertyObject(objManager, "Test");

    auto minPropInfo = IntProperty("MinProperty", 10);
    propObj.addProperty(minPropInfo);

    auto intPropInfo = IntPropertyBuilder("IntProperty2", 12)
                       .setMinValue(EvalValue("$MinProperty"))
                       .build();
    propObj.addProperty(intPropInfo);

    auto floatPropInfo = FloatPropertyBuilder("FloatProperty2", 12.1)
                         .setMinValue(EvalValue("$MinProperty - 3"))
                         .build();

    propObj.addProperty(floatPropInfo);

    propObj.setPropertyValue("IntProperty2", 5);
    ASSERT_EQ(propObj.getPropertyValue("IntProperty2"), 10);

    propObj.setPropertyValue("FloatProperty2", 5.0);
    ASSERT_EQ(propObj.getPropertyValue("FloatProperty2"), 7.0);
}

TEST_F(PropertyObjectTest, AutoCoerceMaxEvalValue)
{
    auto propObj = PropertyObject(objManager, "Test");

    auto maxPropInfo = IntProperty("MaxProperty", 10);
    propObj.addProperty(maxPropInfo);

    auto intPropInfo = IntPropertyBuilder("IntProperty2", 1)
                       .setMaxValue(EvalValue("$MaxProperty"))
                       .build();
    propObj.addProperty(intPropInfo);

    auto floatPropInfo = FloatPropertyBuilder("FloatProperty2", 1.1)
                         .setMaxValue(EvalValue("$MaxProperty + 10"))
                         .build();
    propObj.addProperty(floatPropInfo);

    propObj.setPropertyValue("IntProperty2", 50);
    ASSERT_EQ(propObj.getPropertyValue("IntProperty2"), 10);

    propObj.setPropertyValue("FloatProperty2", 50.0);
    ASSERT_EQ(propObj.getPropertyValue("FloatProperty2"), 20.0);
}

TEST_F(PropertyObjectTest, Inspectable)
{
    auto obj = PropertyObject();

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IPropertyObject::Id);
}

TEST_F(PropertyObjectTest, Id)
{
    constexpr IntfID ID {0x356DD076, 0xE76B, 0x5A15, {{0xB5, 0xF0, 0xEC, 0xAC, 0x30, 0xEB, 0xFA, 0x12}}};
    ASSERT_EQ(ID, IPropertyObject::Id);
}

TEST_F(PropertyObjectTest, ImplementationName)
{
    auto obj = PropertyObject();

    std::string className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    auto prefix = className.find("daq::GenericPropertyObjectImpl<");
    ASSERT_EQ(prefix, 0u);
}

TEST_F(PropertyObjectTest, SumFunctionProp)
{
    auto propObj = PropertyObject();

    auto arguments = List<IArgumentInfo>(ArgumentInfo("Val1", ctInt), ArgumentInfo("Val2", ctInt));
    propObj.addProperty(FunctionProperty("SumFunction", FunctionInfo(ctInt, arguments)));

    auto func = Function([](IntegerPtr val1, IntegerPtr val2)
    {
        return val1 + val2;
    });
    propObj.setPropertyValue("SumFunction", func);

    FunctionPtr getFunc = propObj.getPropertyValue("SumFunction");

    ASSERT_EQ(getFunc(10, 20), 30);
}

TEST_F(PropertyObjectTest, ObjectPropMetadata)
{
    auto propObj = PropertyObject();
    auto childObj = PropertyObject();
    childObj.addProperty(StringProperty("Foo", "Bar"));

    auto prop = ObjectPropertyBuilder("child", childObj)
                .setReadOnly(true)
                .setVisible(false)
                .build();

    propObj.addProperty(prop);

    ASSERT_EQ(propObj.getVisibleProperties().getCount(), 0u);
    ASSERT_THROW(propObj.setPropertyValue("child", PropertyObject()), AccessDeniedException);
    ASSERT_THROW(childObj.setPropertyValue("Foo", "NotBar"), FrozenException);
}

TEST_F(PropertyObjectTest, DISABLED_SerializeAndDeserializeEmpty)
{
    auto comp = PropertyObject();

    auto jsonSerializer = JsonSerializer(true);
    comp.serialize(jsonSerializer);
    auto str = jsonSerializer.getOutput();

    auto jsonDeserializer = JsonDeserializer();
    auto obj = jsonDeserializer.deserialize(str, nullptr).asPtr<IPropertyObject>();
}

TEST_F(PropertyObjectTest, SerializeAndUpdate)
{
    auto defProp = PropertyObject();
    defProp.addProperty(StringProperty("SubStringProp", "-"));

    auto comp = PropertyObject();
    comp.addProperty(StringProperty("StringProp", "-"));
    comp.addProperty(IntProperty("IntProp", 1));
    comp.addProperty(FloatProperty("FloatProp", 1.0));
    comp.addProperty(FloatProperty("BoolProp", False));
    comp.addProperty(ListProperty("ListProp", List<IFloat>()));
    comp.addProperty(DictProperty("DictProp", Dict<IString, IFloat>()));
    comp.addProperty(IntPropertyBuilder("ReadOnlyIntProp", 1).setReadOnly(True).build());
    comp.addProperty(FunctionProperty("Procedure", ProcedureInfo()));
    comp.addProperty(ReferenceProperty("RefProp", EvalValue("%IntProp")));

    comp.setPropertyValue("StringProp", "?");
    comp.setPropertyValue("IntProp", 2);
    comp.setPropertyValue("FloatProp", 2.0);
    comp.setPropertyValue("BoolProp", True);
    comp.setPropertyValue("ListProp", List<IFloat>(2.0, 3.0, 4.0));
    comp.setPropertyValue("DictProp", Dict<IString, IFloat>({{"2", 2.0}, {"3", 3.0}, {"4", 4.0}}));
    comp.setPropertyValue("Procedure", Procedure([] {}));

/*    auto prop = PropertyObject();
    prop.addProperty(StringProperty("SubStringProp", "-"));
    prop.setPropertyValue("SubStringProp", "!");

    comp.setPropertyValue("ObjProp", prop);*/


    auto jsonSerializer = JsonSerializer(true);
    comp.serialize(jsonSerializer);
    auto str = jsonSerializer.getOutput();

    auto comp1 = PropertyObject();
    comp1.addProperty(StringProperty("StringProp", "-"));
    comp1.addProperty(IntProperty("IntProp", 1));
    comp1.addProperty(FloatProperty("FloatProp", 1.0));
    comp1.addProperty(FloatProperty("BoolProp", False));
    comp1.addProperty(ListProperty("ListProp", List<IFloat>()));
    comp1.addProperty(DictProperty("DictProp", Dict<IString, IFloat>()));
    comp1.addProperty(IntPropertyBuilder("ReadOnlyIntProp", 1).setReadOnly(True).build());
    comp1.addProperty(FunctionProperty("Procedure", ProcedureInfo()));
    comp1.addProperty(ReferenceProperty("RefProp", EvalValue("%IntProp")));
    //    comp1.addProperty(ObjectProperty("ObjProp", defProp));

    auto jsonDeserializer = JsonDeserializer();
    jsonDeserializer.update(comp1, str);

    ASSERT_EQ(comp1.getPropertyValue("StringProp"), comp.getPropertyValue("StringProp"));
    ASSERT_EQ(comp1.getPropertyValue("IntProp"), comp.getPropertyValue("IntProp"));
    ASSERT_EQ(comp1.getPropertyValue("BoolProp"), comp.getPropertyValue("BoolProp"));
    ASSERT_EQ(comp1.getPropertyValue("FloatProp"), comp.getPropertyValue("FloatProp"));
    ASSERT_EQ(comp1.getPropertyValue("ListProp"), comp.getPropertyValue("ListProp"));
    ASSERT_EQ(comp1.getPropertyValue("DictProp"), comp.getPropertyValue("DictProp"));
    ASSERT_EQ(comp1.getPropertyValue("RefProp"), comp.getPropertyValue("RefProp"));
    //    ASSERT_EQ(comp1.getPropertyValue("ObjProp"), comp.getPropertyValue("ObjProp"));
}


TEST_F(PropertyObjectTest, UpdateOrder)
{
    std::vector<std::string> readProps;
    const auto propObj = PropertyObject();
    propObj.addProperty(StringProperty("Prop1", "-"));
    propObj.addProperty(StringProperty("Prop2", "-"));

    auto onPropWrite = [&readProps](PropertyObjectPtr& /*sender*/, PropertyValueEventArgsPtr& args)
    {
        readProps.emplace_back(args.getProperty().getName());
    };

    propObj.getOnPropertyValueWrite("Prop1") += onPropWrite;
    propObj.getOnPropertyValueWrite("Prop2") += onPropWrite;

    const std::string jsonReversedPropOrderStr = R"({"__type":"PropertyObject","propValues":{"Prop2":"Value2","Prop1": "Value1"}})";

    const auto jsonDeserializer = JsonDeserializer();
    jsonDeserializer.update(propObj, jsonReversedPropOrderStr);

    ASSERT_EQ(propObj.getPropertyValue("Prop1"), "Value1");
    ASSERT_EQ(propObj.getPropertyValue("Prop2"), "Value2");

    ASSERT_THAT(readProps, testing::ElementsAre("Prop1", "Prop2"));
}

TEST_F(PropertyObjectTest, BeginEndUpdate)
{
    const auto propObj = PropertyObject();

    auto endUpdateCalled = false;

    propObj.getOnEndUpdate() += [&endUpdateCalled](PropertyObjectPtr&, EndUpdateEventArgsPtr& args)
    {
        ASSERT_THAT(args.getProperties(), testing::ElementsAre("Prop1", "Prop2", "Prop3"));
        endUpdateCalled = true;
    };

    propObj.addProperty(StringProperty("Prop1", "-"));
    auto propValueWriteCalled = false;
    propObj.getOnPropertyValueWrite("Prop1") += [&propValueWriteCalled](PropertyObjectPtr&, PropertyValueEventArgsPtr& args)
    {
        ASSERT_EQ(args.getPropertyEventType(), PropertyEventType::Update);
        ASSERT_TRUE(args.getIsUpdating());
        ASSERT_EQ(args.getValue(), "Value1");

        args.setValue("Value1_1");

        propValueWriteCalled = true;
    };

    propObj.addProperty(StringProperty("Prop2", "-"));

    propObj.addProperty(StringProperty("Prop3", "-"));
    propObj.setPropertyValue("Prop3", "Value3");

    propObj.beginUpdate();

    propObj.setPropertyValue("Prop1", "Value1");
    propObj.setPropertyValue("Prop2", "Value2");
    propObj.clearPropertyValue("Prop3");

    ASSERT_EQ(propObj.getPropertyValue("Prop1"), "-");
    ASSERT_EQ(propObj.getPropertyValue("Prop2"), "-");
    ASSERT_EQ(propObj.getPropertyValue("Prop3"), "Value3");

    ASSERT_FALSE(propValueWriteCalled);
    ASSERT_FALSE(endUpdateCalled);

    propObj.endUpdate();

    ASSERT_TRUE(propValueWriteCalled);
    ASSERT_TRUE(endUpdateCalled);

    ASSERT_EQ(propObj.getPropertyValue("Prop1"), "Value1_1");
    ASSERT_EQ(propObj.getPropertyValue("Prop2"), "Value2");
    ASSERT_EQ(propObj.getPropertyValue("Prop3"), "-");
}

TEST_F(PropertyObjectTest, TestContainerClone)
{
    const auto propObj = PropertyObject();
    propObj.addProperty(ListProperty("List", List<IString>("foo", "bar")));
    propObj.addProperty(DictProperty("Dict", Dict<IInteger, IString>({{1, "foo"}, {2, "bar"}})));

    ListPtr<IString> lst = propObj.getPropertyValue("List");
    DictPtr<Int, IString> dct = propObj.getPropertyValue("Dict");

    lst.pushBack("test");
    dct.set(3, "test");

    ASSERT_NE(propObj.getPropertyValue("List"), lst);
    ASSERT_NE(propObj.getPropertyValue("Dict"), dct);

    propObj.setPropertyValue("List", lst);
    propObj.setPropertyValue("Dict", dct);

    ASSERT_EQ(propObj.getPropertyValue("List"), lst);
    ASSERT_EQ(propObj.getPropertyValue("Dict"), dct);

    lst.clear();
    dct.clear();

    ASSERT_NE(propObj.getPropertyValue("List"), lst);
    ASSERT_NE(propObj.getPropertyValue("Dict"), dct);
}

TEST_F(PropertyObjectTest, Clone)
{
    auto propObj = PropertyObject();

    auto propObj1 = PropertyObject();
    propObj1.addProperty(StringProperty("foo", "bar"));

    auto propObj2 = PropertyObject();
    propObj2.addProperty(StringProperty("foo", "bar"));
    
    propObj1.addProperty(ObjectProperty("child", propObj2));
    propObj.addProperty(ObjectProperty("child", propObj1));

    PropertyObjectPtr clonedObj1 = propObj.getPropertyValue("child");
    PropertyObjectPtr clonedObj2 = clonedObj1.getPropertyValue("child");

    ASSERT_NO_THROW(propObj.asPtr<IPropertyObjectInternal>().clone());

    propObj.setPropertyValue("child.child.foo", "test");
    ASSERT_EQ(propObj2.getPropertyValue("foo"), "bar");
    ASSERT_EQ(clonedObj2.getPropertyValue("foo"), "test");
}

TEST_F(PropertyObjectTest, NestedObjectsFrozen)
{
    const auto propObj = PropertyObject();
    const auto propObj1 = PropertyObject();
    propObj.addProperty(ObjectProperty("Child", propObj1));
    ASSERT_TRUE(propObj1.isFrozen());
}

TEST_F(PropertyObjectTest, ClonedObjectSet)
{
    const auto propObj = PropertyObject();
    const auto propObj1 = PropertyObject();
    propObj1.addProperty(IntProperty("MyInt", 10));
    propObj.addProperty(ObjectProperty("Child", propObj1));

    const PropertyObjectPtr cloned = propObj.getPropertyValue("Child");
    propObj.setPropertyValue("Child.MyInt", 15);
    ASSERT_EQ(cloned.getPropertyValue("MyInt"), 15);
}

TEST_F(PropertyObjectTest, ClonedObjectsClear)
{
    const auto propObj = PropertyObject();
    const auto propObj1 = PropertyObject();
    propObj1.addProperty(IntProperty("MyInt", 10));
    propObj.addProperty(ObjectProperty("Child", propObj1));

    const auto cloned = propObj.getPropertyValue("Child");
    propObj.setPropertyValue("Child.MyInt", 15);
    propObj.clearPropertyValue("Child");
    ASSERT_NE(cloned, propObj.getPropertyValue("Child"));
    ASSERT_EQ(propObj.getPropertyValue("Child.MyInt"), 10);
}

TEST_F(PropertyObjectTest, BeginEndUpdateCloned)
{
    const auto propObj = PropertyObject();
    const auto propObj1 = PropertyObject();
    propObj1.addProperty(IntProperty("MyInt", 10));
    propObj1.addProperty(StringProperty("MyString", "foo"));
    propObj.addProperty(ObjectProperty("Child", propObj1));

    const auto oldChild = propObj.getPropertyValue("Child");

    propObj.beginUpdate();

    const auto newChild = PropertyObject();
    newChild.addProperty(IntProperty("NewInt", 15));
    propObj.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("Child", newChild);
    ASSERT_EQ(propObj.getPropertyValue("Child"), oldChild);

    propObj.endUpdate();

    ASSERT_EQ(propObj.getPropertyValue("Child"), newChild);
}

TEST_F(PropertyObjectTest, BeginEndUpdateClonedClear)
{
    const auto propObj = PropertyObject();
    const auto propObj1 = PropertyObject();
    propObj1.addProperty(IntProperty("MyInt", 10));
    propObj1.addProperty(StringProperty("MyString", "foo"));
    propObj.addProperty(ObjectProperty("Child", propObj1));

    const auto oldChild = propObj.getPropertyValue("Child");
    const auto newChild = PropertyObject();
    newChild.addProperty(IntProperty("NewInt", 15));
    propObj.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("Child", newChild);

    propObj.beginUpdate();

    propObj.clearPropertyValue("Child");
    ASSERT_EQ(propObj.getPropertyValue("Child"), newChild);

    propObj.endUpdate();

    ASSERT_NE(propObj.getPropertyValue("Child"), newChild);
    ASSERT_NE(propObj.getPropertyValue("Child"), oldChild);
    ASSERT_EQ(propObj.getPropertyValue("Child.MyString"), "foo");
}

TEST_F(PropertyObjectTest, BeginEndUpdateClonedClassObject)
{
    const auto propObj = PropertyObject(objManager, "NestedObjectClass");
    const auto oldChild = propObj.getPropertyValue("Child");

    propObj.beginUpdate();

    const auto newChild = PropertyObject();
    newChild.addProperty(IntProperty("NewInt", 15));
    propObj.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("Child", newChild);

    ASSERT_EQ(propObj.getPropertyValue("Child"), oldChild);

    propObj.endUpdate();
    ASSERT_EQ(propObj.getPropertyValue("Child"), newChild);

    
    propObj.beginUpdate();

    propObj.clearPropertyValue("Child");
    ASSERT_EQ(propObj.getPropertyValue("Child"), newChild);

    propObj.endUpdate();

    ASSERT_NE(propObj.getPropertyValue("Child"), newChild);
    ASSERT_NE(propObj.getPropertyValue("Child"), oldChild);
    ASSERT_EQ(propObj.getPropertyValue("Child.Child.MyString"), "foo");
}

TEST_F(PropertyObjectTest, ClonedClassObjects)
{
    const auto propObj = PropertyObject(objManager, "NestedObjectClass");
    const PropertyObjectClassPtr classObj = objManager.getType("NestedObjectClass");
    ASSERT_NE(propObj.getPropertyValue("Child"), classObj.getProperty("Child").getDefaultValue());
    ASSERT_TRUE(classObj.getProperty("Child").getDefaultValue().isFrozen());
    ASSERT_FALSE(propObj.getPropertyValue("Child").isFrozen());
}

// TODO: Enable once nested object property deserialization works without classes
TEST_F(PropertyObjectTest, DISABLED_ClonedObjectsSerialize)
{
    const std::string expectedJson = R"({"__type":"PropertyObject","propValues":{"Child1":{"__type":"PropertyObject","propValues":{"Child2":{"__type":"PropertyObject"}}}}})";

    const PropertyObjectPtr propObj1 = PropertyObject();
    const PropertyObjectPtr propObj2 = PropertyObject();
    const PropertyObjectPtr propObj3 = PropertyObject();
    propObj3.addProperty(IntProperty("MyInt", 10));
    propObj2.addProperty(ObjectProperty("Child2", propObj3));
    propObj1.addProperty(ObjectProperty("Child1", propObj2));

    const auto serializer1 = JsonSerializer();
    propObj1.serialize(serializer1);

    const std::string json = serializer1.getOutput().toStdString();

    ASSERT_EQ(json, expectedJson);

    const auto deserializer = JsonDeserializer();

    const PropertyObjectPtr ptr = deserializer.deserialize(String(json.data()), objManager);

    const auto serializer2 = JsonSerializer();
    ptr.serialize(serializer2);

    const std::string deserializedJson = serializer2.getOutput().toStdString();

    ASSERT_EQ(json, deserializedJson);
}

TEST_F(PropertyObjectTest, ClonedClassObjectsSerialize)
{
    const std::string expectedJson = R"({"__type":"PropertyObject","className":"NestedObjectClass","propValues":{"Child":{"__type":"PropertyObject","className":"ObjectClass","propValues":{"Child":{"__type":"PropertyObject","properties":[{"__type":"Property","name":"MyString","valueType":3,"defaultValue":"foo","readOnly":false,"visible":true}]}}}}})";

    const auto propObj = PropertyObject(objManager, "NestedObjectClass");
    const auto serializer1 = JsonSerializer();
    propObj.serialize(serializer1);

    const std::string json = serializer1.getOutput().toStdString();

    ASSERT_EQ(json, expectedJson);

    const auto deserializer = JsonDeserializer();

    const PropertyObjectPtr ptr = deserializer.deserialize(String(json.data()), objManager);

    const auto serializer2 = JsonSerializer();
    ptr.serialize(serializer2);

    const std::string deserializedJson = serializer2.getOutput().toStdString();

    ASSERT_EQ(json, deserializedJson);
}

using BeginEndUpdatePropertyObjectTest = testing::Test;

TEST_F(BeginEndUpdatePropertyObjectTest, Recursive)
{
    auto childPropObj = PropertyObject();
    childPropObj.addProperty(StringPropertyBuilder("ChildStringProp", "-").build());

    auto propObj = PropertyObject();
    bool endUpdateCalled = false;
    propObj.getOnEndUpdate() += [&endUpdateCalled](PropertyObjectPtr&, EndUpdateEventArgsPtr& args)
    {
        ASSERT_THAT(args.getProperties(), testing::ElementsAre("StringProp"));
        endUpdateCalled = true;
    };

    propObj.addProperty(StringPropertyBuilder("StringProp", "-").build());
    propObj.addProperty(ObjectPropertyBuilder("ObjProp", childPropObj).build());

    childPropObj = propObj.getPropertyValue("ObjProp");
    bool childEndUpdateCalled = false;
    childPropObj.getOnEndUpdate() += [&childEndUpdateCalled](PropertyObjectPtr&, EndUpdateEventArgsPtr& args)
    {
        ASSERT_THAT(args.getProperties(), testing::ElementsAre("ChildStringProp"));
        childEndUpdateCalled = true;
    };

    propObj.beginUpdate();

    propObj.setPropertyValue("StringProp", "s");
    ASSERT_EQ(propObj.getPropertyValue("StringProp"), "-");

    childPropObj.setPropertyValue("ChildStringProp", "cs");
    ASSERT_EQ(childPropObj.getPropertyValue("ChildStringProp"), "-");

    propObj.endUpdate();

    ASSERT_TRUE(childEndUpdateCalled);
    ASSERT_TRUE(endUpdateCalled);
    ASSERT_EQ(propObj.getPropertyValue("StringProp"), "s");
    ASSERT_EQ(childPropObj.getPropertyValue("ChildStringProp"), "cs");
}
