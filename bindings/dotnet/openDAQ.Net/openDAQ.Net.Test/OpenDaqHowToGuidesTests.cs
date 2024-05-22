//#define USE_LISTOBJECT
//#define USE_ITERATOR_NOT_IENUMERABLE

// Ignore Spelling: Opc Ua nullable daqref


using System.Collections;
using System.Diagnostics;

using Daq.Core.Objects;
using Daq.Core.OpenDAQ;
using Daq.Core.Types;


namespace openDaq.Net.Test;


public class OpenDaqHowToGuidesTests : OpenDAQTestsBase
{
    public const string SKIP_SETUP        = "SkipSetup";
    public const string SKIP_SETUP_DEVICE = "SkipSetupDevice";

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
        LocalHost
    }


#pragma warning disable CS8625 // Cannot convert null literal to non-nullable reference type.
    private Instance instance = null;
    private Device   device   = null;
#pragma warning restore CS8625 // Cannot convert null literal to non-nullable reference type.



    [SetUp]
    public void _SetUp()
    {
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

            //device = instance.AddDevice(ConnectionStringDaqRef);
            device = ConnectFirstAvailableDevice(instance, eDesiredConnection.Any);
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
        //Console.WriteLine("<<< TearDown()");
        if (instance == null)
        {
            Console.WriteLine("* skipping TearDown()");
            return;
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

        bool doDaqRef    = (desiredConnection == eDesiredConnection.Any) || (desiredConnection == eDesiredConnection.DaqRef);
        bool doOpcUa     = (desiredConnection == eDesiredConnection.Any) || (desiredConnection == eDesiredConnection.OpcUa);
        bool doWinSock   = (desiredConnection == eDesiredConnection.Any) || (desiredConnection == eDesiredConnection.WinSock);
        bool doLocalHost = (desiredConnection == eDesiredConnection.Any) || (desiredConnection == eDesiredConnection.LocalHost);

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
                if (deviceConnectionString.StartsWith("daq.opcua://172."))
                    continue;

                //connectible device?
                if ((doOpcUa && deviceConnectionString.StartsWith(ConnectionProtocolOpcUa, StringComparison.InvariantCultureIgnoreCase))
                    || (doWinSock && deviceConnectionString.StartsWith(ConnectionProtocolWinSock, StringComparison.InvariantCultureIgnoreCase))
                    || (doDaqRef && deviceConnectionString.StartsWith(ConnectionProtocolDaqRef, StringComparison.InvariantCultureIgnoreCase)))
                {
                    connectionString = deviceConnectionString;
                    break;
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


    // Corresponding document: Antora/modules/howto_guides/pages/howto_connect_to_device.adoc
    
    #region Connect to a device

    [Test]
    [Category(SKIP_SETUP)]
    public void ConnectingAvailableDevicesTest()
    {
        // Create an openDAQ(TM) Instance
        Instance instance = OpenDAQFactory.Instance();

        // Discover and print the names and connection strings of openDAQ(TM) OPC UA Devices
        IListObject<DeviceInfo> availableDevicesInfo = instance.AvailableDevices;
        foreach (var deviceInfo in availableDevicesInfo)
            foreach (var capability in deviceInfo.ServerCapabilities)
                if (capability.ProtocolName == "openDAQ OpcUa")
                    Console.WriteLine($"Name: {deviceInfo.Name}, Address: {capability.ConnectionString}");
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void ConnectingOpcUaDevicesTest()
    {
        // Create an openDAQ(TM) Instance
        Instance instance = OpenDAQFactory.Instance();

        // Discover and connect to all openDAQ(TM) OPC UA Devices
        IListObject<Device> devices = CoreTypesFactory.CreateList<Device>();
        IListObject<DeviceInfo> availableDevicesInfo = instance.AvailableDevices;
        foreach (var deviceInfo in availableDevicesInfo)
            foreach (var capability in deviceInfo.ServerCapabilities)
                if (capability.ProtocolName == "openDAQ OpcUa")
                    devices.Add(instance.AddDevice(capability.ConnectionString));

        //Hack: dispose device because otherwise we get exception in native code when GC tries to clean up
        foreach (var device in devices)
            device.Dispose();
        devices.Clear();
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void ConnectingOtherDevicesTest()
    {
        // Create an openDAQ(TM) Instance
        Instance instance = OpenDAQFactory.Instance();

        // Discover and connect to all openDAQ(TM) reference Devices
        IListObject<Device> devices = CoreTypesFactory.CreateList<Device>();
        foreach (var deviceInfo in instance.AvailableDevices)
            if (deviceInfo.ConnectionString.StartsWith("daqref://"))
                devices.Add(instance.AddDevice(deviceInfo.ConnectionString));
    }

    #endregion Connect to a device

    #region Configure a device

    #endregion Configure a device

    #region Add function block

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void FunctionBlockAddingGetAvailableTypesTest()
    {
        // Get available Function Block types
        IDictionary<StringObject, FunctionBlockType> functionBlockTypes = instance.AvailableFunctionBlockTypes;
        foreach (string functionBlockTypeName in functionBlockTypes.Keys)
            Console.WriteLine(functionBlockTypeName);
    }

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void FunctionBlockAddingAddTest()
    {
        // Add Function Block on the host computer
        FunctionBlock functionBlock = instance.AddFunctionBlock("ref_fb_module_statistics");
    }

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void FunctionBlockAddingGetTypeInformationTest()
    {
        FunctionBlock functionBlock = instance.AddFunctionBlock("ref_fb_module_statistics");

        FunctionBlockType functionBlockType = functionBlock.FunctionBlockType;
        Console.WriteLine(functionBlockType.Id);
        Console.WriteLine(functionBlockType.Name);
        Console.WriteLine(functionBlockType.Description);
    }

    // Corresponding document: Antora/modules/howto_guides/pages/howto_add_function_block.adoc
    [Test(ExpectedResult = 0)]
    [Category(SKIP_SETUP)]
    public int FunctionBlockAdding_FullListingTest()
    {
        // Create an openDAQ(TM) Instance, loading modules from the current directory
        Instance instance = OpenDAQFactory.Instance(MODULE_PATH);

        // Add simulated device
        Device device = instance.AddDevice("daqref://device0");

        // Get available Function Block types
        IDictObject<StringObject, FunctionBlockType> functionBlockTypes = instance.AvailableFunctionBlockTypes;
        foreach (string functionBlockTypeName in functionBlockTypes.Keys)
            Console.WriteLine(functionBlockTypeName);

        // If there is no Statistics Function Block available, exit with an error
        if (!functionBlockTypes.ContainsKey("ref_fb_module_statistics"))
            return 1;

        // Add Function Block on the host computer
        FunctionBlock functionBlock = instance.AddFunctionBlock("ref_fb_module_statistics");

        // Print Function Block type info
        FunctionBlockType functionBlockType = functionBlock.FunctionBlockType;
        Console.WriteLine(functionBlockType.Id);
        Console.WriteLine(functionBlockType.Name);
        Console.WriteLine(functionBlockType.Description);

        return 0;
    }

    #endregion Add function block

    #region Configure function block

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void FunctionBlockConfigureGetVisiblePropertiesTest()
    {
        FunctionBlock functionBlock = instance.AddFunctionBlock("ref_fb_module_statistics");

        IListObject<Property> functionBlockProperties = functionBlock.VisibleProperties;
        foreach (var prop in functionBlockProperties)
            Console.WriteLine(prop.Name);
    }

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void FunctionBlockConfigureGetAndSetPropertyValueTest()
    {
        FunctionBlock functionBlock = instance.AddFunctionBlock("ref_fb_module_statistics");

        long currentBlockSize = functionBlock.GetPropertyValue("BlockSize");
        Console.WriteLine($"Current block size is {currentBlockSize}");
        functionBlock.SetPropertyValue("BlockSize", 100);
    }

    [Test]
    public void FunctionBlockConfigureConnectInputPortsTest()
    {
        FunctionBlock functionBlock = instance.AddFunctionBlock("ref_fb_module_statistics");

        functionBlock.GetInputPorts()[0].Connect(device.GetChannels()[0].GetSignals()[0]);
        Signal outputSignal = functionBlock.GetSignals()[0];
        // Read data from the first Signal of the Function Block
        // ...
    }

  // Corresponding document: Antora/modules/howto_guides/pages/howto_configure_function_block.adoc
    [Test]
    [Category(SKIP_SETUP)]
    public void FunctionBlockConfigure_FullListingTest()
    {
        // Create an openDAQ(TM) Instance, loading modules from the current directory
        Instance instance = OpenDAQFactory.Instance(MODULE_PATH);

        // Add simulated device
        Device device = instance.AddDevice("daqref://device0");

        // Add Function Block on the host computer
        FunctionBlock functionBlock = instance.AddFunctionBlock("ref_fb_module_statistics");

        // List properties of the Function Block
        IListObject<Property> functionBlockProperties = functionBlock.VisibleProperties;
        foreach (var prop in functionBlockProperties)
            Console.WriteLine(prop.Name);

        // Print current block size
        long currentBlockSize = functionBlock.GetPropertyValue("BlockSize");
        Console.WriteLine($"Current block size is {currentBlockSize}");
        // Configure the properties of the Function Block
        functionBlock.SetPropertyValue("BlockSize", 100);

        // Connect the first Signal of the first Channel from the Device to the first Input Port on the Function Block
        functionBlock.GetInputPorts()[0].Connect(device.GetChannels()[0].GetSignals()[0]);
    // Read data from the Signal
        // ...

        // Get the output Signal of the Function Block
        Signal outputSignal = functionBlock.GetSignals()[0];

        Console.WriteLine(outputSignal.Descriptor.Name);
    }

    #endregion Configure function block

    #region Read data with Readers

    #region Basic value and domain

    [Test]
    public void ReaderCreateTest()
    {
        Signal signal = device.GetChannels()[0].GetSignals()[0];

        // These calls all create the same reader
        var reader1 = OpenDAQFactory.CreateStreamReader(signal);
        var reader2 = OpenDAQFactory.CreateStreamReader<double, long>(signal);
    }

    [Test]
    public void ReaderReadingDataTest()
    {
        Signal signal = device.GetChannels()[0].GetSignals()[0];

        var reader = OpenDAQFactory.CreateStreamReader<double, long>(signal);

        // Should return 0
        var available = reader.AvailableCount;

        //
        // Signal produces 8 samples
        //

        // Should return 8
        available = reader.AvailableCount;

        nuint readCount = 5;
        double[] values = new double[readCount];
        reader.Read(values, ref readCount);

        Console.WriteLine($"Read {readCount} values");
        foreach (double value in values)
        {
            Console.WriteLine(value);
        }

        readCount = 5;
        double[] newValues = new double[5];
        long[] newDomain = new long[5];
        reader.ReadWithDomain(newValues, newDomain, ref readCount);

        // `readCount` should now be `3`
        Console.WriteLine($"Read another {readCount} value and domain samples");
        for (nuint i = 0; i < readCount; ++i)
        {
            Console.WriteLine($"{newValues[i]}, {newDomain[i]}");
        }
    }

    //[Test]
    public void ReaderHandlingSignalChangesTest()
    {
        //ToDo: call-backs not yet implemented
    }

    //[Test]
    public void ReaderInvalidationAndReuseTest()
    {
        //ToDo: call-backs not yet implemented
    }

    //[Test]
    public void ReaderInvalidationAndReuseTest2()
    {
        //ToDo: uses createPacketForSignal which is unknown
        //ToDo: uses SignalConfig as signal
    }

    #endregion Basic value and domain

    #endregion Read data with Readers

    #region Save and load configuration

  // Corresponding document: Antora/modules/howto_guides/pages/howto_save_load_configuration.adoc
    [Test]
    public void ConfigurationSaveTest()
    {
        // Save Configuration to string
        string jsonStr = instance.SaveConfiguration();
        // Write Configuration string to file
        File.WriteAllText("openDAQconfig.json", jsonStr, System.Text.Encoding.UTF8);
    }

  // Corresponding document: Antora/modules/howto_guides/pages/howto_save_load_configuration.adoc
    [Test]
    public void ConfigurationLoadTest()
    {
        // Read Configuration from file
        string jsonStr = File.ReadAllText("openDAQconfig.json", System.Text.Encoding.UTF8);
        // Load Configuration from string
        instance.LoadConfiguration(jsonStr);
    }

    #endregion Save and load configuration

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void _InternalConnectionTest()
    {
        var device = ConnectFirstAvailableDevice(instance, eDesiredConnection.OpcUa, doLog: true);
        Console.WriteLine($"Connected device: {device.Name}");

        //Hack: dispose device because otherwise we get exception in native code when GC tries to clean up
        device.Dispose();
    }

    //template snippet - clone these lines and rename the method for a new test
    [Test]
    public void _EmptyTest()
    {
    }
}
