// Ignore Spelling: Opc Ua nullable daqref


using System;
using System.Diagnostics;
using System.Drawing;
using System.Reflection;
using System.Runtime.InteropServices;

using Daq.Core.Objects;
using Daq.Core.OpenDAQ;
using Daq.Core.Types;

//using static Daq.Core.Objects.CoreObjectsFactory;
//using static Daq.Core.OpenDAQ.OpenDAQFactory;
//using static Daq.Core.Types.CoreTypesFactory;


namespace openDaq.Net.Test;


public class OpenDaqExplanationsTests : OpenDAQTestsBase
{
    public const string SKIP_SETUP                    = "SkipSetup";
    public const string SKIP_SETUP_DEVICE             = "SkipSetupDevice";
    public const string SETUP_DEVICE_NATIVE_STREAMING = "SetupDeviceNativeStreaming";

    private const string MODULE_PATH = ".";

    private const string ConnectionProtocolDaqRef  = "daqref://";
    private const string ConnectionProtocolOpcUa   = "daq.opcua://";
    private const string ConnectionProtocolWinSock = "daq.lt://";

    private const string ConnectionStringDaqRef = "daqref://device0";

    public enum eDesiredConnection
    {
        Any = 0,
        DaqRef,
        OpcUa,
        WinSock,
        LocalHost,
        NativeStreaming
    }


#pragma warning disable CS8625 // Cannot convert null literal to non-nullable reference type.
    private Instance            instance = null;
    private Device              device   = null;
    private Signal              signal   = null;
//    private IListObject<Signal> signals  = null;
#pragma warning restore CS8625 // Cannot convert null literal to non-nullable reference type.



    [SetUp]
    public void _SetUp()
    {
        //turn off everything for any test here
        base.DontCollectAndFinalize();
        //base.DontCheckAliveObjectCount();
        base.DontWarn();

        if (CheckForSkipSetup(SKIP_SETUP)) //ToDo: use [Category(SKIP_SETUP)] to mark a test for not running this method
        {
            Console.WriteLine($"* skipping Setup()");
            return;
        }

        Console.WriteLine(">>> SetUp() - create instance");

        instance = OpenDAQFactory.Instance(MODULE_PATH);

        if (!CheckForSkipSetup(SKIP_SETUP_DEVICE)) //ToDo: use [Category(SKIP_SETUP_DEVICE)] to mark a test for not connecting the default device
        {
            Console.WriteLine(">>> SetUp() - connect first available device (any)");

            if (!CheckForSkipSetup(SETUP_DEVICE_NATIVE_STREAMING))
            {
                //device = instance.AddDevice(ConnectionStringDaqRef);
                device = ConnectFirstAvailableDevice(instance, eDesiredConnection.Any);
            }
            else
                device = ConnectFirstAvailableDevice(instance, eDesiredConnection.NativeStreaming);

            using var allSignals = device.GetChannels()[0].GetSignals();
            signal = allSignals[0];
//            signals = CoreTypesFactory.CreateList<Signal>();
        }


        // local functions ========================================================================

        static bool CheckForSkipSetup(string category)
        {
            var categories = TestContext.CurrentContext.Test.Properties["Category"];
            bool skipSetup = categories.Contains(category);
            return skipSetup;
        }
    }

    [TearDown]
    public void _TearDown()
    {
        //Console.WriteLine("+ TearDown()");
        if (instance == null)
        {
            Console.WriteLine("* skipping TearDown()");
            return;
        }

//        if (signals != null)
//        {
//            signals.Dispose();
//#pragma warning disable CS8625 // Cannot convert null literal to non-nullable reference type.
//            signals = null;
//#pragma warning restore CS8625 // Cannot convert null literal to non-nullable reference type.
//        }

        if (signal != null)
        {
            signal.Dispose();
#pragma warning disable CS8625 // Cannot convert null literal to non-nullable reference type.
            signal = null;
#pragma warning restore CS8625 // Cannot convert null literal to non-nullable reference type.
        }

        //Hack: dispose of all connected devices and then dispose of the instance to avoid GC messing up with that sequence
        if (device != null)
        {
            Console.WriteLine(">>> TearDown() - disconnect device");

            instance.RemoveDevice(device);
            device.Dispose();
#pragma warning disable CS8625 // Cannot convert null literal to non-nullable reference type.
            device = null;
#pragma warning restore CS8625 // Cannot convert null literal to non-nullable reference type.
        }

        IListObject<Device> devices = instance.GetDevices();

        Console.WriteLine($">>> TearDown() - disconnect {devices.Count} still connected devices");
        foreach (var connectedDevice in devices)
            connectedDevice.Dispose();

        devices.Dispose();

        Console.WriteLine($">>> TearDown() - dispose instance");

        instance.Dispose();
#pragma warning disable CS8625 // Cannot convert null literal to non-nullable reference type.
        instance = null;
#pragma warning restore CS8625 // Cannot convert null literal to non-nullable reference type.
    }



    private static Device ConnectFirstAvailableDevice(Instance daqInstance,
                                                      eDesiredConnection desiredConnection = eDesiredConnection.DaqRef,
                                                      bool doLog = false)
    {
        Console.WriteLine($"Trying to connect to {desiredConnection} device");

        if (doLog) Console.WriteLine("daqInstance.AvailableDevices");
        var availableDevicesInfos = daqInstance.AvailableDevices;
        var deviceInfoCount = availableDevicesInfos.Count;
        if (doLog) Console.WriteLine($"  {deviceInfoCount} devices available");

        //take only the first valid connection string
        string? connectionString = null;

        bool doDaqRef          = (desiredConnection == eDesiredConnection.Any) || (desiredConnection == eDesiredConnection.DaqRef);
        bool doOpcUa           = (desiredConnection == eDesiredConnection.Any) || (desiredConnection == eDesiredConnection.OpcUa);
        bool doWinSock         = (desiredConnection == eDesiredConnection.Any) || (desiredConnection == eDesiredConnection.WinSock);
        bool doLocalHost       = (desiredConnection == eDesiredConnection.Any) || (desiredConnection == eDesiredConnection.LocalHost);
        bool doNativeStreaming =                                                  (desiredConnection == eDesiredConnection.NativeStreaming);

        //foreach (var deviceInfo in availableDevicesInfos) { }

        using (var deviceInfoIterator = availableDevicesInfos.GetEnumerator())
        {
            while (deviceInfoIterator.MoveNext())
            {
                using var deviceInfo       = deviceInfoIterator.Current;
                var deviceName             = deviceInfo.Name;
                var deviceConnectionString = deviceInfo.ConnectionString;

                if (deviceName.StartsWith("HBK-CAL"))
                    continue;

                if (deviceName.StartsWith("HBK-HF-"))
                    continue;

                if (deviceName.StartsWith("HBK-SY-"))
                    continue;

                //uncomment when only local VM should be used (home office)
                //if (deviceConnectionString.StartsWith("daq.opcua://172."))
                //    continue;

                if (!doNativeStreaming)
                {
                    //connectible device?
                    if ((doOpcUa && deviceConnectionString.StartsWith(ConnectionProtocolOpcUa, StringComparison.InvariantCultureIgnoreCase))
                        || (doWinSock && deviceConnectionString.StartsWith(ConnectionProtocolWinSock, StringComparison.InvariantCultureIgnoreCase))
                        || (doDaqRef && deviceConnectionString.StartsWith(ConnectionProtocolDaqRef, StringComparison.InvariantCultureIgnoreCase)))
                    {
                        connectionString = deviceConnectionString;
                        break;
                    }
                }
                else
                {
                    // Find and connect to a Device hosting an Native Streaming server
                    using var capability = deviceInfo.ServerCapabilities.FirstOrDefault(capability => capability.ProtocolName == "OpenDAQNativeStreaming");
                    if (capability != null)
                    {
                        connectionString = capability.ConnectionString;
                        break;
                    }
                }
            }
        }

        availableDevicesInfos.Dispose();

        if (doLocalHost && string.IsNullOrEmpty(connectionString))
        {
            connectionString = "daq.opcua://127.0.0.1";
        }

        Assert.That(connectionString, Is.Not.Null, $"*** Test aborted - No connectible device available ({nameof(desiredConnection)} = {desiredConnection}).");

        Console.WriteLine($"daqInstance.AddDevice(\"{connectionString}\")");
        using var connectionStringObject = (StringObject)connectionString;
        //using var propertyObject = CoreObjectsFactory.CreatePropertyObject();
        var addedDevice = daqInstance.AddDevice(connectionStringObject/*, propertyObject*/);

        Assert.That(addedDevice, Is.Not.Null, "*** Test aborted - Device connection failed.");

        var deviceNameStr = addedDevice.Name;
        Console.WriteLine($"- Device to test: '{deviceNameStr}'");

        return addedDevice;
    }


    // Component Tree

    #region Components ./Antora/modules/explanations/pages/components.adoc

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void Test_0001_ComponentTreeComponentStatusesTest()
    {
        var scalingFb = instance.AddFunctionBlock("RefFBModuleScaling");
        Console.Write("The Scaling Function Block available statuses: ");
        foreach (string statusName in scalingFb.StatusContainer.Statuses.Keys)
            Console.Write($"\"{statusName}\"; ");
        Console.WriteLine();
        Console.WriteLine("The Scaling Function Block \"ComponentStatus\" is {0}",
                          scalingFb.StatusContainer.GetStatus("ComponentStatus"));
        Console.WriteLine("The message is {0}",
                          scalingFb.StatusContainer.GetStatusMessage("ComponentStatus"));
    }

    [Test]
    public void Test_0002_ComponentTreeTraversingFoldersTest()
    {
        #region not for documentation
        FindSignals(device);
        #endregion not for documentation

        void FindSignals(Device device)
        {
            foreach (var signal in device.GetSignals(SearchFactory.Recursive(SearchFactory.Any())))
                Console.WriteLine(signal.GlobalId);
        }
    }

    #endregion Components

    #region Device ./Antora/modules/explanations/pages/device.adoc

    [Test]
    public void Test_0101_ComponentTreeDeviceComponentsTest()
    {
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

        // Gets the SyncComponent of the Device
        SyncComponent syncComponent = device.SyncComponent;
    }

    [Test]
    public void Test_0102_ComponentTreeDeviceInformationTest()
    {
        // Gets the Device information and prints its serial number
        DeviceInfo info = device.Info;
        Console.WriteLine(info.SerialNumber);

        // Prints the name of the first available device
        DeviceInfo availableDeviceInfo = device.AvailableDevices[0];
        Console.WriteLine(availableDeviceInfo.Name);
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0103_ComponentTreeInstanceAndRootDeviceTest()
    {
        Instance instance = OpenDAQFactory.Instance();

        // The below two lines are equivalent.
        var availableDevices1 = instance.AvailableDevices;
        var availableDevices2 = instance.RootDevice.AvailableDevices;
    }

    #endregion Device

    #region Function Block ./Antora/modules/explanations/pages/components.adoc

    [Test]
    public void Test_0201_ComponentTreeFunctionBlockInputPortsTest()
    {
        #region not for documentation
        var fb = instance.AddFunctionBlock("RefFBModuleScaling");
        var inputPort = fb.GetInputPorts()[0];
        #endregion not for documentation

        inputPort.Connect(signal);
        // signal is now connected to the inputPort
        Signal signal1 = inputPort.Signal;
        Debug.Assert(signal1 == signal);
    }

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void Test_0202_ComponentTreeFunctionBlockInstantiationTest()
    {
        FunctionBlock fb = instance.AddFunctionBlock("RefFBModuleFFT");
        // Function Block appears under FunctionBlocks of the instance
        IListObject<FunctionBlock> fbs = instance.GetFunctionBlocks();
        FunctionBlock fb1 = fbs[fbs.Count - 1];
        Debug.Assert(fb == fb1);
    }

    [Test]
    public void Test_0203_ComponentTreeFunctionBlockChannelsTest()
    {
        // get a flat list of channels
        IListObject<Channel> channels = device.GetChannels();
    }

    #endregion Function Block

    #region Signal ./Antora/modules/explanations/pages/signals.adoc
    //Test_0301_

    #endregion Signal

    #region Data Path ./Antora/modules/explanations/pages/data_path.adoc

    [Test]
    public void Test_0401_ComponentTreeDataPathConnectionTest()
    {
        #region not for documentation
        var fb = instance.AddFunctionBlock("RefFBModuleScaling");
        var inputPort = fb.GetInputPorts()[0];
        #endregion not for documentation

        inputPort.Connect(signal);
        // connection object is now accessible on inputPort
        Connection connection = inputPort.Connection;
    }

    [Test]
    public void Test_0402_ComponentTreeDataPathInputPortTest()
    {
        #region not for documentation
        var fb = instance.AddFunctionBlock("RefFBModuleScaling");
        var inputPort = fb.GetInputPorts()[0];
        inputPort.Connect(signal);
        #endregion not for documentation

        Packet packet = inputPort.Connection.Dequeue();
        // process packet, i.e. read samples from the packet
        //ProcessPacket(packet);
    }

    #endregion Data Path


    // Features

    #region Property system ./Antora/modules/explanations/pages/property_system.adoc

    #region Simple Property Object

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0501_FeaturesPropertySystemSimplePropertyObjectTest()
    {
        PropertyObject CreateSimplePropertyObject()
        {
            var propObj = CoreObjectsFactory.CreatePropertyObject();
            propObj.AddProperty(PropertyFactory.StringProperty("MyString", "foo"));
            propObj.AddProperty(PropertyFactory.IntProperty("MyInteger", 0));
            return propObj;
        }

        var propObj = CreateSimplePropertyObject();
        Console.WriteLine((string)propObj.GetPropertyValue("MyString"));
        Console.WriteLine((long)propObj.GetPropertyValue("MyInteger"));

        propObj.SetPropertyValue("MyString", "bar");
        Console.WriteLine((string)propObj.GetPropertyValue("MyString"));
    }

    #endregion Simple Property Object

    #region Property

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0511_FeaturesPropertySystemPropertyTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        propObj.AddProperty(PropertyFactory.StringProperty("MyString", "foo"));
        propObj.SetPropertyValue("MyString", "bar");
        Console.WriteLine((string)propObj.GetPropertyValue("MyString"));
    }

    #region Types of Properties

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_05121_FeaturesPropertySystemNumericalPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        var intProp = CoreObjectsFactory.CreateIntPropertyBuilder("Integer", 10);
        intProp.MinValue = 0;
        intProp.MaxValue = 15;
        propObj.AddProperty(intProp.Build());

        var suggestedValues = CoreTypesFactory.CreateList<FloatObject>(1.23, 3.21, 5.67);
        var floatProp = CoreObjectsFactory.CreateFloatPropertyBuilder("Float", 3.21);
        floatProp.SuggestedValues = suggestedValues.CastList();

        propObj.AddProperty(floatProp.Build());

        // "Integer" is set to 15 due to the max value
        propObj.SetPropertyValue("Integer", 20);
        Console.WriteLine((long)propObj.GetPropertyValue("Integer"));

        // "Float" is set to 2.34 despite the suggested values not containing the value 2.34
        propObj.SetPropertyValue("Float", 2.34);
        Console.WriteLine((double)propObj.GetPropertyValue("Float"));
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_05122_FeaturesPropertySystemSelectionPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        var list = CoreTypesFactory.CreateList<StringObject>("Apple", "Banana", "Kiwi");

        propObj.AddProperty(PropertyFactory.SelectionProperty("ListSelection", list.CastList(), 0));

        var dict = CoreTypesFactory.CreateDict<IntegerObject, StringObject>((0, "foo"), (10, "bar"));
        propObj.AddProperty(PropertyFactory.SparseSelectionProperty("DictSelection", dict.CastDict(), 10));

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
    [Category(SKIP_SETUP)]
    public void Test_05123_FeaturesPropertySystemObjectPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        var child1 = CoreObjectsFactory.CreatePropertyObject();
        var child2 = CoreObjectsFactory.CreatePropertyObject();

        // The order below is important, as "child1" and "child2" are frozen once
        // used as default property values.
        child2.AddProperty(PropertyFactory.StringProperty("String", "foo"));
        child1.AddProperty(PropertyFactory.ObjectProperty("Child", child2));
        propObj.AddProperty(PropertyFactory.ObjectProperty("Child", child1));

        // Prints out the value of the "String" Property of child2
        Console.WriteLine((string)propObj.GetPropertyValue("Child.Child.String"));
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_05124_FeaturesPropertySystemContainerPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        var list = CoreTypesFactory.CreateList<StringObject>("Banana", "Apple", "Kiwi");

        propObj.AddProperty(PropertyFactory.ListProperty("List", list.CastList()));

        var dict = CoreTypesFactory.CreateDict<IntegerObject, StringObject>((0, "foo"), (10, "bar"));
        propObj.AddProperty(PropertyFactory.DictProperty("Dict", dict.CastDict()));

        // Prints out "Banana"
        Console.WriteLine((string)propObj.GetPropertyValue("List").CastList<StringObject>()[0]);
        // Prints out "bar"
        Console.WriteLine((string)propObj.GetPropertyValue("Dict").CastDict<IntegerObject, StringObject>()[10]);

        var list2 = CoreTypesFactory.CreateList<StringObject>("Pear", "Strawberry");

        // Sets a new value for the List Property
        propObj.SetPropertyValue("List", (BaseObject)list2);
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_05125_FeaturesPropertySystemReferencePropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        propObj.AddProperty(PropertyFactory.IntProperty("Integer", 0));
        propObj.AddProperty(PropertyFactory.StringProperty("Prop1", "foo"));
        propObj.AddProperty(PropertyFactory.StringProperty("Prop2", "bar"));

        propObj.AddProperty(PropertyFactory.ReferenceProperty("RefProp", CoreObjectsFactory.CreateEvalValue("switch($Integer, 0, %Prop1, 1, %Prop2)")));

        // Prints "foo"
        Console.WriteLine((string)propObj.GetPropertyValue("RefProp"));

        propObj.SetPropertyValue("Integer", 1);

        // Prints "bar"
        Console.WriteLine((string)propObj.GetPropertyValue("RefProp"));
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_05126_FeaturesPropertySystemProcedurePropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        var arguments = CoreTypesFactory.CreateList(CoreObjectsFactory.CreateArgumentInfo("Val1", CoreType.ctInt),
                                                    CoreObjectsFactory.CreateArgumentInfo("Val2", CoreType.ctInt));

        propObj.AddProperty(PropertyFactory.FunctionProperty("SumFunction",
                                                             CoreObjectsFactory.CreateCallableInfo(arguments.CastList(),
                                                                                                   CoreType.ctInt,
                                                                                                   constFlag: true)));

        var func = CoreTypesFactory.CreateFunction(SumLongs);
        propObj.SetPropertyValue("SumFunction", func);

        Function sumFunc = propObj.GetPropertyValue("SumFunction").Cast<Function>();

        // Prints out 42
        Console.WriteLine((long)sumFunc.Call(((ListObject<IntegerObject>)CoreTypesFactory.CreateList<IntegerObject>(12, 30))
                                                                                         .Cast<BaseObject>()));

        //implement a FuncCallDelegate function
        ErrorCode SumLongs(BaseObject? parameters, out BaseObject? result)
        {
            //initialize output
            result = null;

            //expecting two long parameters
            using var paramsList = parameters?.CastList<IntegerObject>();
            if ((paramsList == null) || (paramsList.Count != 2))
            {
                Console.WriteLine($"-> 'parameters' is not a 'ListObject<IntegerObject>' with 2 entries");

                //tell API that it was not OK
                return CoreTypesFactory.MakeErrorInfo(ErrorCode.OPENDAQ_ERR_INVALIDPARAMETER);
            }

            //returning the sum of the given integer parameters as result
            result = (long)paramsList[0] + (long)paramsList[1];

            //tell API that everything was OK
            return ErrorCode.OPENDAQ_SUCCESS;
        }
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_05127_FeaturesPropertySystemRemainingPropertyTypesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        propObj.AddProperty(PropertyFactory.StringProperty("String", "foo"));
        propObj.AddProperty(PropertyFactory.RatioProperty("Ratio", CoreTypesFactory.CreateRatio(1, 10)));
        propObj.AddProperty(PropertyFactory.BoolProperty("Bool", true));
    }

    #endregion Types of Properties

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0513_FeaturesPropertySystemCreatingAndConfiguringAPropertyTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        PropertyBuilder floatProp = CoreObjectsFactory.CreateFloatPropertyBuilder("MyFloat", 1.123);
        floatProp.MinValue = 0.0;
        floatProp.MaxValue = 10.0;

        propObj.AddProperty(floatProp.Build());
    }

    #region Properties and EvalValue

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_05141_FeaturesPropertySystemPropertiesAndEvalValueTest()
    {
#if !conversion_of_EvalValue_possible
        /*
            [NOTE]
            The `EvalValue` usage for `PropertyBuilder` settings of the C++ example is not supported in the current version of the .NET Bindings. +
            E.g. this is not possible (conversion of `EvalValue` to `Unit`): +
            `amplitudeProp.Unit = CoreObjectsFactory.CreateEvalValue("Unit(%AmplitudeUnit:SelectedValue)");`
        */
#else
        PropertyObject simulatedChannel = CoreObjectsFactory.CreatePropertyObject();
        simulatedChannel.AddProperty(PropertyFactory.SelectionProperty("Waveform", CoreTypesFactory.CreateList<StringObject>("Sine", "Counter").CastList(), 0));
        simulatedChannel.AddProperty(PropertyFactory.ReferenceProperty("Settings", CoreObjectsFactory.CreateEvalValue("if($Waveform == 0, %SineSettings, %CounterSettings)")));

        PropertyBuilder freqProp = CoreObjectsFactory.CreateFloatPropertyBuilder("Frequency", 10.0);
        freqProp.Unit = CoreObjectsFactory.CreateUnit(-1, "Hz", "", "");
        freqProp.MinValue = 0.1;
        freqProp.MaxValue = 1000.0;
        freqProp.SuggestedValues = CoreTypesFactory.CreateList<FloatObject>(0.1, 10.0, 100.0, 1000.0).CastList();

        simulatedChannel.AddProperty(freqProp.Build());

        // Sine settings

        PropertyObject sineSettings = CoreObjectsFactory.CreatePropertyObject();

        sineSettings.AddProperty(PropertyFactory.SelectionProperty("AmplitudeUnit", CoreTypesFactory.CreateList<StringObject>("V", "mV").CastList(), 0));

        PropertyBuilder amplitudeProp = CoreObjectsFactory.CreateFloatPropertyBuilder("Amplitude", 5);
        amplitudeProp.Unit = CoreObjectsFactory.CreateEvalValue("Unit(%AmplitudeUnit:SelectedValue)");          @@@//ToDo: convert EvalValue to Unit
        sineSettings.AddProperty(amplitudeProp.Build());

        sineSettings.AddProperty(PropertyFactory.BoolProperty("EnableScaling", false));

        PropertyBuilder scalingFactor = CoreObjectsFactory.CreateFloatPropertyBuilder("ScalingFactor", 1.0);
        scalingFactor.Visible = CoreObjectsFactory.CreateEvalValue("$EnableScaling");                           @@@//ToDo: convert EvalValue to bool??
        sineSettings.AddProperty(scalingFactor.Build());

        simulatedChannel.AddProperty(PropertyFactory.ObjectProperty("SineSettings", sineSettings));

        // Counter settings

        PropertyObject counterSettings = CoreObjectsFactory.CreatePropertyObject();

        counterSettings.AddProperty(PropertyFactory.IntProperty("Increment", 1));

        counterSettings.AddProperty(PropertyFactory.SelectionProperty("Mode", CoreTypesFactory.CreateList<StringObject>("Infinite", "Loop").CastList(), 0));

        PropertyBuilder loopThreshold = CoreObjectsFactory.CreateIntPropertyBuilder("LoopThreshold", 100);
        loopThreshold.MinValue = 1;
        loopThreshold.Visible = CoreObjectsFactory.CreateEvalValue("$Mode == 1");                               @@@//ToDo: convert EvalValue to bool??
        counterSettings.AddProperty(loopThreshold.Build());

        PropertyBuilder resetProp = CoreObjectsFactory.CreateFunctionPropertyBuilder("Reset", CoreObjectsFactory.CreateCallableInfo(null, CoreType.ctUndefined, constFlag: false));
        resetProp.ReadOnly = true;
        resetProp.Visible = CoreObjectsFactory.CreateEvalValue("$Mode == 0");                                   @@@//ToDo: convert EvalValue to bool??
        counterSettings.AddProperty(resetProp.Build());
        counterSettings.Cast<PropertyObjectProtected>().SetProtectedPropertyValue("Reset", CoreTypesFactory.CreateProcedure(_ => { /* reset counter */ return ErrorCode.OPENDAQ_SUCCESS; }));

        simulatedChannel.AddProperty(PropertyFactory.ObjectProperty("CounterSettings", counterSettings));
#endif
    }

    //[Test]
    [Category(SKIP_SETUP)]
    public void Test_05142_FeaturesPropertySystemReferencingAnotherPropertyTest1()
    {
        #region just for the test to compile
        PropertyObject sineSettings = CoreObjectsFactory.CreatePropertyObject();
        PropertyObject counterSettings = CoreObjectsFactory.CreatePropertyObject();
        #endregion

        PropertyObject simulatedChannel = CoreObjectsFactory.CreatePropertyObject();

        var list = CoreTypesFactory.CreateList<StringObject>("Sine", "Counter");

        simulatedChannel.AddProperty(PropertyFactory.SelectionProperty("Waveform", list.CastList(), 0));
        simulatedChannel.AddProperty(PropertyFactory.ReferenceProperty("Settings", CoreObjectsFactory.CreateEvalValue("if($Waveform == 0, %SineSettings, %CounterSettings)")));

        //...

        simulatedChannel.AddProperty(PropertyFactory.ObjectProperty("SineSettings", sineSettings));

        //...

        simulatedChannel.AddProperty(PropertyFactory.ObjectProperty("CounterSettings", counterSettings));
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_05143_FeaturesPropertySystemReferencingAnotherPropertyTest2()
    {
        // If the value of the "Waveform" Property equals 0, the EvalValue evaluates to the
        // "SineSettings" Property. If not, it evaluates to the "CounterSettings" Property.
        PropertyFactory.ReferenceProperty("Settings", CoreObjectsFactory.CreateEvalValue("if($Waveform == 0, %SineSettings, %CounterSettings)"));
    }

    //[Test]
    [Category(SKIP_SETUP)]
    public void Test_05144_FeaturesPropertySystemReferencingAnotherPropertyTest3()
    {
        /*
            [NOTE]
            The `EvalValue` usage of the C++ example is not supported in the current version of the .NET Bindings. +
            I.e. it is not possible to convert an `EvalValue` to `BoolObject`.
         */

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
    [Category(SKIP_SETUP)]
    public void Test_05145_FeaturesPropertySystemReferencingAnotherPropertyTest4()
    {
        /*
            [NOTE]
            The `EvalValue` usage of the C++ example is not supported in the current version of the .NET Bindings. +
            I.e. it is not possible to convert an `EvalValue` to `Unit`.
         */

        //#region just for the test to compile
        //PropertyObject sineSettings = CoreObjectsFactory.CreatePropertyObject();
        //PropertyBuilder amplitudeProp = CoreObjectsFactory.CreateFloatPropertyBuilder("Amplitude", 5);
        //#endregion

        //var list = CoreTypesFactory.CreateList<StringObject>("V", "mV");

        //sineSettings.AddProperty(PropertyFactory.SelectionProperty("AmplitudeUnit", list.CastList(), 0));

        ////...

        //amplitudeProp.Unit = CoreObjectsFactory.CreateEvalValue("Unit(%AmplitudeUnit:SelectedValue)");
    }

    #endregion Properties and EvalValue

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0515_FeaturesPropertySystemValidationAndCoercionTest()
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
        }, "*** No exception thrown");

        //here always ex != null
        Console.WriteLine($"{ex.GetType().Name} thrown with error code = {ex.ErrorCode}");

        Assert.That(ex.ErrorCode, Is.EqualTo(ErrorCode.OPENDAQ_ERR_VALIDATE_FAILED), "*** Wrong exception error code");
    }

    #endregion Property

    #region Property Object

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0521_FeaturesPropertySystemAddingRemovingPropertiesTest()
    {
        OpenDaqException ex = Assert.Throws<OpenDaqException>(() =>
        {
            var propObj = CoreObjectsFactory.CreatePropertyObject();
            propObj.AddProperty(PropertyFactory.StringProperty("foo", "bar"));
            propObj.RemoveProperty("foo");

            // Throws a not found error
            var fooProp = propObj.GetProperty("foo");
        }, "*** No exception thrown");

        //here always ex != null
        Console.WriteLine($"as expected: {ex.GetType().Name} thrown with error code = {ex.ErrorCode}");

        Assert.That(ex.ErrorCode, Is.EqualTo(ErrorCode.OPENDAQ_ERR_NOTFOUND), "*** Wrong exception error code");
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0522_FeaturesPropertySystemListingPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        propObj.AddProperty(PropertyFactory.StringProperty("String", "foo"));
        propObj.AddProperty(PropertyFactory.IntProperty("Int", 10, visible: false));
        propObj.AddProperty(PropertyFactory.FloatProperty("Float", 15.0));
        propObj.AddProperty(PropertyFactory.ReferenceProperty("FloatRef", CoreObjectsFactory.CreateEvalValue("%Float")));

        // Contains the Properties "String", "Int", "Float", "FloatRef"
        var allProps = propObj.AllProperties;
        // Contains the Properties "String", "FloatRef"
        var visibleProps = propObj.VisibleProperties;

        var order = CoreTypesFactory.CreateList<StringObject>("FloatRef", "Float", "Int", "String");
        propObj.SetPropertyOrder(order);

        // Contains the Properties in the order "FloatRef", "Float", "Int", String"
        var allPropsReverseOrder = propObj.AllProperties;
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0523_FeaturesPropertySystemReadingWritingPropertyValuesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        propObj.AddProperty(PropertyFactory.StringProperty("String", "foo"));

        // Prints "foo"
        Console.WriteLine((string)propObj.GetPropertyValue("String"));
        propObj.SetPropertyValue("String", "bar");
        // Prints "bar"
        Console.WriteLine((string)propObj.GetPropertyValue("String"));
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0524_FeaturesPropertySystemRWNestedPropertyObjectsTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();
        var child1 = CoreObjectsFactory.CreatePropertyObject();
        var child2 = CoreObjectsFactory.CreatePropertyObject();

        child2.AddProperty(PropertyFactory.StringProperty("String", "foo"));
        child1.AddProperty(PropertyFactory.ObjectProperty("Child", child2));
        propObj.AddProperty(PropertyFactory.ObjectProperty("Child", child1));

        propObj.SetPropertyValue("Child.Child.String", "bar");

        // Prints "bar"
        Console.WriteLine((string)propObj.GetPropertyValue("Child.Child.String"));
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0525_FeaturesPropertySystemRWSelectionPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        var list = CoreTypesFactory.CreateList<StringObject>("Banana", "Kiwi");

        propObj.AddProperty(PropertyFactory.SelectionProperty("Selection", list.CastList(), 1));

        // Prints "Kiwi"
        Console.WriteLine((string)propObj.GetPropertySelectionValue("Selection"));
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0526_FeaturesPropertySystemRWListPropertiesTest1()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        var list = CoreTypesFactory.CreateList<StringObject>("Banana", "Kiwi");

        propObj.AddProperty(PropertyFactory.ListProperty("List", list.CastList()));

        // Prints "Banana"
        Console.WriteLine((string)propObj.GetPropertyValue("List[0]"));

        var list2 = CoreTypesFactory.CreateList<StringObject>("Pear", "Strawberry");

        // Sets a new value to the List Property.
        propObj.SetPropertyValue("List", (BaseObject)list2);

        // Prints "[ Pear, Strawberry ]"
        Console.WriteLine(propObj.GetPropertyValue("List").ToString());
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0527_FeaturesPropertySystemRWListPropertiesTest2()
    {
        #region just for the test to compile
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        var list0 = CoreTypesFactory.CreateList<StringObject>("Banana", "Kiwi");

        propObj.AddProperty(PropertyFactory.ListProperty("List", list0.CastList()));
        #endregion

        IListObject<BaseObject> list = propObj.GetPropertyValue("List").CastList<BaseObject>();
        list.Add("Blueberry");
        propObj.SetPropertyValue("List", (BaseObject)list);

        // Prints "[ Banana, Kiwi, Blueberry ]"
        Console.WriteLine(propObj.GetPropertyValue("List").ToString());
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0528_FeaturesPropertySystemRWDictionaryPropertiesTest()
    {
        var propObj = CoreObjectsFactory.CreatePropertyObject();

        var dict0 = CoreTypesFactory.CreateDict<IntegerObject, StringObject>((1, "Banana"), (2, "Kiwi"));
        propObj.AddProperty(PropertyFactory.DictProperty("Dict", dict0.CastDict()));

        IDictObject<IntegerObject, StringObject> dict = propObj.GetPropertyValue("Dict").CastDict<IntegerObject, StringObject>();
        dict[3] = "Blueberry";

        // The "Dict" property now contains {1 : "Banana"}, {2 : "Kiwi"}, {3 : "Blueberry"}
        propObj.SetPropertyValue("Dict", (BaseObject)dict);
    }

    //[Test]
    [Category(SKIP_SETUP)]
    public void Test_0529_FeaturesPropertySystemRWEventsTest()
    {
        /*
            [NOTE]
            Events are not supported in the current version of the .NET Bindings.
         */

        /*
            auto propObj = PropertyObject();
            propObj.addProperty(IntProperty("IntReadCount", 0));

            propObj.addProperty(IntProperty("Int", 10));

            // Coerce the value of "Int" to a maximum of 20.
            propObj.getOnPropertyValueWrite("Int") +=
              [](PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args)
              {
                Int writtenValue = args.getValue();
                if (writtenValue > 20)
                {
                  args.setValue(20);
                }
              };

            // Increment IntReadCount whenever the "Int" Property Value is read.
            propObj.getOnPropertyValueRead("Int") +=
              [](PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args)
              {
                IntegerPtr readCount = sender.getPropertyValue("IntReadCount");
                sender.setPropertyValue("IntReadCount", readCount + 1);
              };


            propObj.setPropertyValue("Int", 30);
            // Prints out 20
            std::cout << propObj.getPropertyValue("Int") << std::endl;
            // Prints out 1
            std::cout << propObj.getPropertyValue("IntReadCount") << std::endl;
         */
    }

    #endregion Property Object

    #region Property Object Class

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0531_FeaturesPropertySystemPropertyObjectClassTest()
    {
        var list = CoreTypesFactory.CreateList<StringObject>("Banana", "Apple", "Kiwi");

        PropertyObjectClassBuilder propClass = CoreObjectsFactory.CreatePropertyObjectClassBuilder("MyClass");
        propClass.AddProperty(PropertyFactory.IntProperty("Integer", 10));
        propClass.AddProperty(PropertyFactory.SelectionProperty("Selection", list.CastList(), 1));
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0532_FeaturesPropertySystemPOCManageTest()
    {
        TypeManager manager = CoreTypesFactory.CreateTypeManager();

        var list = CoreTypesFactory.CreateList<StringObject>("Banana", "Apple", "Kiwi");

        PropertyObjectClassBuilder propClass = CoreObjectsFactory.CreatePropertyObjectClassBuilder("MyClass");
        propClass.AddProperty(PropertyFactory.IntProperty("Integer", 10));
        propClass.AddProperty(PropertyFactory.SelectionProperty("Selection", list.CastList(), 1));
        manager.AddType(propClass.Build());

        PropertyObject propObj = CoreObjectsFactory.CreatePropertyObjectWithClassAndManager(manager, "MyClass");

        // Prints "Apple"
        Console.WriteLine((string)propObj.GetPropertySelectionValue("Selection"));
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0534_FeaturesPropertySystemPOCClassInheritanceTest()
    {
        TypeManager manager = CoreTypesFactory.CreateTypeManager();

        PropertyObjectClassBuilder propClass1 = CoreObjectsFactory.CreatePropertyObjectClassBuilderWithManager(manager, "InheritedClass");
        propClass1.AddProperty(PropertyFactory.StringProperty("InheritedProp", "foo"));
        manager.AddType(propClass1.Build());

        PropertyObjectClassBuilder propClass2 = CoreObjectsFactory.CreatePropertyObjectClassBuilderWithManager(manager, "MyClass");
        propClass2.AddProperty(PropertyFactory.StringProperty("OwnProp", "bar"));
        propClass2.ParentName = "InheritedClass";
        manager.AddType(propClass2.Build());

        PropertyObject propObj = CoreObjectsFactory.CreatePropertyObjectWithClassAndManager(manager, "MyClass");

        // Prints "foo"
        Console.WriteLine((string)propObj.GetPropertyValue("InheritedProp"));
        // Prints "bar"
        Console.WriteLine((string)propObj.GetPropertyValue("OwnProp"));
    }

    //[Test]
    [Category(SKIP_SETUP)]
    public void Test_0535_FeaturesPropertySystemPOCPropertyEventsOnClassesTest()
    {
#if !supports_events
        /*
            [NOTE]
            Event handling is not supported in the current version of the .NET Bindings.
         */
#else
        int readCount = 0;

        var intProp = PropertyFactory.IntProperty("ReadCount", 0);
        intProp.OnPropertyValueRead.AddHandler((PropertyObject sender, PropertyValueEventArgs args) => //ToDo: AddHandler() @@@
        {
            readCount++;
            sender.Cast<PropertyObjectProtected>().SetProtectedPropertyValue("ReadCount", readCount + 1);
            args.Value = readCount;
        });

        TypeManager manager = CoreTypesFactory.CreateTypeManager();
        PropertyObjectClassBuilder propClass = CoreObjectsFactory.CreatePropertyObjectClassBuilderWithManager(manager, "MyClass");
        propClass.AddProperty(intProp);

        manager.AddClass(propClass.Build()); //ToDo: AddClass() not available @@@

        var propObj1 = CoreObjectsFactory.CreatePropertyObjectWithClassAndManager(manager, "MyClass");
        var propObj2 = CoreObjectsFactory.CreatePropertyObjectWithClassAndManager(manager, "MyClass");

        // Prints out 1
        Console.WriteLine((long)propObj1.GetPropertyValue("ReadCount"));
        // Prints out 2
        Console.WriteLine((long)propObj2.GetPropertyValue("ReadCount"));
#endif
    }

    #endregion Property Object Class

    #endregion Property system

    #region Readers ./Antora/modules/explanations/pages/readers.adoc

    [Test]
    public void Test_0601_ConstructingAReaderTest()
    {
        // These calls all create the same Reader (e.g. for a Stream Reader)
        var reader1 = OpenDAQFactory.CreateStreamReader(signal);
        var reader2 = OpenDAQFactory.CreateStreamReader<double, long>(signal);
        // right now there exists no factory function using `SampleType` as parameters
    }

    #endregion Readers ./Antora/modules/explanations/pages/readers.adoc

    #region Multi Reader ./Antora/modules/explanations/pages/multireader_spec.adoc

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0701_MultiReaderExampleTest()
    {
        var instance  = OpenDAQFactory.Instance();
        var refDevice = instance.AddDevice("daqref://device0");
        var signals   = refDevice.GetSignalsRecursive();

        // Create reader that converts values to `double` and time data to `int64`
        var multiReader = OpenDAQFactory.CreateMultiReader<double, long>(signals);

        // Allocate buffers for each signal
        int   signalsCount = signals.Count;
        nuint kBufferSize  = 0;

        var dataBuffers   = new double[signalsCount][];
        var domainBuffers = new long[signalsCount][];

        // read data every 50ms, up to a maximum of kBufferSize samples
        for (int readCount = 0; readCount < 20; readCount++)
        {
            var dataAvailable = multiReader.AvailableCount;
            var count         = Math.Min(kBufferSize, dataAvailable);
            var status        = multiReader.ReadWithDomain(dataBuffers, domainBuffers, ref count);

            if (status.ReadStatus == ReadStatus.Event)
            {
                // Set buffer size based on sample rate, allocate buffers
                // Buffers have 100ms worth of memory for each signal
                //ToDo: reader::getSampleRate() not available in .NET Bindings
                //var sampleRate = reader::getSampleRate(
                //    status.MainDescriptor.Parameters["DomainDataDescriptor"]);
                // simplified for the example:
                var sampleRate = GetSampleRate(status.MainDescriptor
                                                     .Parameters["DomainDataDescriptor"]
                                                     .Cast<DataDescriptor>());

                kBufferSize = sampleRate / 10;

                for (int i = 0; i < signalsCount; ++i)
                {
                    dataBuffers[i]   = new double[kBufferSize];
                    domainBuffers[i] = new long[kBufferSize];
                }
            }
            else if ((status.ReadStatus == ReadStatus.Ok) && (count > 0))
            {
                Console.Write("Data: ");
                foreach (var buf in dataBuffers)
                    Console.Write($"{buf[0]}; ");
                Console.WriteLine();
            }

            System.Threading.Thread.Sleep(50);
        }

        nuint GetSampleRate(DataDescriptor dataDescriptor)
        {
            var resolution = dataDescriptor.TickResolution;
            var delta      = (NumberObject)1;
            var rule       = dataDescriptor.Rule;

            if ((rule != null) && (rule.Type != DataRuleType.Linear))
            {
                throw new NotSupportedException("Only signals with implicit linear-rule as a domain are supported.");
            }
            else if (rule != null)
            {
                delta = rule.Parameters["delta"].Cast<NumberObject>();
            }

            double sampleRate = (double)resolution.Denominator
                              / (double)resolution.Numerator
                              * (double)delta;
            if (sampleRate != (double)(ulong)sampleRate)
            {
                throw new NotSupportedException($"Only signals with integral sample-rate are supported but found signal with {sampleRate} Hz");
            }

            return (nuint)Convert.ToUInt64(sampleRate);
        }
    }

    //[Test]
    [Category(SKIP_SETUP)]
    public void Test_0702_MultiReaderReusingDomainDataTest()
    {
        #region just for the test to compile
        //OpenDAQFactory.CreateMultiReaderStatus(EventPacket mainDescriptor, IDictObject < BaseObject, BaseObject > eventPackets, bool valid, NumberObject offset);
        MultiReaderStatus status  = OpenDAQFactory.CreateMultiReaderStatus(mainDescriptor: null, eventPackets: null, valid: false, offset: 0);
        Context           context = null;
        Component         parent  = null;
        #endregion

        var eventPacket            = status.MainDescriptor;
        var outputDomainDescriptor = eventPacket.Parameters["DomainDataDescriptor"].Cast<DataDescriptor>();
        var outputDomainSignal     = OpenDAQFactory.CreateSignalWithDescriptor(context, outputDomainDescriptor, parent, "outputDomainSignal", "Signal");
    }

    //[Test]
    [Category(SKIP_SETUP)]
    public void Test_0703_MultiReaderReusingDomainDataTest()
    {
        #region just for the test to compile
        //OpenDAQFactory.CreateMultiReaderStatus(EventPacket mainDescriptor, IDictObject < BaseObject, BaseObject > eventPackets, bool valid, NumberObject offset);
        MultiReaderStatus status  = OpenDAQFactory.CreateMultiReaderStatus(mainDescriptor: null, eventPackets: null, valid: false, offset: 0);
        Context           context = null;
        Component         parent  = null;

        var eventPacket = status.MainDescriptor;
        var outputDomainDescriptor = eventPacket.Parameters["DomainDataDescriptor"].Cast<DataDescriptor>();
        var outputDomainSignal = OpenDAQFactory.CreateSignalWithDescriptor(context, outputDomainDescriptor, parent, "outputDomainSignal", "Signal");

        nuint count = 1;
        #endregion

        // `count` corresponds to the amount of samples read
        var outputDomainPacket = OpenDAQFactory.CreateDataPacket(outputDomainDescriptor, count, status.Offset);
        outputDomainSignal.SendPacket(outputDomainPacket);
    }

    //[Test]
    [Category(SKIP_SETUP)]
    public void Test_0704_MultiReaderDifferentSampleRatesTest()
    {
        #region just for the test to compile
        var instance  = OpenDAQFactory.Instance();
        var refDevice = instance.AddDevice("daqref://device0");
        var signals   = refDevice.GetSignalsRecursive();

        // Create reader that converts values to `double` and time data to `int64`
        var multiReader = OpenDAQFactory.CreateMultiReader<double, long>(signals);

        // Allocate buffers for each signal
        int   signalsCount = signals.Count;
        nuint kBufferSize  = 0;

        var dataBuffers   = new double[signalsCount][];
        var domainBuffers = new long[signalsCount][];

        // read data every 50ms, up to a maximum of kBufferSize samples
        for (int readCount = 0; readCount < 20; readCount++)
        {
        #endregion

            List<long> dividers = new();

            var dataAvailable = multiReader.AvailableCount;
            var count = Math.Min(kBufferSize, dataAvailable);
            var status = multiReader.ReadWithDomain(dataBuffers, domainBuffers, ref count);

            if (status.ReadStatus == ReadStatus.Event)
            {
                var packets = status.EventPackets;
                if (!(packets.Values.First().EventId == "DATA_DESCRIPTOR_CHANGED"))
                    continue;

                // SRDiv calculation
                long commonSampleRate = multiReader.CommonSampleRate;
                dividers.Clear();
                Console.Write("Dividers: ");
                foreach (var eventPacket in packets.Values)
                {
                    var descriptor = eventPacket.Parameters["DomainDataDescriptor"].Cast<DataDescriptor>();
                    var sampleRate = GetSampleRate(descriptor);
                    dividers.Add(commonSampleRate / (long)sampleRate);
                    Console.Write($"{dividers.Last()}, ");
                }
                Console.WriteLine();

                // Allocate buffers for 100ms according to commonSampleRate
                long lcm = 1;
                foreach (var div in dividers)
                    lcm = GetLeastCommonMultiple(lcm, div); // https://stackoverflow.com/a/20824923

                // Calculate k as the minimum number of LCM-size blocks to read ~100ms of data
                long k = Math.Max(commonSampleRate / lcm / 10, 1);
                kBufferSize = (nuint)(k * lcm);

                Console.Write("Buffer sizes: ");
                for (int i = 0; i < signalsCount; ++i)
                {
                    dataBuffers[i]   = new double[(long)kBufferSize / dividers[i]];
                    domainBuffers[i] = new long[(long)kBufferSize / dividers[i]];
                    Console.Write($"{(long)kBufferSize / dividers[i]}, ");
                }
                Console.WriteLine();
            }

        #region just for the test to compile
        }

        nuint GetSampleRate(DataDescriptor dataDescriptor)
        {
            var resolution = dataDescriptor.TickResolution;
            var delta      = (NumberObject)1;
            var rule       = dataDescriptor.Rule;

            if ((rule != null) && (rule.Type != DataRuleType.Linear))
            {
                throw new NotSupportedException("Only signals with implicit linear-rule as a domain are supported.");
            }
            else if (rule != null)
            {
                delta = rule.Parameters["delta"].Cast<NumberObject>();
            }

            double sampleRate = (double)resolution.Denominator
                              / (double)resolution.Numerator
                              * (double)delta;
            if (sampleRate != (double)(ulong)sampleRate)
            {
                throw new NotSupportedException($"Only signals with integral sample-rate are supported but found signal with {sampleRate} Hz");
            }

            return (nuint)Convert.ToUInt64(sampleRate);
        }

        static long GetLeastCommonMultiple(long a, long b)
        {
            return (a / gcf(a, b)) * b;

            static long gcf(long a, long b)
            {
                while (b != 0)
                {
                    long temp = b;
                    b = a % b;
                    a = temp;
                }
                return a;
            }
        }
        #endregion
    }

    #endregion Multi Reader ./Antora/modules/explanations/pages/multireader_spec.adoc


    //[Test]
    //[Category(SKIP_SETUP)]
    //public void Test_aabb_Test()
    //{

    //}
}
