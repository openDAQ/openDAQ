//#define USE_LISTOBJECT
//#define USE_ITERATOR_NOT_IENUMERABLE

// Ignore Spelling: Opc Ua nullable daqref


using System.Collections;
using System.Diagnostics;

using Daq.Core.Objects;
using Daq.Core.OpenDAQ;
using Daq.Core.Types;


namespace openDaq.Net.Test;


public class OpenDaqHowToGuidesTests : CoreTypesTestsBase
{
    public const string SKIP_SETUP        = "SkipSetup";
    public const string SKIP_SETUP_DEVICE = "SkipSetupDevice";

    private const string MODULE_PATH = ".";

    private const string ConnectionProtocolDaqRef  = "daqref://";
    private const string ConnectionProtocolOpcUa   = "daq.opcua://";
    private const string ConnectionProtocolWinSock = "daq.ws://";

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

        if (doLog) Console.WriteLine("daqInstance.GetAvailableDevices()");
        var availableDevicesInfos = daqInstance.GetAvailableDevices();
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
                var deviceName             = deviceInfo.GetName();
                var deviceConnectionString = deviceInfo.GetConnectionString();

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

        var deviceNameStr = addedDevice.GetName();
        Console.WriteLine($"- Device to test: '{deviceNameStr}'");

        return addedDevice;
    }



    #region Connect to a device

    [Test]
    [Category(SKIP_SETUP)]
    public void ConnectingGetAvailableDevicesTest()
    {
        // Create an openDAQ(TM) instance, loading modules from the current directory
        Instance instance = OpenDAQFactory.Instance(MODULE_PATH);

        // Discover and print the names and connection strings of openDAQ(TM) devices
        IListObject<DeviceInfo> availableDevicesInfo = instance.GetAvailableDevices();
        foreach (var deviceInfo in availableDevicesInfo)
            if (deviceInfo.GetConnectionString().StartsWith("daq.opcua://"))
                Console.WriteLine($"Name: {deviceInfo.GetName()}, Address: {deviceInfo.GetConnectionString()}");
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void ConnectingOpcUaDevicesTest()
    {
        // Create an openDAQ(TM) instance, loading modules from the current directory
        Instance instance = OpenDAQFactory.Instance(MODULE_PATH);

        IListObject<Device> devices = CoreTypesFactory.CreateList<Device>();
        // Discover and connect to all openDAQ(TM) devices
        foreach (var deviceInfo in instance.GetAvailableDevices())
            if (deviceInfo.GetConnectionString().StartsWith("daq.opcua://"))
                devices.Add(instance.AddDevice(deviceInfo.GetConnectionString()));

        //Hack: dispose device because otherwise we get exception in native code when GC tries to clean up
        foreach (var device in devices)
            device.Dispose();
        devices.Clear();
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void ConnectingOtherDevicesTest()
    {
        // Create an openDAQ(TM) instance, loading modules from the current directory
        Instance instance = OpenDAQFactory.Instance(MODULE_PATH);

        IListObject<Device> devices = CoreTypesFactory.CreateList<Device>();
        // Discover and connect to all openDAQ(TM) reference devices
        foreach (var deviceInfo in instance.GetAvailableDevices())
            if (deviceInfo.GetConnectionString().StartsWith("daqref://"))
                devices.Add(instance.AddDevice(deviceInfo.GetConnectionString()));
    }

    #endregion Connect to a device

    #region Configure a device

    #endregion Configure a device

    #region Add function block

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void FunctionBlockAddingGetAvailableTypesTest()
    {
        // get available function block types
        IDictionary<StringObject, FunctionBlockType> functionBlockTypes = instance.GetAvailableFunctionBlockTypes();
        foreach (string functionBlockTypeName in functionBlockTypes.Keys)
            Console.WriteLine(functionBlockTypeName);
    }

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void FunctionBlockAddingAddTest()
    {
        // add function block on the host computer
        FunctionBlock functionBlock = instance.AddFunctionBlock("ref_fb_module_statistics");
    }

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void FunctionBlockAddingGetTypeInformationTest()
    {
        FunctionBlock functionBlock = instance.AddFunctionBlock("ref_fb_module_statistics");

        FunctionBlockType functionBlockType = functionBlock.GetFunctionBlockType();
        Console.WriteLine(functionBlockType.GetId());
        Console.WriteLine(functionBlockType.GetName());
        Console.WriteLine(functionBlockType.GetDescription());
    }

    [Test(ExpectedResult = 0)]
    [Category(SKIP_SETUP)]
    public int FunctionBlockAdding_FullListingTest()
    {
        // Create an openDAQ(TM) instance, loading modules from the current directory
        Instance instance = OpenDAQFactory.Instance(MODULE_PATH);

        // add simulated device
        Device device = instance.AddDevice("daqref://device0");

        // get available function block types
        IDictObject<StringObject, FunctionBlockType> functionBlockTypes = instance.GetAvailableFunctionBlockTypes();
        foreach (string functionBlockTypeName in functionBlockTypes.Keys)
            Console.WriteLine(functionBlockTypeName);

        // if there is no statistics function block available, exit with an error
        if (!functionBlockTypes.ContainsKey("ref_fb_module_statistics"))
            return 1;

        // add function block on the host computer
        FunctionBlock functionBlock = instance.AddFunctionBlock("ref_fb_module_statistics");

        // print function block type info
        FunctionBlockType functionBlockType = functionBlock.GetFunctionBlockType();
        Console.WriteLine(functionBlockType.GetId());
        Console.WriteLine(functionBlockType.GetName());
        Console.WriteLine(functionBlockType.GetDescription());

        return 0;
    }

    #endregion Add function block

    #region Configure function block

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void FunctionBlockConfigureGetVisiblePropertiesTest()
    {
        FunctionBlock functionBlock = instance.AddFunctionBlock("ref_fb_module_statistics");

        IListObject<Property> functionBlockProperties = functionBlock.GetVisibleProperties();
        foreach (var prop in functionBlockProperties)
            Console.WriteLine(prop.GetName());
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
        // read data from the signal
        // ...
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void FunctionBlockConfigure_FullListingTest()
    {
        // Create an openDAQ(TM) instance, loading modules from the current directory
        Instance instance = OpenDAQFactory.Instance(MODULE_PATH);

        // add simulated device
        Device device = instance.AddDevice("daqref://device0");

        // add function block on the host computer
        FunctionBlock functionBlock = instance.AddFunctionBlock("ref_fb_module_statistics");

        // list properties of the function block
        IListObject<Property> functionBlockProperties = functionBlock.GetVisibleProperties();
        foreach (var prop in functionBlockProperties)
            Console.WriteLine(prop.GetName());

        // print current block size
        long currentBlockSize = functionBlock.GetPropertyValue("BlockSize");
        Console.WriteLine($"Current block size is {currentBlockSize}");

        // configure the properties of the function block
        functionBlock.SetPropertyValue("BlockSize", 100);

        // connect the first signal of the first channel from the device to the first input port on the function block
        functionBlock.GetInputPorts()[0].Connect(device.GetChannels()[0].GetSignals()[0]);

        // get the output signal of the function block
        Signal outputSignal = functionBlock.GetSignals()[0];

        Console.WriteLine(outputSignal.GetDescriptor().GetName());
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
        var available = reader.GetAvailableCount();

        //
        // Signal produces 8 samples
        //

        // Should return 8
        available = reader.GetAvailableCount();

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

    [Test]
    public void ConfigurationSaveTest()
    {
        // save configuration to string
        string jsonStr = instance.SaveConfiguration();

        // write configuration string to file
        File.WriteAllText("config.json", jsonStr, System.Text.Encoding.UTF8);
    }

    [Test]
    public void ConfigurationLoadTest()
    {
        // read configuration from file
        string jsonStr = File.ReadAllText("config.json", System.Text.Encoding.UTF8);

        // load configuration from string
        instance.LoadConfiguration(jsonStr);
    }

    #endregion Save and load configuration

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void _InternalConnectionTest()
    {
        var device = ConnectFirstAvailableDevice(instance, eDesiredConnection.OpcUa, doLog: true);
        Console.WriteLine($"Connected device: {device.GetName()}");

        //Hack: dispose device because otherwise we get exception in native code when GC tries to clean up
        device.Dispose();
    }

    //template snippet - clone these lines and rename the method for a new test
    [Test]
    public void _EmptyTest()
    {
    }
}
