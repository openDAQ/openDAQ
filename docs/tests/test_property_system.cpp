#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

#include "coreobjects/argument_info_factory.h"
#include "coreobjects/callable_info_factory.h"

using PropertySystemTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/internals/pages/property_system.adoc
TEST_F(PropertySystemTest, SimpleExample)
{
    auto propObj = PropertyObject();
    propObj.addProperty(StringProperty("MyString", "foo"));
    propObj.addProperty(IntProperty("MyInteger", 0));

    ASSERT_EQ(propObj.getPropertyValue("MyString"), "foo");
    ASSERT_EQ(propObj.getPropertyValue("MyInteger"), 0);

    propObj.setPropertyValue("MyString", "bar");
    ASSERT_EQ(propObj.getPropertyValue("MyString"), "bar");
}

// Corresponding document: Antora/modules/internals/pages/property_system.adoc
TEST_F(PropertySystemTest, NumericalProps)
{
    auto propObj = PropertyObject();
    auto intProp = IntPropertyBuilder("Integer", 10).setMinValue(0).setMaxValue(15).build();
    propObj.addProperty(intProp);

    auto floatProp = FloatPropertyBuilder("Float", 3.21).setSuggestedValues(List<IFloat>(1.23, 3.21, 5.67)).build();
    propObj.addProperty(floatProp);
    
    propObj.setPropertyValue("Integer", 20);
    ASSERT_EQ(propObj.getPropertyValue("Integer"), 15);
        
    propObj.setPropertyValue("Float", 2.34);
    ASSERT_EQ(propObj.getPropertyValue("Float"), 2.34);
}

// Corresponding document: Antora/modules/internals/pages/property_system.adoc
TEST_F(PropertySystemTest, SelectionProps)
{
    auto propObj = PropertyObject();
    propObj.addProperty(SelectionProperty("ListSelection", List<IString>("Apple", "Banana", "Kiwi"), 1));

    auto dict = Dict<Int, IString>();
    dict.set(0, "foo");
    dict.set(10, "bar");
    propObj.addProperty(SparseSelectionProperty("DictSelection", dict, 10));

    // Prints "1"
    ASSERT_EQ(propObj.getPropertyValue("ListSelection"), 1);
    // Prints "Banana"
    ASSERT_EQ(propObj.getPropertySelectionValue("ListSelection"), "Banana");
    // Selects "Kiwi"
    propObj.setPropertyValue("ListSelection", 2);

    // Prints "bar"
    ASSERT_EQ(propObj.getPropertySelectionValue("DictSelection"), "bar");
    // Selects "foo"
    propObj.setPropertyValue("DictSelection", 0);
}

// Corresponding document: Antora/modules/internals/pages/property_system.adoc
TEST_F(PropertySystemTest, ObjectProps)
{
    auto propObj = PropertyObject();
    auto child1 = PropertyObject();
    auto child2 = PropertyObject();

    propObj.addProperty(ObjectProperty("Child", child1));
    child1.addProperty(ObjectProperty("Child", child2));
    child2.addProperty(StringProperty("String", "foo"));

    // Prints out the value of the "String" property of child2
    ASSERT_EQ(propObj.getPropertyValue("Child.Child.String"), "foo");
}

TEST_F(PropertySystemTest, ContainerProps)
{
    auto propObj = PropertyObject();
    propObj.addProperty(ListProperty("List", List<IString>("Banana", "Apple", "Kiwi")));

    auto dict = Dict<Int, IString>();
    dict.set(0, "foo");
    dict.set(10, "bar");
    propObj.addProperty(DictProperty("Dict", dict));

    ASSERT_EQ(propObj.getPropertyValue("List").asPtr<IList>()[0], "Banana");
    ASSERT_EQ(propObj.getPropertyValue("Dict").asPtr<IDict>().get(10), "bar");
    
    propObj.setPropertyValue("List", List<IString>("Pear", "Strawberry"));
}

TEST_F(PropertySystemTest, ReferenceProps)
{
    auto propObj = PropertyObject();
    propObj.addProperty(IntProperty("Integer", 0));
    propObj.addProperty(StringProperty("Prop1", "foo"));
    propObj.addProperty(StringProperty("Prop2", "bar"));

    propObj.addProperty(ReferenceProperty("RefProp", EvalValue("switch($Integer, 0, %Prop1, 1, %Prop2)")));
    
    ASSERT_EQ(propObj.getPropertyValue("RefProp"), "foo");

    propObj.setPropertyValue("Integer", 1);
    
    ASSERT_EQ(propObj.getPropertyValue("RefProp"), "bar");
}

TEST_F(PropertySystemTest, FunctionProps)
{
    auto propObj = PropertyObject();

    auto arguments = List<IArgumentInfo>(ArgumentInfo("Val1", ctInt), ArgumentInfo("Val2", ctInt));
    propObj.addProperty(FunctionProperty("SumFunction", FunctionInfo(ctInt, arguments)));

    auto func = Function([](IntegerPtr val1, IntegerPtr val2)
    {
        return val1 + val2;
    });
    propObj.setPropertyValue("SumFunction", func);

    FunctionPtr sumFunc = propObj.getPropertyValue("SumFunction");

    ASSERT_EQ(sumFunc(12, 30), 42);
}

TEST_F(PropertySystemTest, OtherProps)
{
    auto propObj = PropertyObject();
    propObj.addProperty(StringProperty("String", "foo"));
    propObj.addProperty(RatioProperty("Ratio", Ratio(1, 10)));
    propObj.addProperty(BoolProperty("Bool", true));
}

TEST_F(PropertySystemTest, CreateConfigureProp)
{
    auto propObj = PropertyObject();

    PropertyPtr floatProp = FloatPropertyBuilder("MyFloat", 1.123).setMinValue(0.0).setMaxValue(10.0).build();

    propObj.addProperty(floatProp);
}

TEST_F(PropertySystemTest, SimulatedChannelProp)
{
    PropertyObjectPtr simulatedChannel = PropertyObject();
    simulatedChannel.addProperty(SelectionProperty("Waveform", List<IString>("Sine", "Counter"), 0));
    simulatedChannel.addProperty(ReferenceProperty("Settings", EvalValue("if($Waveform == 0, %SineSettings, %CounterSettings)")));

    PropertyPtr freqProp = FloatPropertyBuilder("Frequency", 10.0)
                               .setUnit(Unit("Hz"))
                               .setMinValue(0.1)
                               .setMaxValue(1000.0)
                               .setSuggestedValues(List<IFloat>(0.1, 10.0, 100.0, 1000.0))
                               .build();
    simulatedChannel.addProperty(freqProp);

    // Sine settings

    PropertyObjectPtr sineSettings = PropertyObject();

    sineSettings.addProperty(SelectionProperty("AmplitudeUnit", List<IString>("V", "mV"), 0));

    PropertyPtr amplitudeProp = FloatPropertyBuilder("Amplitude", 5).setUnit(EvalValue("Unit(%AmplitudeUnit:SelectedValue)")).build();
    sineSettings.addProperty(amplitudeProp);

    sineSettings.addProperty(BoolProperty("EnableScaling", false));

    PropertyPtr scalingFactor = FloatPropertyBuilder("ScalingFactor", 1.0).setVisible(EvalValue("$EnableScaling")).build();
    sineSettings.addProperty(scalingFactor);

    simulatedChannel.addProperty(ObjectProperty("SineSettings", sineSettings));

    // Counter settings

    PropertyObjectPtr counterSettings = PropertyObject();

    counterSettings.addProperty(IntProperty("Increment", 1));

    counterSettings.addProperty(SelectionProperty("Mode", List<IString>("Infinite", "Loop"), 0));

    PropertyPtr loopThreshold = IntPropertyBuilder("LoopThreshold", 100).setMinValue(1).setVisible(EvalValue("$Mode == 1")).build();
    counterSettings.addProperty(loopThreshold);

    PropertyPtr resetProp = FunctionPropertyBuilder("Reset", ProcedureInfo()).setReadOnly(true).setVisible(EvalValue("$Mode == 0")).build();
    counterSettings.addProperty(resetProp);
    counterSettings.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("Reset", Procedure([](){}));

    simulatedChannel.addProperty(ObjectProperty("CounterSettings", counterSettings));
}

TEST_F(PropertySystemTest, ValidationCoercion)
{
    auto propObj = PropertyObject();
    auto coercedProp = IntPropertyBuilder("CoercedProp", 5).setCoercer(Coercer("if(Value < 10, Value, 10)")).build();
    propObj.addProperty(coercedProp);

    auto validatedProp = IntPropertyBuilder("ValidatedProp", 5).setValidator(Validator("Value < 10")).build();
    propObj.addProperty(validatedProp);
    
    propObj.setPropertyValue("CoercedProp", 15);
    ASSERT_EQ(propObj.getPropertyValue("CoercedProp"), 10);

    ASSERT_ANY_THROW(propObj.setPropertyValue("ValidatedProp", 15));
}

TEST_F(PropertySystemTest, RemoveProp)
{
    auto propObj = PropertyObject();
    propObj.addProperty(StringProperty("foo", "bar"));
    propObj.removeProperty("foo");
}

TEST_F(PropertySystemTest, ListingProps)
{
    auto propObj = PropertyObject();
    propObj.addProperty(StringProperty("String", "foo"));
    propObj.addProperty(IntProperty("Int", 10, false));
    propObj.addProperty(FloatProperty("Float", 15.0));
    propObj.addProperty(ReferenceProperty("FloatRef", EvalValue("%Float")));
    
    auto allProps = propObj.getAllProperties();
    ASSERT_EQ(allProps.getCount(), 4);
    auto visibleProps = propObj.getVisibleProperties();
    ASSERT_EQ(visibleProps.getCount(), 2);

    auto order = List<IString>("FloatRef", "Float", "Int", "String");
    propObj.setPropertyOrder(order);
    
    auto allPropsReverseOrder = propObj.getAllProperties();
    ASSERT_EQ(allPropsReverseOrder[0].getName(), "FloatRef");
}


TEST_F(PropertySystemTest, ReadWritePropValues)
{
    auto propObj = PropertyObject();
    propObj.addProperty(StringProperty("String", "foo"));

    ASSERT_EQ(propObj.getPropertyValue("String"), "foo");
    propObj.setPropertyValue("String", "bar");
    ASSERT_EQ(propObj.getPropertyValue("String"), "bar");
}

TEST_F(PropertySystemTest, NestedObjects)
{
    auto propObj = PropertyObject();

    auto child1 = PropertyObject();
    propObj.addProperty(ObjectProperty("Child", child1));

    auto child2 = PropertyObject();
    child2.addProperty(StringProperty("String", "foo"));
    child1.addProperty(ObjectProperty("Child", child2));

    propObj.setPropertyValue("Child.Child.String", "bar");
    ASSERT_EQ(propObj.getPropertyValue("Child.Child.String"), "bar");
}

TEST_F(PropertySystemTest, SelectionPropertiesRead)
{
    auto propObj = PropertyObject();
    propObj.addProperty(SelectionProperty("Selection", List<IString>("Banana", "Kiwi"), 1));

    ASSERT_EQ(propObj.getPropertySelectionValue("Selection"), "Kiwi");
}

TEST_F(PropertySystemTest, ListPropertiesRead)
{
    auto propObj = PropertyObject();
    propObj.addProperty(ListProperty("List", List<IString>("Banana", "Kiwi")));

    ASSERT_EQ(propObj.getPropertyValue("List[0]"), "Banana");
}

TEST_F(PropertySystemTest, EventsPropObj)
{
    auto propObj = PropertyObject();
    propObj.addProperty(IntProperty("IntReadCount", 0));
    propObj.addProperty(IntProperty("Int", 10));
    
    propObj.getOnPropertyValueWrite("Int") += 
      [](PropertyObjectPtr& /*sender*/, PropertyValueEventArgsPtr& args)
      {
        Int writtenValue = args.getValue();
        if (writtenValue > 20)
        {
          args.setValue(20);
        }
      };
    
    propObj.getOnPropertyValueRead("Int") +=
      [](PropertyObjectPtr& sender, PropertyValueEventArgsPtr& /*args*/)
      {
        IntegerPtr readCount = sender.getPropertyValue("IntReadCount");
        sender.setPropertyValue("IntReadCount", readCount + 1);
      };

    propObj.setPropertyValue("Int", 30);
    ASSERT_EQ(propObj.getPropertyValue("Int"), 20);
    ASSERT_EQ(propObj.getPropertyValue("IntReadCount"), 1);
}

TEST_F(PropertySystemTest, EventsProp)
{
    TypeManagerPtr manager = TypeManager();

    int readCount = 0;

    auto intProp = IntProperty("ReadCount", 0);
    intProp.getOnPropertyValueRead() +=
      [&](PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args)
      {
        readCount++;
        sender.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue("ReadCount", readCount + 1);
        args.setValue(readCount);
    };

    PropertyObjectClassPtr propClass = PropertyObjectClassBuilder(manager, "MyClass").addProperty(intProp).build();
    manager.addType(propClass);

    auto propObj1 = PropertyObject(manager, "MyClass");
    auto propObj2 = PropertyObject(manager, "MyClass");

    ASSERT_EQ(propObj1.getPropertyValue("ReadCount"), 1);
    ASSERT_EQ(propObj2.getPropertyValue("ReadCount"), 2);
}

TEST_F(PropertySystemTest, Class)
{
    PropertyObjectClassPtr propClass = PropertyObjectClassBuilder("MyClass")
                                                  .addProperty(IntProperty("Integer", 10))
                                                  .addProperty(SelectionProperty("Selection", List<IString>("Banana", "Apple", "Kiwi"), 0))
                                                  .build();
}


TEST_F(PropertySystemTest, Manager)
{
    TypeManagerPtr manager = TypeManager();

    PropertyObjectClassPtr propClass = PropertyObjectClassBuilder("MyClass")
                                           .addProperty(IntProperty("Integer", 10))
                                           .addProperty(SelectionProperty("Selection", List<IString>("Banana", "Apple", "Kiwi"), 1))
                                           .build();

    manager.addType(propClass);

    PropertyObjectPtr propObj = PropertyObject(manager, "MyClass");

    ASSERT_EQ(propObj.getPropertySelectionValue("Selection"), "Apple");
}

TEST_F(PropertySystemTest, Inheritance)
{
    TypeManagerPtr manager = TypeManager();

    PropertyObjectClassPtr propClass1 =
        PropertyObjectClassBuilder(manager, "InheritedClass").addProperty(StringProperty("InheritedProp", "foo")).build();
    manager.addType(propClass1);

    PropertyObjectClassPtr propClass2 = PropertyObjectClassBuilder(manager, "MyClass")
                                            .addProperty(StringProperty("OwnProp", "bar"))
                                            .setParentName("InheritedClass")
                                            .build();
    manager.addType(propClass2);

    auto propObj = PropertyObject(manager, "MyClass");

    ASSERT_EQ(propObj.getPropertyValue("InheritedProp"), "foo");
    ASSERT_EQ(propObj.getPropertyValue("OwnProp"), "bar");
}
END_NAMESPACE_OPENDAQ
