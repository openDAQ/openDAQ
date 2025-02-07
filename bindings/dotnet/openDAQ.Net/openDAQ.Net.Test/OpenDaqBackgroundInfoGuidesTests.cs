//#define USE_LISTOBJECT
//#define USE_ITERATOR_NOT_IENUMERABLE

// Ignore Spelling: Opc Ua nullable daqref


using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Numerics;

using Daq.Core.Objects;
using Daq.Core.OpenDAQ;
using Daq.Core.Types;


namespace openDaq.Net.Test;


public class OpenDaqBackgroundInfoGuidesTests : OpenDAQTestsBase
{
    private const string REFERENCE_DEVICE = "daqref://device0";

    #region ./Antora/background_info/pages/device.adoc

    [Test]
    public void DeviceComponentsTest()
    {
        #region just for the test to compile
        var daqInstance = OpenDAQFactory.Instance(".");
        var device = daqInstance.AddDevice(REFERENCE_DEVICE);
        #endregion

        // Assumes a Device object is available in the `device` variable

        // Gets the Device's function blocks
        IListObject<FunctionBlock> functionBlocks = device.GetFunctionBlocks();
        // Gets the Device's Signals
        IListObject<Signal> signals = device.GetSignals();
        // Gets all Sub-Devices
        IListObject<Device> devices = device.GetDevices();

        // Gets the `io` folder.
        Folder inputsOutputsFolder = device.InputsOutputsFolder;
        // Shortcut to obtaining a list of channels that are the leaf Components of the `io` Folder
        IListObject<Channel> channels = device.GetChannels();

        // Gets all custom Components of the Device
        IListObject<Component> customComponents = device.CustomComponents;
    }

    [Test]
    public void DeviceInformationTest()
    {
        #region just for the test to compile
        var daqInstance = OpenDAQFactory.Instance(".");
        var device = daqInstance.AddDevice(REFERENCE_DEVICE);
        #endregion

        // Gets the Device information and prints its serial number
        DeviceInfo info = device.Info;
        Console.WriteLine(info.SerialNumber);

        // Prints the name of the first available device
        DeviceInfo availableDeviceInfo = device.AvailableDevices[0];
        Console.WriteLine(availableDeviceInfo.Name);
    }

    [Test]
    public void InstanceAndRootDeviceTest()
    {
        Instance instance = OpenDAQFactory.Instance();

        // The below two lines are equivalent.
        var availableDevices1 = instance.AvailableDevices;
        var availableDevices2 = instance.RootDevice.AvailableDevices;
    }

    #endregion device.adoc

    #region ./Antora/background_info/pages/function_blocks.adoc

    [Test]
    public void InputPortsTest()
    {
        #region just for the test to compile
        var daqInstance = OpenDAQFactory.Instance(".");
        var device = daqInstance.AddDevice(REFERENCE_DEVICE);
        var signal = device.GetChannels()[0].GetSignals()[0];
        var functionBlock = daqInstance.AddFunctionBlock("RefFBModuleStatistics");
        var inputPort = functionBlock.GetInputPorts()[0];
        #endregion

        inputPort.Connect(signal);
        // signal is now connected to the inputPort
        Signal signal1 = inputPort.Signal;
        Debug.Assert(signal1 == signal);
    }

    [Test]
    public void FunctionBlockInstantiationTest()
    {
        #region just for the test to compile
        var daqInstance = OpenDAQFactory.Instance(".");

        //foreach (var key in daqInstance.AvailableFunctionBlockTypes.Keys)
        //    Console.WriteLine((string)key);
        #endregion

        FunctionBlock fb = daqInstance.AddFunctionBlock("RefFBModuleFFT");
        // Function Block appears under FunctionBlocks of the instance
        IListObject<FunctionBlock> fbs = daqInstance.GetFunctionBlocks();
        FunctionBlock fb1 = fbs[fbs.Count - 1];
        Debug.Assert(fb == fb1);
    }

    [Test]
    public void ChannelsTest()
    {
        #region just for the test to compile
        var daqInstance = OpenDAQFactory.Instance(".");
        var device = daqInstance.AddDevice(REFERENCE_DEVICE);
        #endregion

        // get a flat list of channels
        IListObject<Channel> channels = device.GetChannels();
    }

    #endregion function_blocks.adoc

    #region ./Antora/background_info/pages/property_system.adoc

    [Test]
    public void SimplePropertyObjectExampleTest()
    {
        PropertyObject CreateSimplePropertyObject()
        {
            var propObj = CoreObjectsFactory.CreatePropertyObject();
            propObj.AddProperty(CoreObjectsFactory.CreateStringProperty("MyString", "foo", true));
            propObj.AddProperty(CoreObjectsFactory.CreateIntProperty("MyInteger", 0, true));
            return propObj;
        }

        var propObj = CreateSimplePropertyObject();
        Console.WriteLine((string)propObj.GetPropertyValue("MyString"));
        Console.WriteLine((long)propObj.GetPropertyValue("MyInteger"));

        propObj.SetPropertyValue("MyString", "bar");
        Console.WriteLine((string)propObj.GetPropertyValue("MyString"));
    }

    [Test]
    public void PropertyTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        propObj.AddProperty(CoreObjectsFactory.CreateStringProperty("MyString", "foo", true));
        propObj.SetPropertyValue("MyString", "bar");
        Console.WriteLine((string)propObj.GetPropertyValue("MyString"));
    }

    [Test]
    public void NumericalPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        var intProp = CoreObjectsFactory.CreateIntPropertyBuilder("Integer", 10);
        intProp.MinValue = 0;
        intProp.MaxValue = 15;
        propObj.AddProperty(intProp.Build());

        var suggestedValues = CoreTypesFactory.CreateList<BaseObject>();
        suggestedValues.Add((FloatObject)1.23);
        suggestedValues.Add((FloatObject)3.21);
        suggestedValues.Add((FloatObject)5.67);
        var floatProp = CoreObjectsFactory.CreateFloatPropertyBuilder("Float", 3.21);
        floatProp.SuggestedValues = suggestedValues;

        propObj.AddProperty(floatProp.Build());

        // "Integer" is set to 15 due to the max value
        propObj.SetPropertyValue("Integer", 20);
        Console.WriteLine((long)propObj.GetPropertyValue("Integer"));

        // "Float" is set to 2.34 despite the suggested values not containing the value 2.34
        propObj.SetPropertyValue("Float", 2.34);
        Console.WriteLine((double)propObj.GetPropertyValue("Float"));
    }

    [Test]
    public void SelectionPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        var list = CoreTypesFactory.CreateList<BaseObject>();
        list.Add("Apple");
        list.Add("Banana");
        list.Add("Kiwi");

        propObj.AddProperty(CoreObjectsFactory.CreateSelectionProperty("ListSelection", list, 0, true));

        var dict = CoreTypesFactory.CreateDict<BaseObject, BaseObject>();
        dict.Add(0, "foo");
        dict.Add(10, "bar");
        propObj.AddProperty(CoreObjectsFactory.CreateSparseSelectionProperty("DictSelection", dict, 10, true));

        // Prints "1"
        Console.WriteLine((long)propObj.GetPropertyValue("ListSelection"));
        // Prints "Banana"
        Console.WriteLine((string)propObj.GetPropertySelectionValue("ListSelection"));
        // Selects "Kiwi"
        propObj.SetPropertyValue("ListSelection", 2);

        // Prints "bar"
        Console.WriteLine((string)propObj.GetPropertySelectionValue("DictSelection"));
        // Selects "foo"
        propObj.SetPropertyValue("DictSelection", 0);
    }

    [Test]
    public void ObjectPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        var child1 = CoreObjectsFactory.CreatePropertyObject();
        var child2 = CoreObjectsFactory.CreatePropertyObject();

        // The order below is important, as "child1" and "child2" are frozen once
        // used as default property values.
        child2.AddProperty(CoreObjectsFactory.CreateStringProperty("String", "foo", true));
        child1.AddProperty(CoreObjectsFactory.CreateObjectProperty("Child", child2));
        propObj.AddProperty(CoreObjectsFactory.CreateObjectProperty("Child", child1));

        // Prints out the value of the "String" Property of child2
        Console.WriteLine((string)propObj.GetPropertyValue("Child.Child.String"));
    }

    [Test]
    public void ContainerPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        var list = CoreTypesFactory.CreateList<BaseObject>();
        list.Add("Banana");
        list.Add("Apple");
        list.Add("Kiwi");

        propObj.AddProperty(CoreObjectsFactory.CreateListProperty("List", list, true));

        var dict = CoreTypesFactory.CreateDict<BaseObject, BaseObject>();
        dict.Add(0, "foo");
        dict.Add(10, "bar");
        propObj.AddProperty(CoreObjectsFactory.CreateDictProperty("Dict", dict, true));

        // Prints out "Banana"
        Console.WriteLine((string)propObj.GetPropertyValue("List").CastList<StringObject>()[0]);
        // Prints out "bar"
        Console.WriteLine((string)propObj.GetPropertyValue("Dict").CastDict<IntegerObject, StringObject>()[10]);

        var list2 = CoreTypesFactory.CreateList<BaseObject>();
        list2.Add("Pear");
        list2.Add("Strawberry");

        // Sets a new value for the List Property
        propObj.SetPropertyValue("List", (BaseObject)list2);
    }

    [Test]
    public void ReferencePropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        propObj.AddProperty(CoreObjectsFactory.CreateIntProperty("Integer", 0, true));
        propObj.AddProperty(CoreObjectsFactory.CreateStringProperty("Prop1", "foo", true));
        propObj.AddProperty(CoreObjectsFactory.CreateStringProperty("Prop2", "bar", true));

        propObj.AddProperty(CoreObjectsFactory.CreateReferenceProperty("RefProp", CoreObjectsFactory.CreateEvalValue("switch($Integer, 0, %Prop1, 1, %Prop2)")));

        // Prints "foo"
        Console.WriteLine((string)propObj.GetPropertyValue("RefProp"));

        propObj.SetPropertyValue("Integer", 1);

        // Prints "bar"
        Console.WriteLine((string)propObj.GetPropertyValue("RefProp"));
    }

    [Test]
    public void RemainingPropertyTypesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        propObj.AddProperty(CoreObjectsFactory.CreateStringProperty("String", "foo", true));
        propObj.AddProperty(CoreObjectsFactory.CreateRatioProperty("Ratio", CoreTypesFactory.CreateRatio(1, 10), true));
        propObj.AddProperty(CoreObjectsFactory.CreateBoolProperty("Bool", true, true));
    }

    [Test]
    public void CreatingAndConfiguringAPropertyTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        PropertyBuilder floatProp = CoreObjectsFactory.CreateFloatPropertyBuilder("MyFloat", 1.123);
        floatProp.MinValue = 0.0;
        floatProp.MaxValue = 10.0;

        propObj.AddProperty(floatProp.Build());
    }

    [Test]
    public void ReferencingAnotherPropertyTest1()
    {
        #region just for the test to compile
        PropertyObject sineSettings = CoreObjectsFactory.CreatePropertyObject();
        PropertyObject counterSettings = CoreObjectsFactory.CreatePropertyObject();
        #endregion

        PropertyObject simulatedChannel = CoreObjectsFactory.CreatePropertyObject();

        var list = CoreTypesFactory.CreateList<BaseObject>();
        list.Add("Sine");
        list.Add("Counter");

        simulatedChannel.AddProperty(CoreObjectsFactory.CreateSelectionProperty("Waveform", list, 0, true));
        simulatedChannel.AddProperty(CoreObjectsFactory.CreateReferenceProperty("Settings", CoreObjectsFactory.CreateEvalValue("if($Waveform == 0, %SineSettings, %CounterSettings)")));

        //...

        simulatedChannel.AddProperty(CoreObjectsFactory.CreateObjectProperty("SineSettings", sineSettings));

        //...

        simulatedChannel.AddProperty(CoreObjectsFactory.CreateObjectProperty("CounterSettings", counterSettings));
    }

    [Test]
    public void ReferencingAnotherPropertyTest2()
    {
        // If the value of the "Waveform" Property equals 0, the EvalValue evaluates to the
        // "SineSettings" Property. If not, it evaluates to the "CounterSettings" Property.
        CoreObjectsFactory.CreateReferenceProperty("Settings", CoreObjectsFactory.CreateEvalValue("if($Waveform == 0, %SineSettings, %CounterSettings)"));
    }

    //[Test]
    public void ReferencingAnotherPropertyTest3()
    {
        // Not supported in .NET

        //#region just for the test to compile
        //PropertyBuilder scalingFactor = CoreObjectsFactory.CreateFloatPropertyBuilder("ScalingFactor", 1.0);
        //PropertyBuilder loopThreshold = CoreObjectsFactory.CreateIntPropertyBuilder("LoopThreshold", 100);
        //loopThreshold.MinValue = 1;
        //#endregion

        ////...

        //// The ScalingFactor Property is shown if EnableScaling is true.
        //scalingFactor.Visible = CoreObjectsFactory.CreateEvalValue("$EnableScaling");

        ////...

        //// The LoopThreshold Property is shown if the "Mode" Property is set to 1.
        //loopThreshold.Visible = CoreObjectsFactory.CreateEvalValue("$Mode == 1");
    }

    //[Test]
    public void ReferencingAnotherPropertyTest4()
    {
        // Not supported in .NET

        //#region just for the test to compile
        //PropertyObject sineSettings = CoreObjectsFactory.CreatePropertyObject();
        //PropertyBuilder amplitudeProp = CoreObjectsFactory.CreateFloatPropertyBuilder("Amplitude", 5);
        //#endregion

        //var list = CoreTypesFactory.CreateList<BaseObject>();
        //list.Add("V");
        //list.Add("mV");

        //sineSettings.AddProperty(CoreObjectsFactory.CreateSelectionProperty("AmplitudeUnit", list, 0, true));

        ////...

        //amplitudeProp.Unit = CoreObjectsFactory.CreateEvalValue("Unit(%AmplitudeUnit:SelectedValue)");
    }

    [Test]
    public void ValidationAndCoercionTest()
    {
        OpenDaqException ex = Assert.Throws<OpenDaqException>(() =>
        {
            var propObj = CoreObjectsFactory.CreatePropertyObject();
            var coercedProp = CoreObjectsFactory.CreateIntPropertyBuilder("CoercedProp", 5);
            coercedProp.Coercer = CoreObjectsFactory.CreateCoercer("if(Value < 10, Value, 10)");
            propObj.AddProperty(coercedProp.Build());

            var validatedProp = CoreObjectsFactory.CreateIntPropertyBuilder("ValidatedProp", 5);
            validatedProp.Validator = CoreObjectsFactory.CreateValidator("Value < 10");
            propObj.AddProperty(validatedProp.Build());

            // Sets the value to 10
            propObj.SetPropertyValue("CoercedProp", 15);

            // Throws a validation error
            propObj.SetPropertyValue("ValidatedProp", 15);
        });

        if (ex != null)
            Console.WriteLine($"{ex.GetType().Name} thrown with error code = {ex.ErrorCode}");
        else
            Console.WriteLine($"No exception thrown");

        Assert.That(ex.ErrorCode, Is.EqualTo(ErrorCode.OPENDAQ_ERR_VALIDATE_FAILED), "*** Wrong exception error code");
    }

    [Test]
    public void AddingRemovingPropertiesTest()
    {
        OpenDaqException ex = Assert.Throws<OpenDaqException>(() =>
        {
            var propObj = CoreObjectsFactory.CreatePropertyObject();
            propObj.AddProperty(CoreObjectsFactory.CreateStringProperty("foo", "bar", true));
            propObj.RemoveProperty("foo");

            // Throws a not found error
            var fooProp = propObj.GetProperty("foo");
        }, "*** No exception thrown");

        //here always ex != null
        Console.WriteLine($"as expected: {ex.GetType().Name} thrown with error code = {ex.ErrorCode}");

        Assert.That(ex.ErrorCode, Is.EqualTo(ErrorCode.OPENDAQ_ERR_NOTFOUND), "*** Wrong exception error code");
    }

    [Test]
    public void ListingPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        propObj.AddProperty(CoreObjectsFactory.CreateStringProperty("String", "foo", true));
        propObj.AddProperty(CoreObjectsFactory.CreateIntProperty("Int", 10, false));
        propObj.AddProperty(CoreObjectsFactory.CreateFloatProperty("Float", 15.0, true));
        propObj.AddProperty(CoreObjectsFactory.CreateReferenceProperty("FloatRef", CoreObjectsFactory.CreateEvalValue("%Float")));

        // Contains the Properties "String", "Int", "Float", "FloatRef"
        var allProps = propObj.AllProperties;
        // Contains the Properties "String", "FloatRef"
        var visibleProps = propObj.VisibleProperties;

        var order = CoreTypesFactory.CreateList<StringObject>();
        order.Add("FloatRef");
        order.Add("Float");
        order.Add("Int");
        order.Add("String");
        propObj.SetPropertyOrder(order);

        // Contains the Properties in the order "FloatRef", "Float", "Int", String"
        var allPropsReverseOrder = propObj.AllProperties;
    }

    [Test]
    public void ReadingWritingPropertyValuesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        propObj.AddProperty(CoreObjectsFactory.CreateStringProperty("String", "foo", false));

        // Prints "foo"
        Console.WriteLine((string)propObj.GetPropertyValue("String"));
        propObj.SetPropertyValue("String", "bar");
        // Prints "bar"
        Console.WriteLine((string)propObj.GetPropertyValue("String"));
    }

    [Test]
    public void RWNestedPropertyObjectsTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        var child1 = CoreObjectsFactory.CreatePropertyObject();
        var child2 = CoreObjectsFactory.CreatePropertyObject();

        child2.AddProperty(CoreObjectsFactory.CreateStringProperty("String", "foo", true));
        child1.AddProperty(CoreObjectsFactory.CreateObjectProperty("Child", child2));
        propObj.AddProperty(CoreObjectsFactory.CreateObjectProperty("Child", child1));

        propObj.SetPropertyValue("Child.Child.String", "bar");

        // Prints "bar"
        Console.WriteLine((string)propObj.GetPropertyValue("Child.Child.String"));
    }

    [Test]
    public void RWSelectionPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        var list = CoreTypesFactory.CreateList<BaseObject>();
        list.Add("Banana");
        list.Add("Kiwi");

        propObj.AddProperty(CoreObjectsFactory.CreateSelectionProperty("Selection", list, 1, true));

        // Prints "Kiwi"
        Console.WriteLine((string)propObj.GetPropertySelectionValue("Selection"));
    }

    [Test]
    public void RWListPropertiesTest1()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        var list = CoreTypesFactory.CreateList<BaseObject>();
        list.Add("Banana");
        list.Add("Kiwi");

        propObj.AddProperty(CoreObjectsFactory.CreateListProperty("List", list, true));

        // Prints "Banana"
        Console.WriteLine((string)propObj.GetPropertyValue("List[0]"));

        var list2 = CoreTypesFactory.CreateList<BaseObject>();
        list2.Add("Pear");
        list2.Add("Strawberry");

        // Sets a new value to the List Property.
        propObj.SetPropertyValue("List", (BaseObject)list2);

        // Prints "[ Pear, Strawberry ]"
        Console.WriteLine(propObj.GetPropertyValue("List").ToString());
    }

    [Test]
    public void RWListPropertiesTest2()
    {
        #region just for the test to compile
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        var list0 = CoreTypesFactory.CreateList<BaseObject>();
        list0.Add("Banana");
        list0.Add("Kiwi");

        propObj.AddProperty(CoreObjectsFactory.CreateListProperty("List", list0, true));
        #endregion

        IListObject<BaseObject> list = propObj.GetPropertyValue("List").CastList<BaseObject>();
        list.Add("Blueberry");
        propObj.SetPropertyValue("List", (BaseObject)list);

        // Prints "[ Banana, Kiwi, Blueberry ]"
        Console.WriteLine(propObj.GetPropertyValue("List").ToString());
    }

    [Test]
    public void RWDictionaryPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        var dict0 = CoreTypesFactory.CreateDict<BaseObject, BaseObject>();
        dict0.Add(1, "Banana");
        dict0.Add(12, "Kiwi");
        propObj.AddProperty(CoreObjectsFactory.CreateDictProperty("Dict", dict0, true));

        IDictObject<IntegerObject, StringObject> dict = propObj.GetPropertyValue("Dict").CastDict<IntegerObject, StringObject>();
        dict[3] = "Blueberry";

        // The "Dict" property now contains {1 : "Banana"}, {2 : "Kiwi"}, {3 : "Blueberry"}
        propObj.SetPropertyValue("Dict", (BaseObject)dict);
    }

    [Test]
    public void PropertyObjectClassTest()
    {
        PropertyObjectClassBuilder propClass = CoreObjectsFactory.CreatePropertyObjectClassBuilder("MyClass");

        propClass.AddProperty(CoreObjectsFactory.CreateIntProperty("Integer", 10, true));

        var list = CoreTypesFactory.CreateList<BaseObject>();
        list.Add("Banana");
        list.Add("Apple");
        list.Add("Kiwi");

        propClass.AddProperty(CoreObjectsFactory.CreateSelectionProperty("Selection", list, 1, true));
    }

    [Test]
    public void POCManageTest()
    {
        TypeManager manager = CoreTypesFactory.CreateTypeManager();

        PropertyObjectClassBuilder propClass = CoreObjectsFactory.CreatePropertyObjectClassBuilder("MyClass");

        propClass.AddProperty(CoreObjectsFactory.CreateIntProperty("Integer", 10, true));

        var list = CoreTypesFactory.CreateList<BaseObject>();
        list.Add("Banana");
        list.Add("Apple");
        list.Add("Kiwi");

        propClass.AddProperty(CoreObjectsFactory.CreateSelectionProperty("Selection", list, 1, true));

        manager.AddType(propClass.Build());

        PropertyObject propObj = CoreObjectsFactory.CreatePropertyObjectWithClassAndManager(manager, "MyClass");

        // Prints "Apple"
        Console.WriteLine((string)propObj.GetPropertySelectionValue("Selection"));
    }

    [Test]
    public void POCClassInheritanceTest()
    {
        TypeManager manager = CoreTypesFactory.CreateTypeManager();

        PropertyObjectClassBuilder propClass1 = CoreObjectsFactory.CreatePropertyObjectClassBuilderWithManager(manager, "InheritedClass");
        propClass1.AddProperty(CoreObjectsFactory.CreateStringProperty("InheritedProp", "foo", true));
        manager.AddType(propClass1.Build());

        PropertyObjectClassBuilder propClass2 = CoreObjectsFactory.CreatePropertyObjectClassBuilderWithManager(manager, "MyClass");
        propClass2.AddProperty(CoreObjectsFactory.CreateStringProperty("OwnProp", "bar", true));
        propClass2.ParentName = "InheritedClass";
        manager.AddType(propClass2.Build());

        PropertyObject propObj = CoreObjectsFactory.CreatePropertyObjectWithClassAndManager(manager, "MyClass");

        // Prints "foo"
        Console.WriteLine((string)propObj.GetPropertyValue("InheritedProp"));
        // Prints "bar"
        Console.WriteLine((string)propObj.GetPropertyValue("OwnProp"));
    }

    #endregion property_system.adoc

    //template snippet - clone these lines and rename the method for a new test
    //[Test]
    public void _EmptyTest()
    {
    }
}
