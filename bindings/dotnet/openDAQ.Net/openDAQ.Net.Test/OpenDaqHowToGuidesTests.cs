// Ignore Spelling: Opc Ua nullable daqref


using Daq.Core.Objects;
using Daq.Core.OpenDAQ;
using Daq.Core.Types;

//using static Daq.Core.Objects.CoreObjectsFactory;
//using static Daq.Core.OpenDAQ.OpenDAQFactory;
//using static Daq.Core.Types.CoreTypesFactory;


namespace openDaq.Net.Test;


public class OpenDaqHowToGuidesTests : OpenDAQTestsBase
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
    private Instance instance = null;
    private Device   device   = null;
    private Signal   signal   = null;
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

            using var channels = device.GetChannels();
            if (channels.Count > 0)
            {
                using var signals = channels[0].GetSignals();
                if (signals.Count > 0)
                {
                    signal = signals[0];
                }
            }
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


    #region Connect to a device

    // Corresponding document: Antora/modules/howto_guides/pages/howto_connect_to_device.adoc

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0001_ConnectingAvailableDevicesTest()
    {
        // Create an openDAQ(TM) Instance
        Instance instance = OpenDAQFactory.Instance();

        // Discover and print the names and connection strings of openDAQ(TM) OPC UA Devices
        IListObject<DeviceInfo> availableDevicesInfo = instance.AvailableDevices;
        foreach (var deviceInfo in availableDevicesInfo)
            foreach (var capability in deviceInfo.ServerCapabilities)
                if (capability.ProtocolName == "OpenDAQOPCUA")
                    Console.WriteLine($"Name: {deviceInfo.Name}, Address: {capability.ConnectionString}");
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0002_ConnectingOpcUaDevicesTest()
    {
        // Create an openDAQ(TM) Instance
        Instance instance = OpenDAQFactory.Instance();

        // Discover and connect to all openDAQ(TM) OPC UA Devices
        IListObject<Device> devices = CoreTypesFactory.CreateList<Device>();
        IListObject<DeviceInfo> availableDevicesInfo = instance.AvailableDevices;
        foreach (var deviceInfo in availableDevicesInfo)
            foreach (var capability in deviceInfo.ServerCapabilities)
                if (capability.ProtocolName == "OpenDAQOPCUA")
                    devices.Add(instance.AddDevice(capability.ConnectionString));

        //Hack: dispose device because otherwise we get exception in native code when GC tries to clean up
        foreach (var device in devices)
            device.Dispose();
        devices.Clear();
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0003_ConnectingOtherDevicesTest()
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

    #region Add function block

    // Corresponding document: Antora/modules/howto_guides/pages/howto_add_function_block.adoc

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void Test_0101_FunctionBlockAddingGetAvailableTypesTest()
    {
        // Get available Function Block types
        IDictionary<StringObject, FunctionBlockType> functionBlockTypes = instance.AvailableFunctionBlockTypes;
        foreach (string functionBlockTypeName in functionBlockTypes.Keys)
            Console.WriteLine(functionBlockTypeName);
    }

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void Test_0102_FunctionBlockAddingAddTest()
    {
        // Add Function Block on the host computer
        FunctionBlock functionBlock = instance.AddFunctionBlock("RefFBModuleStatistics");
    }

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void Test_0103_FunctionBlockAddingGetTypeInformationTest()
    {
        FunctionBlock functionBlock = instance.AddFunctionBlock("RefFBModuleStatistics");

        FunctionBlockType functionBlockType = functionBlock.FunctionBlockType;
        Console.WriteLine(functionBlockType.Id);
        Console.WriteLine(functionBlockType.Name);
        Console.WriteLine(functionBlockType.Description);
    }

    [Test(ExpectedResult = 0)]
    [Category(SKIP_SETUP)]
    public int Test_0104_FunctionBlockAdding_FullListingTest()
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
        if (!functionBlockTypes.ContainsKey("RefFBModuleStatistics"))
            return 1;

        // Add Function Block on the host computer
        FunctionBlock functionBlock = instance.AddFunctionBlock("RefFBModuleStatistics");

        // Print Function Block type info
        FunctionBlockType functionBlockType = functionBlock.FunctionBlockType;
        Console.WriteLine(functionBlockType.Id);
        Console.WriteLine(functionBlockType.Name);
        Console.WriteLine(functionBlockType.Description);

        return 0;
    }

    #endregion Add function block

    #region Configure function block

    // Corresponding document: Antora/modules/howto_guides/pages/howto_configure_function_block.adoc

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void Test_0201_FunctionBlockConfigureGetVisiblePropertiesTest()
    {
        FunctionBlock functionBlock = instance.AddFunctionBlock("RefFBModuleStatistics");

        IListObject<Property> functionBlockProperties = functionBlock.VisibleProperties;
        foreach (var prop in functionBlockProperties)
            Console.WriteLine(prop.Name);
    }

    [Test]
    [Category(SKIP_SETUP_DEVICE)]
    public void Test_0202_FunctionBlockConfigureGetAndSetPropertyValueTest()
    {
        FunctionBlock functionBlock = instance.AddFunctionBlock("RefFBModuleStatistics");

        long currentBlockSize = functionBlock.GetPropertyValue("BlockSize");
        Console.WriteLine($"Current block size is {currentBlockSize}");
        functionBlock.SetPropertyValue("BlockSize", 100);
    }

    [Test]
    public void Test_0203_FunctionBlockConfigureConnectInputPortsTest()
    {
        FunctionBlock functionBlock = instance.AddFunctionBlock("RefFBModuleStatistics");

        functionBlock.GetInputPorts()[0].Connect(device.GetChannels()[0].GetSignals()[0]);
        Signal outputSignal = functionBlock.GetSignals()[0];
        // Read data from the first Signal of the Function Block
        // ...
    }

    // Corresponding document: Antora/modules/howto_guides/pages/howto_configure_function_block.adoc
    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0204_FunctionBlockConfigure_FullListingTest()
    {
        // Create an openDAQ(TM) Instance, loading modules from the current directory
        Instance instance = OpenDAQFactory.Instance(MODULE_PATH);

        // Add simulated device
        Device device = instance.AddDevice("daqref://device0");

        // Add Function Block on the host computer
        FunctionBlock functionBlock = instance.AddFunctionBlock("RefFBModuleStatistics");

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

    #region Instance configuration

    // Corresponding document: Antora/modules/howto_guides/pages/howto_configure_instance.adoc
    //T0301_

    #endregion Instance configuration

    #region Configure instance provider

    // Corresponding document: Antora/modules/howto_guides/pages/howto_configure_instance_providers.adoc
    //T0401_

    #endregion Configure instance provider

    #region Configure streaming

    // Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0501_ConfigureStreamingServerSideConfigurationTest()
    {
        // Create an openDAQ(TM) Instance
        Instance instance = OpenDAQFactory.Instance(MODULE_PATH);

        // Add simulated device
        instance.SetRootDevice("daqref://device1");

        // Creates and registers a Server capability with the ID `OpenDAQLTStreaming` and the default port number 7414
        instance.AddServer("OpenDAQLTStreaming", null);

        // Creates and registers a Server capability with the ID `OpenDAQNativeStreaming` and the default port number 7420
        instance.AddServer("OpenDAQNativeStreaming", null);

        // As the Streaming servers were added first, the registered Server capabilities are published over OPC UA
        instance.AddServer("OpenDAQOPCUA", null);

        //while (true)
        //    Thread.Sleep(100);

        Thread.Sleep(2000);
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0502_ConfigureStreamingForStructureEnabledDeviceAutomaticallyTest()
    {
        // Create a new Instance that we will use for all the interactions with the SDK
        Instance instance = OpenDAQFactory.Instance(MODULE_PATH);

        // Create an empty Property object
        PropertyObject deviceConfig = CoreObjectsFactory.CreatePropertyObject();

        // Add property to allow multiple Streaming protocols with native protocol having the first priority
        var prioritizedStreamingProtocols =
                CoreTypesFactory.CreateList<BaseObject>("OpenDAQLTStreaming", "OpenDAQNativeStreaming");
        deviceConfig.AddProperty(PropertyFactory.ListProperty("PrioritizedStreamingProtocols",
                                                              prioritizedStreamingProtocols));

        // Set property to disregard direct Streaming connections for nested Devices,
        // and establish the minimum number of streaming connections possible.
        var streamingConnectionHeuristicProp =
                PropertyFactory.SelectionProperty("StreamingConnectionHeuristic",
                                                  CoreTypesFactory.CreateList<BaseObject>("MinConnections",
                                                                                          "MinHops",
                                                                                          "NotConnected"),
                                                  0);
        deviceConfig.AddProperty(streamingConnectionHeuristicProp);

        // Find and connect to a Device hosting an OPC UA TMS server
        var availableDevices = instance.AvailableDevices;
        Device device = null;
        foreach (var deviceInfo in availableDevices)
        {
            foreach (var capability in deviceInfo.ServerCapabilities)
            {
                if (capability.ProtocolName == "OpenDAQOPCUA")
                {
                    device = instance.AddDevice(capability.ConnectionString, deviceConfig);
                    break;
                }
            }
        }

        if (device == null)
            Console.WriteLine("No relevant Device found!");
        else
            // Output the name of the added Device
            Console.WriteLine(device.Info.Name);
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0503_ConfigureStreamingForStructureEnabledDeviceManuallyTest()
    {
        // Create a new Instance that we will use for all the interactions with the SDK
        Instance instance = OpenDAQFactory.Instance(MODULE_PATH);

        // Create an empty Property object
        PropertyObject deviceConfig = CoreObjectsFactory.CreatePropertyObject();

        // Set property to disable automatic Streaming connection
        var streamingConnectionHeuristicProp =
                PropertyFactory.SelectionProperty("StreamingConnectionHeuristic",
                                                  CoreTypesFactory.CreateList<BaseObject>("MinConnections",
                                                                                          "MinHops",
                                                                                          "NotConnected"),
                                                  2);
        deviceConfig.AddProperty(streamingConnectionHeuristicProp);

        // Connect to a Device hosting an OPC UA TMS server using connection string
        Device device = instance.AddDevice("daq.opcua://127.0.0.1", deviceConfig);

        if (device == null)
        {
            Console.WriteLine("No relevant Device found!");
            return;
        }
        else
        {
            // Output the name of the added Device
            Console.WriteLine(device.Info.Name);
        }

        // Connect to a Native Streaming protocol using connection string
        Streaming streaming = device.AddStreaming("daq.ns://127.0.0.1");

        // Get all Device's Signals recursively
        var deviceSignals = device.GetSignals(OpenDAQFactory.CreateRecursiveSearchFilter(OpenDAQFactory.CreateAnySearchFilter()));

        // Associate Device's Signals with Streaming
        streaming.AddSignals(deviceSignals);
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_0504_ConfigureStreamingConnectingPseudoDevicesTest()
    {
        // Create a new Instance that we will use for all the interactions with the SDK
        Instance instance = OpenDAQFactory.Instance(MODULE_PATH);

        // Find and connect to a Device hosting an Native Streaming server
        var availableDevices = instance.AvailableDevices;
        Device device = null;
        foreach (var deviceInfo in availableDevices)
        {
            foreach (var capability in deviceInfo.ServerCapabilities)
            {
                if (capability.ProtocolName == "OpenDAQNativeStreaming")
                {
                    device = instance.AddDevice(capability.ConnectionString);
                    break;
                }
            }
        }

        if (device == null)
        {
            Console.WriteLine("No relevant Device found!");
            return;
        }
        else
        {
            // Output the name of the added Device
            Console.WriteLine(device.Info.Name);
        }
    }

    [Test]
    [Category(SETUP_DEVICE_NATIVE_STREAMING)]
    public void Test_0505_ConfigureStreamingPerSignalTest()
    {
        //...

        // Get the first Signal of connected Device
        // (e.g. running "Reference device simulator" from examples\dotnet\howto_guides\howto_guides_simulator.cs)
        MirroredSignalConfig signal = device.GetSignalsRecursive()[0].Cast<MirroredSignalConfig>();

        // Find and output the Streaming sources available for Signal
        string nativeStreamingSource = null;
        string websocketStreamingSource = null;
        Console.WriteLine($"Signal supports {signal.StreamingSources.Count} streaming sources:");
        foreach (string source in signal.StreamingSources)
        {
            Console.WriteLine(source);
            if (source.StartsWith("daq.ns://"))
                nativeStreamingSource = source;
            if (source.StartsWith("daq.lt://"))
                websocketStreamingSource = source;
        }

        // Output the active Streaming source of Signal
        Console.WriteLine("Active streaming source of signal: " + signal.ActiveStreamingSource);

        // Output the Streaming status for the Signal to verify that streaming is enabled
        Console.WriteLine("Streaming enabled status for signal is: " + (signal.Streamed ? "true" : "false"));

        // Change the active Streaming source of Signal
        signal.ActiveStreamingSource = nativeStreamingSource;
    }

    [Test(ExpectedResult = 0)]
    [Category(SKIP_SETUP)]
    public int Test_0506_ConfigureStreaming_FullListingTest()
    {
        // Create a new Instance that we will use for all the interactions with the SDK
        Instance instance = OpenDAQFactory.Instance();

        // Create an empty Property object
        PropertyObject deviceConfig = CoreObjectsFactory.CreatePropertyObject();

        // Add property to allow multiple Streaming protocols with native protocol having the first priority
        var prioritizedStreamingProtocols =
                CoreTypesFactory.CreateList<BaseObject>("OpenDAQLTStreaming", "OpenDAQNativeStreaming");
        deviceConfig.AddProperty(PropertyFactory.ListProperty("PrioritizedStreamingProtocols",
                                                              prioritizedStreamingProtocols));

        // Set property to disregard direct Streaming connections for nested Devices,
        // and establish the minimum number of streaming connections possible.
        var streamingConnectionHeuristicProp =
                PropertyFactory.SelectionProperty("StreamingConnectionHeuristic",
                                                  CoreTypesFactory.CreateList<BaseObject>("MinConnections",
                                                                                          "MinHops",
                                                                                          "Fallbacks",
                                                                                          "NotConnected"),
                                                  0);
        deviceConfig.AddProperty(streamingConnectionHeuristicProp);

        // Find and connect to a Device using the device info connection string
        var availableDevices = instance.AvailableDevices;
        Device device = null;
        foreach (var deviceInfo in availableDevices)
        {
            if (deviceInfo.ConnectionString.StartsWith("daq://"))
            {
                device = instance.AddDevice(deviceInfo.ConnectionString, deviceConfig);
                break;
            }
        }

        // Exit if no Device is found
        if (device == null)
        {
            Console.WriteLine("*** No relevant Device found!");
            return 1;
        }

        // Output the name of the added Device
        Console.WriteLine(device.Info.Name);

        // Find the AI Signal
        var signals = device.GetSignalsRecursive();

        Channel channel = null;
        MirroredSignalConfig signal = null;
        foreach (var sig in signals)
        {
            var name = sig.Descriptor.Name;

            if (name.StartsWith("AI"))
            {
                signal = sig.Cast<MirroredSignalConfig>();
                channel = signal.Parent.Parent.Cast<Channel>();
                break;
            }
        }

        if (signal == null)
        {
            Console.WriteLine("*** No AI signal found!");
            return 1;
        }

        // Find and output the Streaming sources of Signal
        string nativeStreamingSource = null;
        string websocketStreamingSource = null;
        Console.WriteLine($"AI signal has {signal.StreamingSources.Count} Streaming sources:");
        foreach (string source in signal.StreamingSources)
        {
            Console.WriteLine(source);
            if (source.StartsWith("daq.ns://"))
                nativeStreamingSource = source;
            if (source.StartsWith("daq.lt://"))
                websocketStreamingSource = source;
        }

        // Check the active Streaming source of Signal
        if (signal.ActiveStreamingSource != websocketStreamingSource)
        {
            Console.WriteLine("*** Wrong active Streaming source of AI signal");
            return 1;
        }
        // Output samples using Reader with Streaming LT
        ReadSamples(signal);

        // Change the active Streaming source of Signal
        signal.ActiveStreamingSource = nativeStreamingSource;
        // Output samples using Reader with native Streaming
        ReadSamples(signal);

        Console.WriteLine();
        Console.Write("Press a key to exit the application ...");
        //Console.ReadKey(intercept: true);
        Console.WriteLine();
        return 0;



        // Function to read samples from the signal
        void ReadSamples(MirroredSignalConfig signal)
        {
            var reader = OpenDAQFactory.CreateStreamReader<double, ulong>(signal);

            // Get the resolution and origin
            DataDescriptor descriptor = signal.DomainSignal.Descriptor;
            Ratio resolution = descriptor.TickResolution;
            string origin = descriptor.Origin;
            string unitSymbol = descriptor.Unit.Symbol;

            Console.WriteLine($"\nReading signal: {signal.Name}; active Streaming source: {signal.ActiveStreamingSource}");
            Console.WriteLine("Origin: " + origin);

            // Allocate buffer for reading double samples
            double[] samples = new double[100];
            ulong[] domainSamples = new ulong[100];
            for (int i = 0; i < 40; ++i)
            {
                Thread.Sleep(25);

                // Read up to 100 samples every 25 ms, storing the amount read into `count`
                nuint count = 100;
                reader.ReadWithDomain(samples, domainSamples, ref count);
                if (count > 0)
                {
                    double domainValue = (long)domainSamples[count - 1] * resolution;
                    Console.WriteLine("Value: " + samples[count - 1] + ", Domain: " + domainValue + unitSymbol);
                }
            }
        }
    }

    #endregion Configure streaming

    #region Measure a single value

    // Corresponding document: Antora/modules/howto_guides/pages/howto_measure_single_value.adoc
    //T0601

    #endregion Measure a single value

    #region Save and load configuration

    // Corresponding document: Antora/modules/howto_guides/pages/howto_save_load_configuration.adoc

    [Test]
    public void Test_0701_ConfigurationSaveTest()
    {
        // Save Configuration to string
        string jsonStr = instance.SaveConfiguration();
        // Write Configuration string to file
        File.WriteAllText("openDAQconfig.json", jsonStr, System.Text.Encoding.UTF8);
    }

    [Test]
    public void Test_0702_ConfigurationLoadTest()
    {
        // Read Configuration from file
        string jsonStr = File.ReadAllText("openDAQconfig.json", System.Text.Encoding.UTF8);
        // Load Configuration from string
        instance.LoadConfiguration(jsonStr);
    }

    #endregion Save and load configuration

    #region Read data with Readers

    #region Basic value and domain

    // Corresponding document: Antora/modules/howto_guides/pages/howto_read_with_domain.adoc

    [Test]
    public void Test_0801_ReadWithDomainReaderCreateTest()
    {
        // These calls all create the same reader
        var reader1 = OpenDAQFactory.CreateStreamReader(signal);
        var reader2 = OpenDAQFactory.CreateStreamReader<double, long>(signal);
    }

    [Test]
    public void Test_0802_ReadWithDomainReaderReadingDataTest()
    {
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

    //[Test] //not a real test
    public void Test_0803_ReadWithDomainReaderHandlingSignalChangesTest()
    {
        // Signal Sample Type value is `Float64` (double)

        //Hint: StreamReaderBuilder not yet available in .NET Bindings (no "SkipEvents" possible)
        //var reader = OpenDAQFactory.StreamReaderBuilder<double, long>()
        //                           .SetSignal(signal)
        //                           //.SetValueReadType(SampleType::Float64)
        //                           //.SetDomainReadType(SampleType::Int64)
        //                           .SetSkipEvents(true)
        //                           .Build();
        var reader = OpenDAQFactory.CreateStreamReader<double, long>(signal);

        // Signal produces 2 samples { 1.1, 2.2 }

        //
        // The value Sample Type of the `signal` changes from `Float64` to `Int32`
        //

        // Signal produces 2 samples { 3, 4 }

        // If Descriptor has changed, Reader will return Reader status with that event
        // Call succeeds and results in 2 samples { 1.1, 2.2 }
        nuint count = 5;
        double[] values = new double[5];
        var status = reader.Read(values, ref count);
        System.Diagnostics.Debug.Assert(status.ReadStatus == ReadStatus.Event, "status.ReadStatus != ReadStatus.Event");

        // The subsequent call succeeds because `Int32` is convertible to `Float64`
        // and results in 2 samples { 3.0, 4.0 }
        reader.Read(values, ref count);

        //
        // The value Sample Type of the `signal` changes from `Int32` to `Int64`
        //

        // Signal produces 2 samples { 5, 6 }

        // Reader reads 0 values and returns status with new Event Packet
        nuint newCount = 2;
        double[] newValues = new double[2];
        var newStatus = reader.Read(newValues, ref newCount);
        System.Diagnostics.Debug.Assert(newCount == 0u, "newCount != 0");
        System.Diagnostics.Debug.Assert(newStatus.ReadStatus == ReadStatus.Event, "newStatus.ReadStatus != ReadStatus.Event");
    }

    //[Test] //not a real test
    public void Test_0804_ReadWithDomainReaderInvalidationAndReuseTest()
    {
        //Hint: StreamReaderBuilder not yet available in .NET Bindings (no "SkipEvents" possible)
        //var reader = OpenDAQFactory.StreamReaderBuilder<long, long>()
        //                           .SetSignal(signal)
        //                           //.SetValueReadType(SampleType::Int64)
        //                           //.SetDomainReadType(SampleType::Int64)
        //                           .SetSkipEvents(true)
        //                           .Build();
        var reader = OpenDAQFactory.CreateStreamReader<long, long>(signal);

        // Signal produces 5 samples { 1, 2, 3, 4, 5 }

        nuint count = 2;
        long[] values = new long[2];
        var firstStatus = reader.Read(values, ref count);  // count = 0, firstStatus = Event //currently no "SkipEvents" possible
        reader.Read(values, ref count);  // count = 2, values = { 1, 2 }

        // Reuse the Reader
        //var newReader = OpenDAQFactory.CreateStreamReaderFromExisting<double, long>(reader); //currently not possible to change types
        var newReader = OpenDAQFactory.CreateStreamReaderFromExisting<long, long>(reader);

        // New Reader successfully continues on from previous Reader's position
        count = 2;
        long[] newValues = new long[2];
        newReader.Read(newValues, ref count);  // count = 2, values = { 3, 4 }

        // The old Reader has been invalidated when reused by a new one
        count = 2;
        long[] oldValues = new long[2];
        var status = reader.Read(oldValues, ref count);
        System.Diagnostics.Debug.Assert(status.Valid == false, "status.Valid != false");
    }

    //[Test] //not a real test
    public void Test_0805_ReadWithDomainReaderInvalidationAndReuseTest2()
    {
        //ToDo: uses createPacketForSignal which is unknown
        //ToDo: uses SignalConfig as signal
    }

    #endregion Basic value and domain

    #region Only last N samples

    // Corresponding document: Antora/modules/howto_guides/pages/howto_read_last_n_samples.adoc

    //[Test] //not a real test
    public void Test_0901_OnlyLastNSamplesTest()
    {
        var reader = OpenDAQFactory.CreateTailReader(signal, 5);

        // Signal produces 3 samples: { 1, 2, 3 }

        nuint count = 5;
        double[] values = new double[5];
        long[] domain = new long[5];

        reader.ReadWithDomain(values, domain, ref count); // count = 3, values = { 1, 2, 3, 0, 0 }
    }

    //[Test] //not a real test
    public void Test_0902_OnlyLastNSamplesTest()
    {
        //Hint: TailReaderBuilder not yet available in .NET Bindings (no "SkipEvents" possible)
        //var reader = OpenDAQFactory.TailReaderBuilder().SetSignal(signal).SetHistorySize(5).SetSkipEvents(true).Build();
        var reader = OpenDAQFactory.CreateTailReader(signal, 5);

        // The Signal produces 3 samples: 1, 2, 3

        nuint count = 5;
        double[] values = new double[5];

        reader.Read(values, ref count); // count = 3, values = { 1, 2, 3, 0, 0 }

        // The Signal produces 3 samples: 4, 5, 6

        count = 5;
        reader.Read(values, ref count); // count = 5, values = { 2, 3, 4, 5, 6 }
    }

    [Test]
    public void Test_0903_OnlyLastNSamplesReadingAboveHistorySizeTest()
    {
        var reader = OpenDAQFactory.CreateTailReader(signal, 5);

        /*
         * The Signal produces 3 Packets
         * [Packet 1]: 4 samples
         * [Packet 2]: 3 samples
         * [Packet 3]: 1 sample
         * -------------------------------
         *      Total: 8 samples
         */

        nuint count = 10;
        double[] values = new double[10];

        // Will return staus.ReadStatus == ReadStatus.Fail and count == 0
        var status = reader.Read(values, ref count);

        // Will succeed as [Packet 3] and [Packet 2] together are less than 5 samples
        // and we still need to keep the [Packet 1] around to satisfy the minimum of 5 samples
        count = 8;
        status = reader.Read(values, ref count); //returns status.ReadStatus == ReadStatus.Event and count == 0
        status = reader.Read(values, ref count); //returns status.ReadStatus == ReadStatus.Ok and count == 8
    }

    //[Test(ExpectedResult = 0)] //not a real test
    public int Test_0904_OnlyLastNSamples_FullListingTest()
    {
#if not_applicable_in_csharp
        SignalConfig signal = setupExampleSignal();

        example1(signal);
        example2(signal);
        example3(signal);

        return 0;

        /*
         * Example 1: Behavior of the Tail Reader before getting the full history-size samples
         */
        void example1(SignalConfig signal)
        {
            var reader = OpenDAQFactory.CreateTailReader(signal, 5);
            System.Diagnostics.Debug.Assert(reader.AvailableCount == 0u, "reader.AvailableCount != 0u");

            // Allocate the buffers for the reader to copy data into
            nuint count = 0;
            double[] values = new double[5];
            long[] domain = new long[5];

            // Is below the history-size
            count = 3;
            reader.ReadWithDomain(values, domain, ref count);
            System.Diagnostics.Debug.Assert(count == 0, "count != 0");

            try
            {
                // Is more than the history-size
                count = 6;
                reader.ReadWithDomain(values, domain, ref count);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"*** {ex.GetType().Name}: {ex.Message}");
            }

            // The Signal produces 3 samples: 1, 2, 3
            var packet = createPacketForSignal(signal, 3, 0);
            double[] data = static_cast<double*>(packet.Data);
            data[0] = 1;
            data[1] = 2;
            data[2] = 3;
            signal.SendPacket(packet);

            count = 5;
            reader.ReadWithDomain(values, domain, ref count);

            // count = 3, values = { 1, 2, 3, 0, 0 }
            System.Diagnostics.Debug.Assert(count == 3u, "count != 3u");
            System.Diagnostics.Debug.Assert(values[0] == 1, "values[0] != 1");
            System.Diagnostics.Debug.Assert(values[1] == 2, "values[1] != 2");
            System.Diagnostics.Debug.Assert(values[2] == 3, "values[2] != 3");
            System.Diagnostics.Debug.Assert(values[3] == 0, "values[3] != 0");
            System.Diagnostics.Debug.Assert(values[4] == 0, "values[4] != 0");
        }

        /*
        * Example 2: Subsequent reads can have overlapping samples
        */
        void example2(SignalConfig signal)
        {
            var reader = TailReaderBuilder().setSignal(signal).setHistorySize(5).setSkipEvents(true).build();

            // The Signal produces 3 samples: 1, 2, 3
            SizeT FIRST_PACKET_SAMPLES = 3u;
            var packet = createPacketForSignal(signal, FIRST_PACKET_SAMPLES);
            var data = static_cast<double*>(packet.getData());
            data[0] = 1;
            data[1] = 2;
            data[2] = 3;
            signal.sendPacket(packet);

            // Allocate the buffers for the reader to copy data into
            nuint count{ 5}
            ;
            double[] values = new double[5]{ }
            ;
            reader.read(values, ref count);

            // count = 3, values = { 1, 2, 3, 0, 0 }
            System.Diagnostics.Debug.Assert(count == 3u, "count != 3u");
            System.Diagnostics.Debug.Assert(values[0] == 1, "values[0] != 1");
            System.Diagnostics.Debug.Assert(values[1] == 2, "values[1] != 2");
            System.Diagnostics.Debug.Assert(values[2] == 3, "values[2] != 3");
            System.Diagnostics.Debug.Assert(values[3] == 0, "values[3] != 0");
            System.Diagnostics.Debug.Assert(values[4] == 0, "values[4] != 0");

            // The Signal produces 3 samples: 4, 5, 6
            var packet2 = createPacketForSignal(signal, 3, FIRST_PACKET_SAMPLES);
            var data2 = static_cast<double*>(packet2.getData());
            data2[0] = 4;
            data2[1] = 5;
            data2[2] = 6;
            signal.sendPacket(packet2);

            count = 5;
            reader.read(values, ref count);

            // count = 5, values = { 2, 3, 4, 5, 6 }
            System.Diagnostics.Debug.Assert(count == 5, "count != 5");
            System.Diagnostics.Debug.Assert(values[0] == 2, "values[0] != 2");
            System.Diagnostics.Debug.Assert(values[1] == 3, "values[1] != 3");
            System.Diagnostics.Debug.Assert(values[2] == 4, "values[2] != 4");
            System.Diagnostics.Debug.Assert(values[3] == 5, "values[3] != 5");
            System.Diagnostics.Debug.Assert(values[4] == 6, "values[4] != 6");
        }

        void example3(SignalConfig signal)
        {
            var reader = TailReaderBuilder().setSignal(signal).setHistorySize(5).setSkipEvents(true).build();

            /*
             * The Signal produces 3 Packets
             * [Packet 1]: 4 samples
             * [Packet 2]: 3 samples
             * [Packet 3]: 1 sample
             * -------------------------------
             *      Total: 8 samples
             */

            var packet1 = createPacketForSignal(signal, 4);
            var packet2 = createPacketForSignal(signal, 3);
            var packet3 = createPacketForSignal(signal, 1);
            signal.sendPacket(packet1);
            signal.sendPacket(packet2);
            signal.sendPacket(packet3);

            System.Diagnostics.Debug.Assert(reader.AvailableCount == 8u, "reader.AvailableCount != 8u");

            // Allocate the buffers for the reader to copy data into
            nuint count{ }
            ;
            double[] values = new double[10]{ }
            ;

            try
            {
                count = 10;

                // Will throw a SizeTooLargeException
                reader.read(values, ref count);
            }
            catch (SizeTooLargeExcept e)
            {
                Console.WriteLine("*** Exception: " + e.what());
            }

            // Will succeed as [Packet 3] and [Packet 2] together are less than 5 samples,
            // and we still need to keep [Packet 1] around to satisfy the minimum of 5 samples
            count = 8;
            reader.read(values, ref count);

            System.Diagnostics.Debug.Assert(count == 8u, "count != 8u");
        }

        /*
         * Set up the Signal with Float64 data
         */
        SignalConfig setupExampleSignal()
        {
            var logger = Logger();
            var context = Context(Scheduler(logger, 1), logger, nullptr, nullptr);

            var signal = Signal(context, nullptr, "example signal");
            signal.setDescriptor(setupDescriptor(SampleType::Float64));

            return signal;
        }

        DataDescriptor setupDescriptor(SampleType type, DataRule rule)
        {
            // Set up the data descriptor with the provided Sample-Type
            //var dataDescriptor = OpenDAQFactory.CreateDataDescriptorBuilder().SetSampleType(type);
            var dataDescriptor = OpenDAQFactory.CreateDataDescriptorBuilder();
            dataDescriptor.SetSampleType(type);

            // For the Domain, we provide a Linear Rule to generate time-stamps
            if (rule.assigned())
                dataDescriptor.setRule(rule);

            return dataDescriptor.build();
        }

        DataPacket createPacketForSignal(Signal signal, nuint numSamples, long offset)
        {
            // Create a Data Packet where the values are generated via the +1 rule starting at 0
            var domainPacket =
                OpenDAQFactory.CreateDataPacket(setupDescriptor(SampleType.Int64, OpenDAQFactory.CreateLinearDataRule(1, 0)),
                                                numSamples,
                                                offset  // offset from 0 to start the sample generation at
                                                );

            return OpenDAQFactory.CreateDataPacketWithDomain(domainPacket, signal.Descriptor, numSamples, offset);
        }
#else
        //Since there are several features not available for the .NET Bindings, there is no full listing for this language. Please refer to the specific sections above for the examples.
        return 0;
#endif
    }

    #endregion Only last N samples

    #region Absolute time-stamps

    // Corresponding document: Antora/modules/howto_guides/pages/howto_read_with_timestamps.adoc

    [Test]
    public void Test_1001_ReadWithAbsoluteTimeStampsTimeReaderTest()
    {
        var reader = OpenDAQFactory.CreateStreamReader(signal);

        // Signal produces 5 samples

        var timeReader = OpenDAQFactory.CreateTimeReader(reader, signal);

        nuint count = 5;
        double[] values = new double[5];
        DateTime[] timeStamps = new DateTime[5];

        // Read with Time Reader
        timeReader.ReadWithDomain(values, timeStamps, ref count);

        for (nuint i = 0; i < count; ++i)
        {
            Console.WriteLine($"{timeStamps[i]:yyyy-MM-dd HH:mm:ss.fff}: {values[i]}");
        }
    }

    //[Test] //not a real test
    public int Test_1002_ReadWithAbsoluteTimeStampsWrappedReaderTest()
    {
        //The timestamps are not available in the original Reader for the .NET Bindings, as the Time Reader is a wrapper around the original Reader and does not modify it.
        return 0;
    }

    //[Test(ExpectedResult = 0)] //not a real test
    public int Test_1003_ReadWithAbsoluteTimeStamps_FullListingTest()
    {
        //Since there are several features not available for the .NET Bindings, there is no full listing for this language. Please refer to the specific sections above for the examples.
        return 0;
    }

    #endregion Absolute time-stamps

    #region Time-outs

    // Corresponding document: Antora/modules/howto_guides/pages/howto_read_with_timeouts.adoc
    //T1101_

    #endregion Time-outs

    #region Align multiple signals

    // Corresponding document: Antora/modules/howto_guides/pages/howto_read_aligned_signals.adoc
    //T1201_

    #endregion Align multiple signals

    #endregion Read data with Readers

    #region Access control

    // Corresponding document: Antora/modules/howto_guides/pages/howto_access_control.adoc

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_1301_AccessControlCreateServerTest()
    {
        var users = CoreTypesFactory.CreateList<BaseObject>();
        users.Add(OpenDAQFactory.User("opendaq", "opendaq123"));
        users.Add(OpenDAQFactory.User("root", "root123", new List<string>{ "admin" }));

        var authenticationProvider = CoreObjectsFactory.CreateStaticAuthenticationProvider(true, users);

        var builder = OpenDAQFactory.CreateInstanceBuilder();
        builder.AuthenticationProvider = authenticationProvider;

        var instance = builder.Build();
        instance.AddStandardServers();

        //Console.ReadLine();
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_1302_AccessControlHashingTest()
    {
        var users = CoreTypesFactory.CreateList<BaseObject>();
        users.Add(OpenDAQFactory.User("opendaq", "$2a$12$MmSt1b9YEHB5SpLNyikiD.37NvN23UA7zLH6Y98ob5HF0OsKH0IuO"));
        users.Add(OpenDAQFactory.User("root", "$2a$12$ceV7Q2j.vZcuz05hy1EkC.GHH8PIrv0D5wz7iLH9twsyumgZ4tGI2", new List<string> { "admin" }));
        var authenticationProvider = CoreObjectsFactory.CreateStaticAuthenticationProvider(true, users);

        var builder = OpenDAQFactory.CreateInstanceBuilder();
        builder.AuthenticationProvider = authenticationProvider;

        var instance = builder.Build();
        instance.AddStandardServers();

        //Console.ReadLine();
    }

    //[Test]
    //[Category(SKIP_SETUP)]
    public void Test_1303_AccessControlConnectWithUserNameTest()
    {
        var prepareAndStartServer = () =>
        {
            var users = CoreTypesFactory.CreateList<BaseObject>();
            users.Add(OpenDAQFactory.User("opendaq", "$2a$12$MmSt1b9YEHB5SpLNyikiD.37NvN23UA7zLH6Y98ob5HF0OsKH0IuO"));
            users.Add(OpenDAQFactory.User("root", "$2a$12$ceV7Q2j.vZcuz05hy1EkC.GHH8PIrv0D5wz7iLH9twsyumgZ4tGI2", new List<string> { "admin" }));
            var authenticationProvider = CoreObjectsFactory.CreateStaticAuthenticationProvider(true, users);

            var builder = OpenDAQFactory.CreateInstanceBuilder();
            builder.AuthenticationProvider = authenticationProvider;

            var instance = builder.Build();
            instance.AddStandardServers();
            return instance;
        };

        var serverInstance = prepareAndStartServer();

        // Start of the example

        var instance = OpenDAQFactory.Instance();

        var config = instance.CreateDefaultAddDeviceConfig();
        var generalConfig = config.GetPropertyValue("General").Cast<PropertyObject>();

        generalConfig.SetPropertyValue("Username", "opendaq");
        generalConfig.SetPropertyValue("Password", "opendaq123");

        var device = instance.AddDevice("daq.nd://127.0.0.1", config);
        Console.WriteLine("Connected to: " + device.Name);
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_1304_AccessControlProtectedObjectTest()
    {
        var users = CoreTypesFactory.CreateList<BaseObject>();
        users.Add(OpenDAQFactory.User("opendaq", "$2a$12$MmSt1b9YEHB5SpLNyikiD.37NvN23UA7zLH6Y98ob5HF0OsKH0IuO"));
        users.Add(OpenDAQFactory.User("root", "$2a$12$ceV7Q2j.vZcuz05hy1EkC.GHH8PIrv0D5wz7iLH9twsyumgZ4tGI2", new List<string> { "admin" }));

        var authenticationProvider = CoreObjectsFactory.CreateStaticAuthenticationProvider(true, users);

        var builder = OpenDAQFactory.CreateInstanceBuilder();
        builder.AuthenticationProvider = authenticationProvider;

        var instance = builder.Build();
        instance.AddStandardServers();
        instance.AddDevice("daqref://device0");

        //Console.ReadLine();
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_1305_AccessControlAllowTest()
    {
        var targetObject = CoreObjectsFactory.CreatePropertyObject();
        var parentObject = CoreObjectsFactory.CreatePropertyObject();
        parentObject.AddProperty(PropertyFactory.ObjectProperty("TargetObject", targetObject));

        var maskRw = CoreObjectsFactory.CreatePermissionMaskBuilder();
        maskRw.Read();
        maskRw.Write();

        var maskX = CoreObjectsFactory.CreatePermissionMaskBuilder();
        maskX.Execute();

        var parentPermissions = CoreObjectsFactory.CreatePermissionsBuilder();
        parentPermissions.Assign("everyone", maskRw);
        parentObject.PermissionManager.SetPermissions(parentPermissions.Build());

        var permissions = CoreObjectsFactory.CreatePermissionsBuilder();
        permissions.Inherit(true);
        permissions.Allow("everyone", maskX);
        targetObject.PermissionManager.SetPermissions(permissions.Build());

        // target object permissions:
        // everyone: rwx

        var user = OpenDAQFactory.User("", "");
        Assert.That(targetObject.PermissionManager.IsAuthorized(user, Permission.Read), Is.True);
        Assert.That(targetObject.PermissionManager.IsAuthorized(user, Permission.Write), Is.True);
        Assert.That(targetObject.PermissionManager.IsAuthorized(user, Permission.Execute), Is.True);
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_1306_AccessControlDenyTest()
    {
        var targetObject = CoreObjectsFactory.CreatePropertyObject();
        var parentObject = CoreObjectsFactory.CreatePropertyObject();
        parentObject.AddProperty(PropertyFactory.ObjectProperty("TargetObject", targetObject));

        var maskRwx = CoreObjectsFactory.CreatePermissionMaskBuilder();
        maskRwx.Read();
        maskRwx.Write();
        maskRwx.Execute();

        var maskX = CoreObjectsFactory.CreatePermissionMaskBuilder();
        maskX.Execute();

        var parentPermissions = CoreObjectsFactory.CreatePermissionsBuilder();
        parentPermissions.Assign("everyone", maskRwx);
        parentObject.PermissionManager.SetPermissions(parentPermissions.Build());

        var permissions = CoreObjectsFactory.CreatePermissionsBuilder();
        permissions.Inherit(true);
        permissions.Deny("everyone", maskX);
        targetObject.PermissionManager.SetPermissions(permissions.Build());

        // target object permissions:
        // everyone: rw

        var user = OpenDAQFactory.User("", "");
        Assert.That(targetObject.PermissionManager.IsAuthorized(user, Permission.Read), Is.True);
        Assert.That(targetObject.PermissionManager.IsAuthorized(user, Permission.Write), Is.True);
        Assert.That(targetObject.PermissionManager.IsAuthorized(user, Permission.Execute), Is.False);
    }

    [Test]
    [Category(SKIP_SETUP)]
    public void Test_1307_AccessControlAssignTest()
    {
        var targetObject = CoreObjectsFactory.CreatePropertyObject();
        var parentObject = CoreObjectsFactory.CreatePropertyObject();
        parentObject.AddProperty(PropertyFactory.ObjectProperty("TargetObject", targetObject));

        var maskRwx = CoreObjectsFactory.CreatePermissionMaskBuilder();
        maskRwx.Read();
        maskRwx.Write();
        maskRwx.Execute();

        var maskR = CoreObjectsFactory.CreatePermissionMaskBuilder();
        maskR.Read();

        var parentPermissions = CoreObjectsFactory.CreatePermissionsBuilder();
        parentPermissions.Assign("everyone", maskRwx);
        parentPermissions.Assign("guest", maskR);
        parentObject.PermissionManager.SetPermissions(parentPermissions.Build());

        var permissions = CoreObjectsFactory.CreatePermissionsBuilder();
        permissions.Inherit(true);
        permissions.Assign("everyone", maskR);
        targetObject.PermissionManager.SetPermissions(permissions.Build());

        // target object permissions:
        // everyone: r
        // guest: r

        var user = OpenDAQFactory.User("", "");
        Assert.That(targetObject.PermissionManager.IsAuthorized(user, Permission.Read), Is.True);
        Assert.That(targetObject.PermissionManager.IsAuthorized(user, Permission.Write), Is.False);
        Assert.That(targetObject.PermissionManager.IsAuthorized(user, Permission.Execute), Is.False);

        var guest = OpenDAQFactory.User("guest", "guest", new List<string> { "guest" });
        Assert.That(targetObject.PermissionManager.IsAuthorized(guest, Permission.Read), Is.True);
        Assert.That(targetObject.PermissionManager.IsAuthorized(guest, Permission.Write), Is.False);
        Assert.That(targetObject.PermissionManager.IsAuthorized(guest, Permission.Execute), Is.False);
    }

    #endregion Access control

    //template snippet - clone these lines and rename the method for a new test
    //[Test]
    //public void _EmptyTest()
    //{
    //}
}
