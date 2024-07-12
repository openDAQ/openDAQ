
// Ignore Spelling: Opc Ua daqref daq npos cout endl dio Usings

#define LOCAL_ONLY
//#define USE_LISTOBJECT
//#define USE_IENUMERATOR_NOT_IENUMERABLE


using System.Diagnostics;

using Daq.Core.Types;
using Daq.Core.Objects;
using Daq.Core.OpenDAQ;
using Daq.Core;


namespace openDaq.Net.Test;


public class OpenDaq_Tests : OpenDAQTestsBase
{
    private const string ConnectionProtocolDaqRef  = "daqref://";
    private const string ConnectionProtocolOpcUa   = "daq.opcua://";
    private const string ConnectionProtocolWinSock = "daq.lt://";

    public enum eDesiredConnection
    {
        Any = 0,
        DaqRef,
        OpcUa,
        WinSock,
        LocalHost
    }

    public enum eDesiredReader
    {
        Stream = 0,
        Tail,
        Block,
        Multi,
        //Packet
    }

    [SetUp]
    public void Setup()
    {
        base.DontWarn();
    }

    //[TearDown]
    //public void TearDown()
    //{
    //}

    private static Device ConnectFirstAvailableDevice(Instance daqInstance,
                                                      eDesiredConnection desiredConnection = eDesiredConnection.DaqRef,
                                                      bool doLog = false)
    {
        /*
            // Create a fresh openDAQ(TM) instance that we will use for all the interactions with the openDAQ(TM) SDK
            daq::InstancePtr instance = daq::Instance(MODULE_PATH);

            // Find and connect to a device hosting an openDAQ(TM) OPC UA server
            const auto availableDevices = instance.getAvailableDevices();
            daq::DevicePtr device;
            for (const auto& deviceInfo : availableDevices)
            {
                if (deviceInfo.getConnectionString().toStdString().find("daq.opcua://") != std::string::npos)
                {
                    device = instance.addDevice(deviceInfo.getConnectionString());
                    break;
                }
            }
         */

        Console.WriteLine($"Trying to connect to {desiredConnection} device");

        if (doLog) Console.WriteLine("daqInstance.AvailableDevices");
        var availableDevicesInfos = daqInstance.AvailableDevices;
        var deviceInfoCount = availableDevicesInfos.Count;
        if (doLog) Console.WriteLine($"  {deviceInfoCount} devices available");

        //take only the first valid connection string
        string? connectionString = null;

        bool doDaqRef  = (desiredConnection == eDesiredConnection.Any) || (desiredConnection == eDesiredConnection.DaqRef);
        bool doOpcUa   = (desiredConnection == eDesiredConnection.Any) || (desiredConnection == eDesiredConnection.OpcUa);
        bool doWinSock = (desiredConnection == eDesiredConnection.Any) || (desiredConnection == eDesiredConnection.WinSock);
        bool doLocalHost = (desiredConnection == eDesiredConnection.Any) || (desiredConnection == eDesiredConnection.LocalHost);

        //foreach (var deviceInfo in availableDevicesInfos) { }

        using (var deviceInfoIterator = availableDevicesInfos.GetEnumerator())
        {
            while (deviceInfoIterator.MoveNext())
            {
                using var deviceInfo             = deviceInfoIterator.Current;
                var       deviceConnectionString = deviceInfo.ConnectionString;
                var       deviceName             = deviceInfo.Name;

                if (deviceName.StartsWith("HBK-CAL"))
                    continue;

                if (deviceName.StartsWith("HBK-HF-"))
                    continue;

                if (deviceName.StartsWith("HBK-SY-"))
                    continue;

                //define when only local VM should be used (home office)
#if LOCAL_ONLY
                if (deviceConnectionString.StartsWith("daq.opcua://172.")
                   )
                {
                    continue;
                }
#endif
#if FUSION_BRIDGE_ONLY
                if (deviceConnectionString.StartsWith("daq.opcua://172.")
                    //|| deviceConnectionString.StartsWith("daq.opcua://192.") && !deviceName.StartsWith("HBK-B")
                    //&& !deviceConnectionString.Equals("daq.opcua://172.19.195.187/")
                    //&& !deviceConnectionString.Equals("daq.opcua://172.19.195.188/")
                    )
                {
                    continue;
                }
#endif

                //connectible device?
                if ((doOpcUa && deviceConnectionString.StartsWith(ConnectionProtocolOpcUa, StringComparison.InvariantCultureIgnoreCase))
                    || (doWinSock && deviceConnectionString.StartsWith(ConnectionProtocolWinSock, StringComparison.InvariantCultureIgnoreCase))
                    || (doDaqRef && deviceConnectionString.StartsWith(ConnectionProtocolDaqRef, StringComparison.InvariantCultureIgnoreCase))) //DaqRef last (fall-back for Any)!
                {
                    connectionString = deviceConnectionString;
                    break;
                }
            }
        }

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

    private static void ShowAllProperties(PropertyObject propertyObject, string varName)
    {
        Console.WriteLine(varName + ".AllProperties");
        var properties = propertyObject.AllProperties;

        Console.WriteLine($"  {properties.Count} properties found");
        using var propIterator = properties.GetEnumerator();
        while (propIterator.MoveNext())
        {
            using var property = propIterator.Current;
            var propertyName = property.Name;

            Console.WriteLine($"  - {propertyName} ({property.ValueType})");
        }
    }


    [Test]
    public void GetVersionTest()
    {
        Version coreTypesVersion   = new();
        Version coreObjectsVersion = new();
        Version openDaqVersion     = new();

        Assert.Multiple(() =>
        {
            Assert.DoesNotThrow(() => coreTypesVersion   = CoreTypesFactory.SdkVersion,   "CoreTypesFactory.GetSdkVersion() failed");
            Assert.DoesNotThrow(() => coreObjectsVersion = CoreObjectsFactory.SdkVersion, "CoreObjectsFactory.GetSdkVersion() failed");
            Assert.DoesNotThrow(() => openDaqVersion     = OpenDAQFactory.SdkVersion,     "OpenDAQFactory.GetSdkVersion() failed");
        });

        Console.WriteLine($"CoreTypes SDK version   = {coreTypesVersion} / .NET Bindings version = {CoreTypesDllInfo.Version}");
        Console.WriteLine($"CoreObjects SDK version = {coreObjectsVersion} / .NET Bindings version = {CoreObjectsDllInfo.Version}");
        Console.WriteLine($"openDAQ SDK version     = {coreTypesVersion} / .NET Bindings version = {OpenDAQDllInfo.Version}");
    }

    [Test]
    public void GuidTest()
    {
        Type daqInstanceInterfaceType = typeof(Instance);
        Guid guid = daqInstanceInterfaceType.GUID;

        Assert.Multiple(() =>
        {
            Assert.That(daqInstanceInterfaceType.Name, Is.EqualTo(nameof(Instance)));
            Assert.That(guid, Is.EqualTo(Guid.Parse("60f2c1f4-83a8-5498-9ba8-b8a848873d4a")));
        });
    }

    [Test]
    public void CreateObjectsGuidValidationTest()
    {
        Assert.Multiple(() =>
        {
            Assert.DoesNotThrow(() => { using var test = CoreTypesFactory.CreateString("test"); });
            Assert.DoesNotThrow(() => { using var test = CoreTypesFactory.CreateInteger(999L); });
            Assert.DoesNotThrow(() => { using var test = CoreTypesFactory.CreateFloat(999D); });
            Assert.DoesNotThrow(() => { using var test = CoreObjectsFactory.CreatePropertyObject(); });
            //Assert.DoesNotThrow(() => { using var test = OpenDAQFactory.Create(); });
        });
    }

    [Test]
    public void InstanceTestManualDispose()
    {
        //instruct TearDown function not to collect and finalize managed objects explicitly
        base.DontCollectAndFinalize();

        Instance daqInstance = OpenDAQFactory.Instance(".");
        Assert.That(daqInstance.IsDisposed, Is.False);

        //can do something with 'daqInstance' here
        using var info = daqInstance.Info;

        var name = info.Name;
        Console.WriteLine($"daqInstance name = '{name}'");

        //finally free managed resources (release reference)
        daqInstance.Dispose();
        Assert.That(daqInstance.IsDisposed, Is.True);
    }

    [Test]
    public void InstanceTestAutoDispose1()
    {
        //instruct TearDown function not to collect and finalize managed objects explicitly
        base.DontCollectAndFinalize();

        Instance daqInstance;

        using (daqInstance = OpenDAQFactory.Instance("."))
        {
            Assert.That(daqInstance.IsDisposed, Is.False);

            //can do something with 'daqInstance' here
            using var info = daqInstance.Info;

            var name = info.Name;
            Console.WriteLine($"daqInstance name = '{name}'");
        } //losing scope here, automatically calling Dispose() and thus freeing managed resources (release reference)

        Assert.That(daqInstance.IsDisposed, Is.True);
    }

    [Test]
    public void InstanceTestAutoDispose2()
    {
        //instruct TearDown function not to collect and finalize managed objects explicitly
        base.DontCollectAndFinalize();

        using Instance daqInstance = OpenDAQFactory.Instance(".");

        Assert.That(daqInstance.IsDisposed, Is.False);

        //can do something with 'daqInstance' here
        using var info = daqInstance.Info;

        var name = info.Name;
        Console.WriteLine($"daqInstance name = '{name}'");

        //losing scope at the end of this method, automatically calling Dispose() and thus freeing managed resources (release reference)
    }

    [Test]
    public void ListObjectDisposeTest()
    {
        //instruct TearDown function not to collect and finalize managed objects explicitly
        base.DontCollectAndFinalize();

        Console.WriteLine("> creating...");
        var list = CoreTypesFactory.CreateList<BaseObject>();
        var obj  = CoreTypesFactory.CreateBaseObject();

        Assert.That(list.IsDisposed, Is.False);
        Assert.That(list.Count, Is.EqualTo(0));
        Assert.That(obj.IsDisposed, Is.False);

        ((ListObject<BaseObject>)list).PrintReferenceCount();
        obj.PrintReferenceCount();

        Console.WriteLine("> adding to list...");
        list.Add(obj);
        Assert.That(list.Count, Is.EqualTo(1));

        ((ListObject<BaseObject>)list).PrintReferenceCount();
        obj.PrintReferenceCount();

        Console.WriteLine("> disposing object...");
        obj.Dispose();
        Assert.That(obj.IsDisposed, Is.True);

        ((ListObject<BaseObject>)list).PrintReferenceCount();
        obj.PrintReferenceCount();

        Console.WriteLine("> getting first element...");
        obj = list[0];

        ((ListObject<BaseObject>)list).PrintReferenceCount();
        obj.PrintReferenceCount();

        Console.WriteLine("> disposing list...");
        list.Dispose();
        Assert.That(list.IsDisposed, Is.True);
        Assert.That(obj.IsDisposed, Is.False);

        ((ListObject<BaseObject>)list).PrintReferenceCount();
        obj.PrintReferenceCount();

        Console.WriteLine("> disposing object...");
        obj.Dispose();
        Assert.That(obj.IsDisposed, Is.True);

        ((ListObject<BaseObject>)list).PrintReferenceCount();
        obj.PrintReferenceCount();
    }

    [Test]
    public void GetAvailableDevicesTest()
    {
        //Hint: below it makes no sense checking objects for null since the functions throw 'OpenDaqException' in case of an error ( Result.Failed(errorCode) )
        //Hint: using statements are used below that the objects get dereferenced properly when losing scope (calling Dispose() for each "using" object at the end of this method)

        using Instance daqInstance = OpenDAQFactory.Instance(".");

        /*
            // Create a fresh openDAQ(TM) instance that we will use for all the interactions with the openDAQ(TM) SDK
            daq::InstancePtr instance = daq::Instance(MODULE_PATH);

            // Find and output the names and connection strings of all available devices
            daq::ListPtr<daq::IDeviceInfo> availableDevicesInfo = instance.getAvailableDevices();
            for (const auto& deviceInfo : availableDevicesInfo)
                std::cout << "Name: " << deviceInfo.getName() << ", Connection string: " << deviceInfo.getConnectionString() << std::endl;
         */

        Console.WriteLine("> daqInstance.AvailableDevices");
        var availableDevicesInfos = daqInstance.AvailableDevices;

        Console.WriteLine($"  {availableDevicesInfos.Count} devices available");

        //list all devices
        foreach (var deviceInfo in availableDevicesInfos)
        {
            var    deviceName       = deviceInfo.Name;
            var    connectionString = deviceInfo.ConnectionString;
            string model            = deviceInfo.GetPropertyValue("model");
            string deviceClass      = deviceInfo.GetPropertyValue("deviceClass");
            string softwareRevision = deviceInfo.HasProperty("softwareRevision") ? deviceInfo.GetPropertyValue("softwareRevision") : "n/a";

            Console.WriteLine($"  - Name = '{deviceName}', Connection string = '{connectionString}'");
            Console.WriteLine($"    model = '{model}', deviceClass = '{deviceClass}', softwareRevision = '{softwareRevision}'");

            //ShowAllProperties(deviceInfo, nameof(deviceInfo));
        }

        //not necessary here due to using statement above
        //daqInstance.Dispose();
        //Assert.That(daqInstance.IsDisposed, Is.True);
    }

    [TestCase(eDesiredConnection.DaqRef)]
    [TestCase(eDesiredConnection.DaqRef, true)]
    [TestCase(eDesiredConnection.OpcUa)]
    [TestCase(eDesiredConnection.OpcUa, true)]
    [TestCase(eDesiredConnection.WinSock)]
    [TestCase(eDesiredConnection.WinSock, true)]
    [TestCase(eDesiredConnection.LocalHost)]
    [TestCase(eDesiredConnection.LocalHost, true)]
    public void ConnectDeviceNoUsingsTest(eDesiredConnection desiredConnection, bool disposeDeviceLast = false)
    {
        Console.WriteLine($"Connect first {desiredConnection} device and dispose device {(disposeDeviceLast ? "last" : "first")}");

        var daqInstance = OpenDAQFactory.Instance(".");

        var device = ConnectFirstAvailableDevice(daqInstance, desiredConnection, doLog: true);

        //var deviceInfo = device.Info;
        //ShowAllProperties(deviceInfo, nameof(deviceInfo));

        if (!disposeDeviceLast) device.Dispose();

        daqInstance.Dispose();

        if (disposeDeviceLast) device.Dispose();
    }

    [TestCase(eDesiredConnection.DaqRef)]
    [TestCase(eDesiredConnection.OpcUa)]
    [TestCase(eDesiredConnection.WinSock)]
    public void GetAndConnectAvailableDevicesNoUsingsTest(eDesiredConnection desiredConnection)
    {
        // From CppTests

        // Create an Instance, loading modules from default location
        var instance = OpenDAQFactory.Instance();

        Console.WriteLine("instance.AvailableDevices");
        Stopwatch sw = Stopwatch.StartNew();
        var deviceInfos = instance.AvailableDevices;
        sw.Stop();
        Console.WriteLine($"  {deviceInfos.Count} devices available - elapsed in {sw.Elapsed.TotalMilliseconds} ms");
        sw.Reset();

        Debug.Print("+++> connecting...");
#if USE_LISTOBJECT
        var devices = CoreTypesFactory.CreateList<Device>(); //no-go, somehow with this the devices are being collected AFTER the instance and it crashes
#endif

        bool doAny     = (desiredConnection == eDesiredConnection.Any);
        bool doDaqRef  = doAny || (desiredConnection == eDesiredConnection.DaqRef);
        bool doOpcUa   = doAny || (desiredConnection == eDesiredConnection.OpcUa);
        bool doWinSock = doAny || (desiredConnection == eDesiredConnection.WinSock);

#if USE_IENUMERATOR_NOT_IENUMERABLE
        using var deviceInfoIterator = deviceInfos.GetEnumerator();
        while (deviceInfoIterator.MoveNext())
        {
            /*using*/ var deviceInfo = deviceInfoIterator.Current;
#else
        foreach (var deviceInfo in deviceInfos)
        {
#endif
            var deviceConnectionString = deviceInfo.ConnectionString;

            //connectible device?
            if ((doOpcUa && deviceConnectionString.StartsWith(ConnectionProtocolOpcUa, StringComparison.InvariantCultureIgnoreCase))
                || (doWinSock && deviceConnectionString.StartsWith(ConnectionProtocolWinSock, StringComparison.InvariantCultureIgnoreCase))
                || (doDaqRef && deviceConnectionString.StartsWith(ConnectionProtocolDaqRef, StringComparison.InvariantCultureIgnoreCase))) //DaqRef last (fall-back for Any)!
            {
                Console.WriteLine($"  - connecting {deviceConnectionString}");

                sw.Start();

                try
                {
                    // Connect to device and store it in a list
                    using var device = instance.AddDevice(deviceConnectionString); //when 'using' is missing, there's an access violation exception in C++ on GC.Collect()

#if USE_LISTOBJECT
                    devices.PushBack(device);
#endif

                    sw.Stop();

                    device.PrintReferenceCount();
                }
                catch (OpenDaqException ex)
                {
                    sw.Stop();
                    Console.WriteLine($"  *** connection failed: {ex.GetType().Name} - {ex}");
                }
            }
            else
            {
                Console.WriteLine($"  - {deviceConnectionString}");
            }

            deviceInfo.Dispose(); //free as it's not needed anymore
        }

        // Output the names and connection strings of all connected-to devices
#if !USE_LISTOBJECT
        var devices = instance.GetDevices();
#endif

        Debug.Print($"+++> connected {devices.Count} devices");
        Console.WriteLine($"{devices.Count} connected devices - elapsed in {sw.Elapsed.TotalMilliseconds} ms");

#if USE_IENUMERATOR_NOT_IENUMERABLE
        using var deviceIterator = devices.GetEnumerator();
        while (deviceIterator.MoveNext())
        {
            /*using*/ var device = deviceIterator.Current;
#else
        foreach (var device in devices)
        {
#endif
            var info = device.Info;
            Console.WriteLine($"  Name: '{info.Name}', Connection string: '{info.ConnectionString}'");

            device.PrintReferenceCount();
            device.Dispose(); //because right now there is an issue with GC collecting all devices when collecting 'Instance'
        }

        //cleanup (not in C++ code example due to Smart-Pointers)
//#if USE_IENUMERATOR_NOT_IENUMERABLE
//        deviceIterator = devices.GetEnumerator();
//        while (deviceIterator.MoveNext())
//        {
//            using var device = deviceIterator.Current;
//#else
//            foreach (var device in devices)
//        {
//#endif
//            instance.RemoveDevice(device);
//            device.Dispose();
//        }
//        devices.Clear();
    }

    [TestCase(eDesiredConnection.DaqRef)]
    [TestCase(eDesiredConnection.OpcUa)]
    public void GetAvailableChannelsTest(eDesiredConnection desiredConnection)
    {
        //Hint: below it makes no sense checking objects for null since the functions throw 'OpenDaqException' in case of an error ( Result.Failed(errorCode) )
        //Hint: using statements are used below that the objects get dereferenced properly when losing scope (calling Dispose() for each "using" object at the end of this method)

        using Instance daqInstance = OpenDAQFactory.Instance(".");

        using var addedDevice = ConnectFirstAvailableDevice(daqInstance, desiredConnection, doLog: true);

        var foundDevicesAfterAdd = daqInstance.GetDevices();
        Console.WriteLine($"  found {foundDevicesAfterAdd.Count} devices after AddDevice");

        Console.WriteLine("addedDevice.GetChannelsRecursive()");
        var channels = addedDevice.GetChannelsRecursive();

        ulong channelCount = (ulong)channels.Count;
        Console.WriteLine($"  found {channelCount} channels");
        Assert.That(channelCount, Is.GreaterThanOrEqualTo(1)); //crashes because no channels available and then the GC issue kicks in

        using (var channelIterator = channels.GetEnumerator())
        {
            int i = 0;
            while (channelIterator.MoveNext() && (i++ < 50))
            {
                using var channel = channelIterator.Current;
                var channelName   = channel.Name;
                var localId       = channel.LocalId;
                var globalId      = channel.GlobalId;

                Console.WriteLine($"  - {i,2}: {channelName} ({localId}) ({globalId})");
            }
        }

        //Console.WriteLine("daqInstance.RemoveDevice()");
        //daqInstance.RemoveDevice(addedDevice);
        //addedDevice.Dispose(); //because right now there is an issue with GC collecting all devices with collecting 'Instance'

        //var foundDevicesAfterRemove = daqInstance.GetDevices();
        //Console.WriteLine($"  found {foundDevicesAfterRemove.Count} devices after RemoveDevice");

        //foundDevicesAfterRemove.Dispose();
        //channels.Dispose();
        //foundDevicesAfterAdd.Dispose();

        ////not necessary here due to using statement above
        ////daqInstance.Dispose();
        ////Assert.That(daqInstance.IsDisposed, Is.True);
    }

    [TestCase(eDesiredConnection.DaqRef)]
    [TestCase(eDesiredConnection.OpcUa)]
    public void GetAvailableSignalsTest(eDesiredConnection desiredConnection)
    {
        //Hint: below it makes no sense checking objects for null since the functions throw 'OpenDaqException' in case of an error ( Result.Failed(errorCode) )
        //Hint: using statements are used below that the objects get dereferenced properly when losing scope (calling Dispose() for each "using" object at the end of this method)

        using Instance daqInstance = OpenDAQFactory.Instance(".");

        using var addedDevice = ConnectFirstAvailableDevice(daqInstance, desiredConnection, doLog: true);

        var foundDevicesAfterAdd = daqInstance.GetDevices();
        Console.WriteLine($"  found {foundDevicesAfterAdd.Count} devices after AddDevice");

        Console.WriteLine("addedDevice.GetSignalsRecursive()");
        var signals = addedDevice.GetSignalsRecursive();

        int signalCount = signals.Count;
        Console.WriteLine($"  found {signalCount} signals");
        Assert.That(signalCount, Is.GreaterThanOrEqualTo(1));

        using (var signalIterator = signals.GetEnumerator())
        {
            int i = 0;
            while (signalIterator.MoveNext() && (i++ < 10))
            {
                using var signal = signalIterator.Current;
                var signalName   = signal.Name;
                var localId      = signal.LocalId;
                var globalId     = signal.GlobalId;

                Console.WriteLine($"  - {i,2}: {signalName} ({localId}) ({globalId})");
            }
        }

        //not necessary here due to using statement above
        //daqInstance.Dispose();
        //Assert.That(daqInstance.IsDisposed, Is.True);
    }

    [TestCase(eDesiredConnection.DaqRef, int.MinValue)]
    [TestCase(eDesiredConnection.DaqRef, ulong.MinValue)]
    [TestCase(eDesiredConnection.DaqRef, double.MinValue)]
    [TestCase(eDesiredConnection.OpcUa, int.MinValue)]
    [TestCase(eDesiredConnection.OpcUa, ulong.MinValue)]
    [TestCase(eDesiredConnection.OpcUa, double.MinValue)]
    public void StreamReaderTest<TValueType>(eDesiredConnection desiredConnection, TValueType _)
        where TValueType : struct
    {
        //Hint: below it makes no sense checking objects for null since the functions throw 'OpenDaqException' in case of an error ( Result.Failed(errorCode) )
        //Hint: using statements are used below that the objects get dereferenced properly when losing scope (calling Dispose() for each "using" object at the end of this method)

        using var daqInstance = OpenDAQFactory.Instance(".");

        using var addedDevice = ConnectFirstAvailableDevice(daqInstance, desiredConnection);

        /*
            // Output 40 samples using reader
            using namespace std::chrono_literals;
            daq::StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(device.getSignals(false)[0]);

            // Allocate buffer for reading double samples
            double samples[100];
            for (int i = 0; i < 40; ++i)
            {
                std::this_thread::sleep_for(25ms);

                // Read up to 100 samples, storing the amount read into `count`
                daq::SizeT count = 100;
                reader.read(samples, &count);
                if (count > 0)
                    std::cout << samples[count - 1] << std::endl;
            }
         */

        Console.WriteLine("addedDevice.GetSignalsRecursive()");
        var signals = addedDevice.GetSignalsRecursive(); //Hint: here addedDevice.GetSignals() returns an empty list

        ulong signalCount = (ulong)signals.Count;
        Console.WriteLine($"  found {signalCount} signals");
        Assert.That(signalCount, Is.GreaterThanOrEqualTo(1));

        int analogSignalNo = 0; //one-based
        int signalNo = 0;
        using (var signalIterator = signals.GetEnumerator())
        {
            int i = 0;
            while (signalIterator.MoveNext() && (i++ < 10))
            {
                using var sig = signalIterator.Current;
                var sigName = sig.Name;

                Console.WriteLine($"  - {i,2}: {sigName}");

                ++signalNo;
                //if ((sigNameTxt.Equals("AnalogValue") || sigNameTxt.Equals("ai0")) && (analogSignalNo == 0))
                if ((sigName.Equals("DigitalValue") || sigName.Equals("ai0")) && (analogSignalNo == 0))
                    analogSignalNo = signalNo;
            }
        }
        Assert.That(analogSignalNo, Is.GreaterThanOrEqualTo(1));

        //Console.WriteLine("addedDevice.GetChannels()");
        //using var channels = addedDevice.GetChannels();
        //Console.WriteLine($"  {channels.Count} channels available");

        //using var firstChannel = channels[0];
        //using var channelName = firstChannel.Name;
        //Console.WriteLine($"  - using channel 0 '{channelName}'");

        //Console.WriteLine("firstChannel.GetSignals()");
        //using var signals = firstChannel.GetSignals();
        //Console.WriteLine($"  {signals.Count} signals available");

        //take the first available signal
        using var signal = signals[analogSignalNo - 1];
        var signalName = signal.Name;
        Console.WriteLine($"  using signal {analogSignalNo} '{signalName}'");

        Console.WriteLine("OpenDAQFactory.CreateStreamReader()");
        //here using-sub-scope to free resources asap
        nuint count = 500;
        TValueType defaultValue = (TValueType)Convert.ChangeType(99999, typeof(TValueType));
        TValueType[] samples = new TValueType[count];
        Array.Fill(samples, defaultValue);
        nuint samplesCount = 0;
        using (var reader = OpenDAQFactory.CreateStreamReader<TValueType>(signal))
        {
            Console.WriteLine($"  ValueReadType = {reader.ValueReadType}, DomainReadType = {reader.DomainReadType}");

            for (int readBlockNo = 0; readBlockNo < 10; ++readBlockNo)
            {
                //ToDo: do we get buffer overrun when we just leave reader open for too long without reading?
                Thread.Sleep(25);

                //read up to 'count' samples, storing the amount read into array 'samples'
                count = (nuint)samples.Length; //reset to array size
                reader.Read(samples, ref count, 1000);
                samplesCount += count;

                string values = (count == 0) ? string.Empty : $"(0: {samples[0]:+0.000;-0.000} ... {count-1}: {samples[count - 1]:+0.000;-0.000;±0.000})";
                Console.WriteLine($"  Block {readBlockNo + 1,2} read {count,3} values {values}");
            }
        }

        Assert.That(samplesCount, Is.GreaterThan((nuint)0), "*** No samples received.");

        //......................

        //not necessary here due to using statement above
        //daqInstance.Dispose();
        //Assert.That(daqInstance.IsDisposed, Is.True);
    }

    [TestCase(eDesiredReader.Stream, double.MinValue)]
    [TestCase(eDesiredReader.Tail, double.MinValue)]
    [TestCase(eDesiredReader.Block, double.MinValue)]
    //[TestCase(eDesiredReader.Multi, double.MinValue)]
    public void TimeReaderTest<TValueType>(eDesiredReader desiredReader, TValueType _)
        where TValueType : struct
    {
        //Hint: below it makes no sense checking objects for null since the functions throw 'OpenDaqException' in case of an error ( Result.Failed(errorCode) )
        //Hint: using statements are used below that the objects get dereferenced properly when losing scope (calling Dispose() for each "using" object at the end of this method)

        using var daqInstance = OpenDAQFactory.Instance(".");

        using var addedDevice = ConnectFirstAvailableDevice(daqInstance, eDesiredConnection.DaqRef);

        Console.WriteLine("addedDevice.GetSignalsRecursive()");
        using var signal = addedDevice.GetSignalsRecursive()[0]; //Hint: here addedDevice.GetSignals() returns an empty list

        var signalName     = signal.Name;
        var tickResolution = signal.DomainSignal.Descriptor.TickResolution;
        Console.WriteLine($"  using signal 0 '{signalName}'");
        Console.WriteLine($"  domain signal tick-resolution = '{tickResolution.Numerator}/{tickResolution.Denominator:N0}'");

        nuint count       = 100;
        nuint blockSize   = 1;     //10 for BlockReader (see switch/case below)
        nuint historySize = count; //only for TailReader; must not be lower than count, otherwise there will always status=fail and count=0

        TValueType[] samples    = new TValueType[count];
        DateTime[]   timeStamps = new DateTime[count];

        TValueType defaultValue = (TValueType)Convert.ChangeType(99999, typeof(TValueType));
        Array.Fill(samples, defaultValue);

        SampleReader? sampleReader = null;

        Console.WriteLine($"OpenDAQFactory.Create{desiredReader}Reader()");
        switch (desiredReader)
        {
            case eDesiredReader.Block:
                blockSize = 10;
                Console.WriteLine($"  buffer size = {count}");
                Console.WriteLine($"  block size  = {blockSize}");
                sampleReader = OpenDAQFactory.CreateBlockReader<TValueType, Int64>(signal, blockSize); //not working when data not available (native exception)
                break;

            //case eDesiredReader.Multi:
            //    sampleReader = OpenDAQFactory.CreateMultiReader<TValueType, Int64>(signals);
            //    break;

            case eDesiredReader.Stream:
                Console.WriteLine($"  buffer size = {count}");
                sampleReader = OpenDAQFactory.CreateStreamReader<TValueType, Int64>(signal);
                break;

            case eDesiredReader.Tail:
                Console.WriteLine($"  buffer size  = {count}");
                Console.WriteLine($"  history size = {historySize}");
                sampleReader = OpenDAQFactory.CreateTailReader<TValueType, Int64>(signal, historySize); //not working when historySize < count (always reading count=0)
                break;

            default:
                Assert.Fail($"Desired reader not (yet) supported: {nameof(eDesiredReader)}.{desiredReader}");
                break;
        } //switch

        Assert.That(sampleReader, Is.Not.Null, "*** No SampleReader instance available.");

        Console.WriteLine("OpenDAQFactory.CreateTimeReader()");
        TimeReader timeReader = OpenDAQFactory.CreateTimeReader(sampleReader, signal);
        int readFailures = 0;

        Console.WriteLine($"  ValueReadType = {sampleReader.ValueReadType}, DomainReadType = {sampleReader.DomainReadType}");

        nuint readSamplesCount = 0;

        Console.WriteLine("looping timeReader.ReadWithDomain()");
        const int sleepTime = 25;
        for (int readBlockNo = 0; readBlockNo < 10; ++readBlockNo)
        {
            //read up to 'count' samples or blocks, storing the amount read into array 'samples' and 'timeStamps'
            count = (nuint)samples.Length / blockSize; //reset to array size or block count

            //ToDo: do we get buffer overrun when we just leave reader open for too long without reading?

            //wait until there are enough samples available for our buffers (up to one second)
            int loopCount = 1000 / sleepTime;
            do
            {
                Thread.Sleep(sleepTime);
            }
            while ((sampleReader.Empty()) && (--loopCount > 0));

            nuint samplesOrBlocksCountAvailable = sampleReader.AvailableCount;

            Console.WriteLine($"  Block {readBlockNo + 1,2}: waited {1000 - (loopCount * sleepTime)}ms -> {samplesOrBlocksCountAvailable} of {count} available");

            Assert.That(!sampleReader.Empty(), "*** No data available."); //somehow using Is.GreaterThan((nuint)0) is giving a runtime error here

            using var status = timeReader.ReadWithDomain(samples, timeStamps, ref count, 1000);

            nuint samplesCount = count * blockSize; //recalculate for BlockReader (otherwise x1)
            readSamplesCount += samplesCount;

            if (status?.ReadStatus == ReadStatus.Ok)
            {
                string valueString = (samplesCount == 0)
                                     ? string.Empty
                                     : $"(0: {samples[0]:+0.000;-0.000} ... {samplesCount - 1}: {samples[samplesCount - 1]:+0.000;-0.000;±0.000} @ {timeStamps[0]:yyyy-MM-dd HH:mm:ss.fffffff} ... {timeStamps[samplesCount - 1]:HH:mm:ss.fffffff})";

                Console.WriteLine($"            read {samplesCount,3} values {valueString}");
            }
            else if (status?.ReadStatus == ReadStatus.Event)
            {
                Console.WriteLine($"            event occurred'");
            }
            else
            {
                ++readFailures;
                Console.WriteLine($"            read failed with ReadStatus = '{status?.ReadStatus}'");
                break;
            }
        }

        if (sampleReader != null)
        {
            sampleReader.Dispose();
            sampleReader = null;
        }

        Assert.That(readFailures, Is.EqualTo(0), "*** There have been read failures.");
        Assert.That(readSamplesCount, Is.GreaterThan((nuint)0), "*** No samples received.");
    }

    //[TestCase("AI_01",  "AnalogValue",  eDesiredConnection.OpcUa)]
    //[TestCase("DIO_7",  "DigitalValue", eDesiredConnection.OpcUa)]
    [TestCase("refch0", "ai0",          eDesiredConnection.OpcUa)]
    [TestCase("refch0", "ai0",          eDesiredConnection.DaqRef)]
    public void RendererTest(string searchChannelName, string signalSearchName, eDesiredConnection desiredConnection)
    {
        Stopwatch sw = Stopwatch.StartNew();

        using var daqInstance = OpenDAQFactory.Instance(".");

        sw.Stop(); Console.WriteLine("----- elapsed in {0} ms", sw.Elapsed.TotalMilliseconds); sw.Restart();

        using var addedDevice = ConnectFirstAvailableDevice(daqInstance, desiredConnection);

        sw.Stop(); Console.WriteLine("----- elapsed in {0} ms", sw.Elapsed.TotalMilliseconds); sw.Restart();

        Console.WriteLine("addedDevice.GetChannels()");
        var channels = addedDevice.GetChannels();
        Console.WriteLine($"  {channels.Count} channels available");

        sw.Stop(); Console.WriteLine("----- elapsed in {0} ms", sw.Elapsed.TotalMilliseconds); sw.Restart();

        //search for channel with searchChannelName
        int foundChannelNo = 0; //one-based
        int channelNo = 0;
        using (var channelIterator = channels.GetEnumerator())
        {
            //int i = 0;
            while (channelIterator.MoveNext() /*&& (i++ < 5)*/)
            {
                using var chan = channelIterator.Current;
                var chanName = chan.Name;

                Console.WriteLine($"  - {channelNo,2}: {chanName}");

                ++channelNo;
                if (chanName.Equals(searchChannelName) && (foundChannelNo == 0))
                    foundChannelNo = channelNo; //store first occurrence
            }
        }

        sw.Stop(); Console.WriteLine("----- elapsed in {0} ms", sw.Elapsed.TotalMilliseconds); sw.Restart();

        //Assert.That(foundChannelNo, Is.GreaterThanOrEqualTo(1), "channel '{0}' not found", searchChannelName);
        if (foundChannelNo == 0)
        {
            Assert.Warn($"Test skipped: Channel '{searchChannelName}' not found");
            return;
        }

        using var channel = channels[foundChannelNo - 1];
        var channelName = channel.Name;
        Console.WriteLine($"  > using channel {foundChannelNo} '{channelName}'");

        ShowAllProperties(channel!, "channel");

        sw.Stop(); Console.WriteLine("----- elapsed in {0} ms", sw.Elapsed.TotalMilliseconds); sw.Restart();

        Console.WriteLine("channel.GetSignals()");
        var signals = channel.GetSignals();
        Console.WriteLine($"  {signals.Count} signals available");

        //search for signal with searchSignalName
        int foundSignalNo = 0; //one-based
        int signalNo = 0;
        using (var signalIterator = signals.GetEnumerator())
        {
            //int i = 0;
            while (signalIterator.MoveNext() /*&& (i++ < 5)*/)
            {
                using var sig = signalIterator.Current;
                var sigName = sig.Name;

                Console.WriteLine($"  - {signalNo,2}: {sigName}");

                ++signalNo;
                if (sigName.Equals(signalSearchName) && (foundSignalNo == 0))
                    foundSignalNo = signalNo; //store first occurrence
            }
        }

        sw.Stop(); Console.WriteLine("----- elapsed in {0} ms", sw.Elapsed.TotalMilliseconds); sw.Restart();

        //Assert.That(foundSignalNo, Is.GreaterThanOrEqualTo(1), "channel '{0}' has no signal '{1}'", channelName, signalSearchName);
        if (foundSignalNo == 0)
        {
            Assert.Warn($"Test skipped: Channel '{channelName}' has no signal '{signalSearchName}'");
            return;
        }

        using var signal = signals[foundSignalNo - 1];
        var signalName = signal.Name;
        Console.WriteLine($"  > using signal {foundSignalNo} '{signalName}'");

        ShowAllProperties(signal!, "signal");

        sw.Stop(); Console.WriteLine("----- elapsed in {0} ms", sw.Elapsed.TotalMilliseconds); sw.Restart();

        if (signalSearchName != "DigitalValue")
        {
            Console.WriteLine("channel.SetPropertyValue");

            using StringObject frequencyPropertyName = "Frequency";
            using FloatObject frequencyValue = 5d;
            Console.WriteLine($"  SetPropertyValue({frequencyPropertyName}, {frequencyValue.Value})");
            channel.SetPropertyValue(frequencyPropertyName, frequencyValue);
            //channel.SetPropertyValue("Frequency", 5d);

            using StringObject amplitudePropertyName = "Amplitude";
            using FloatObject amplitudeValue = 10d;
            Console.WriteLine($"  SetPropertyValue({amplitudePropertyName}, {amplitudeValue.Value})");
            channel.SetPropertyValue(amplitudePropertyName, amplitudeValue);

//            if (desiredConnection == eDesiredConnection.OpcUa)
//            {
//                using StringObject inputMuxPropertyName = "InputMux";
//                using IntegerObject inputMuxValue = 7;
//                Console.WriteLine($"  SetPropertyValue({inputMuxPropertyName}, {inputMuxValue.Value})");
//                channel.SetPropertyValue(inputMuxPropertyName, inputMuxValue);
//            }
        }
        //else
        //{
        //    using StringObject rangePropertyName = "Range";
        //    using var rangeProperty = channel.GetPropertyValue(rangePropertyName);
        //    using var rangeValueOld = rangeProperty.QueryInterface<IntegerObject>();
        //    long range = rangeValueOld.Value;
        //    Console.WriteLine($"Range = {range}"); //-> 0

        //    using StringObject dioPropertyName = "DigitalOutputValue";
        //    using var dioProperty = channel.GetPropertyValue(dioPropertyName);
        //    using var dioValueOld = dioProperty.QueryInterface<IntegerObject>();
        //    long dio = dioValueOld.Value;
        //    Console.WriteLine($"DigitalOutputValue = {dio}"); //-> 0
        //}

        sw.Stop(); Console.WriteLine("----- elapsed in {0} ms", sw.Elapsed.TotalMilliseconds); sw.Restart();

        GetFunctionBlocks(daqInstance, out FunctionBlock? renderer, out FunctionBlock? statistics);

        sw.Stop(); Console.WriteLine("----- elapsed in {0} ms", sw.Elapsed.TotalMilliseconds); sw.Restart();

        Assert.Multiple(() =>
        {
            Assert.That(renderer, Is.Not.Null, "No renderer found");
            Assert.That(statistics, Is.Not.Null, "No statistics found");
        });

        ShowAllProperties(renderer!, "renderer");
        ShowAllProperties(statistics!, "statistics");

        sw.Stop(); Console.WriteLine("----- elapsed in {0} ms", sw.Elapsed.TotalMilliseconds); sw.Restart();

        Console.WriteLine("renderer!.SetPropertyValue");

        using StringObject durationPropertyName = "Duration";
        using FloatObject durationValue = 5d;
        Console.WriteLine($"  SetPropertyValue({durationPropertyName}, {durationValue.Value})");
        renderer!.SetPropertyValue(durationPropertyName, durationValue);

        Console.WriteLine("statistics!.SetPropertyValue");

        using StringObject blockSizePropertyName = "BlockSize";
        using IntegerObject blockSizeValue = 20;
        Console.WriteLine($"  SetPropertyValue({blockSizePropertyName}, {blockSizeValue.Value})");
        statistics!.SetPropertyValue(blockSizePropertyName, blockSizeValue);

        sw.Stop(); Console.WriteLine("----- elapsed in {0} ms", sw.Elapsed.TotalMilliseconds); sw.Restart();

        Console.WriteLine("renderer!.GetInputPorts()");
        var inputPorts = renderer!.GetInputPorts();
        Console.WriteLine($"  {inputPorts.Count} input ports available");

        using var inputPort = inputPorts[0];
        var inputPortName = inputPort.Name;
        Console.WriteLine($"  > using input port 0 '{inputPortName}'");

        Console.WriteLine("-> inputPort.Connect(signal)");
        inputPort.Connect(signal);

        Console.WriteLine("statistics!.GetSignals()");
        var statisticsSignals = statistics!.GetSignals();
        Console.WriteLine($"  {statisticsSignals.Count} statistics signals available");

        using var statisticsSignal = statisticsSignals[0];
        var statisticsSignalName = statisticsSignal.Name;
        Console.WriteLine($"  > using statistics signal 0 '{statisticsSignalName}'");

        Console.WriteLine("statistics!.GetInputPorts()");
        var statisticsInputPorts = statistics!.GetInputPorts();
        Console.WriteLine($"  {statisticsInputPorts.Count} statistics input ports available");

        using var statisticsInputPort = statisticsInputPorts[0];
        var statisticsInputPortName = statisticsInputPort.Name;
        Console.WriteLine($"  > using statistics input port 0 '{statisticsInputPortName}'");

        Console.WriteLine("-> statisticsInputPort.Connect(statisticsSignal)");
        statisticsInputPort.Connect(statisticsSignal);

        sw.Stop(); Console.WriteLine("----- elapsed in {0} ms", sw.Elapsed.TotalMilliseconds); //sw.Restart();

        Console.WriteLine("loop");

        if (signalSearchName == "DigitalValue")
        {
            Thread.Sleep(10000);
        }
        else
        {
            string amplitudePropertyName = "Amplitude";
            double amplitudeStep = 0.1d;
            Stopwatch stopWatch = Stopwatch.StartNew();
            while (stopWatch.Elapsed.Seconds < 10)
            {
                using var amplitudeValueObjectOld = channel.GetPropertyValue(amplitudePropertyName);
                using var amplitudeValueOld = amplitudeValueObjectOld.Cast<FloatObject>();
                //double amplitude = channel.GetPropertyValue("Amplitude");

                double amplitude = amplitudeValueOld;
                if (9.95d < amplitude || amplitude < 1.05d)
                {
                    amplitudeStep *= -1d;
                }
                amplitude += amplitudeStep;

                using var amplitudeValueNew = (FloatObject)amplitude;
                channel.SetPropertyValue(amplitudePropertyName, amplitudeValueNew);
                //channel.SetPropertyValue("Amplitude", amplitude);
                Thread.Sleep(10);
            }
        }

        //cleanup
        //ToDo: do added devices and function blocks have to be removed? it seems not to be so, since my tests exit without still tracked objects
        renderer?.Dispose();
        statistics?.Dispose();


        //=== local functions =====================================================================


        static void GetFunctionBlocks(Instance daqInstance, out FunctionBlock? renderer, out FunctionBlock? statistics)
        {
            renderer = null;
            statistics = null;

            Console.WriteLine("daqInstance.GetAvailableFunctionBlocks()");
            var availableFunctionBlockInfos = daqInstance.AvailableFunctionBlockTypes;

            Console.WriteLine($"  {availableFunctionBlockInfos.Count} function blocks available");

            var functionBlockKeys = availableFunctionBlockInfos.Keys;

            using (var keyIterator = functionBlockKeys.GetEnumerator())
            {
                while (keyIterator.MoveNext())
                {
                    using var keyObject = keyIterator.Current;
                    string key = keyObject;

                    using var functionBlockInfo = availableFunctionBlockInfos[keyObject];
                    var functionBlockId = functionBlockInfo.Id;
                    Console.WriteLine($"  - '{key}' ({functionBlockId})");

                    using var propertyObject = CoreObjectsFactory.CreatePropertyObject();

                    switch (functionBlockId.ToString())
                    {
                        case "ref_fb_module_renderer":
                            renderer = daqInstance.AddFunctionBlock(functionBlockId, propertyObject);
                            break;
                        case "ref_fb_module_statistics":
                            statistics = daqInstance.AddFunctionBlock(functionBlockId, propertyObject);
                            break;
                    }
                }
            }
        }
    }

    [Test]
    public void FunctionBlockTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");
        //using var addedDevice = ConnectFirstAvailableDevice(daqInstance, eDesiredConnection.DaqRef);

        Console.WriteLine("daqInstance.GetAvailableFunctionBlocks()");
        var availableFunctionBlockInfos = daqInstance.AvailableFunctionBlockTypes;

        Console.WriteLine($"  {availableFunctionBlockInfos.Count} function blocks available");

        var functionBlockKeys = availableFunctionBlockInfos.Keys;

        using (var keyIterator = functionBlockKeys.GetEnumerator())
        {
            while (keyIterator.MoveNext())
            {
                using var keyObject = keyIterator.Current;
                string key = keyObject;

                using var functionBlockInfo = availableFunctionBlockInfos[keyObject];
                var functionBlockId = functionBlockInfo.Id;
                Console.WriteLine($"  - '{key}' ({functionBlockId})");

                Console.WriteLine($"    add to daqInstance");
                using var functionBlock = daqInstance.AddFunctionBlock(functionBlockId);

                using var properties = functionBlock.AllProperties;
                Console.WriteLine($"    {properties.Count} properties available");

                foreach (var property in properties)
                {
                    string valueType   = property.ValueType.ToString();
                    string? keyType    = (property.ValueType == CoreType.ctDict) ? property.KeyType.ToString() : null;
                    string? itemType   = (property.ValueType == CoreType.ctDict) || (property.ValueType == CoreType.ctList) ? property.ItemType.ToString() : null;
                    string? structType = (property.ValueType == CoreType.ctStruct) ? property.StructType.Name : null;
                    Console.WriteLine($"    - {property.Name} ({valueType}: {keyType}; {itemType}; {structType})");

                    using var selectionValues = property.SelectionValues;

                    if (selectionValues != null)
                    {
                        using var list = selectionValues.Cast<ListObject<BaseObject>>();
                        using var dict = selectionValues.Cast<DictObject<BaseObject, BaseObject>>();

                        if (list != null)
                        {
                            Console.WriteLine($"      {list.Count} selection values in list");
                        }
                        else if (dict != null)
                        {
                            Console.WriteLine($"      {dict.Count} selection values in dictionary");
                        }
                        else
                        {
                            Console.WriteLine("      selection values not a list nor dictionary");
                        }
                    }
                }

                var statusContainer = functionBlock.StatusContainer;
                var statuses        = statusContainer.Statuses;
                Console.WriteLine($"    {statuses.Count} statuses available");
                foreach (string statusName in statuses.Keys)
                {
                    Console.WriteLine($"    - {statusName}");
                }
            }
        }

        //*** always empty list as none connected ***

        //var channel = addedDevice.GetChannels()[0];

        //Console.WriteLine("channel.GetFunctionBlocks()");
        //var functionBlocks = channel.GetFunctionBlocks();

        //Console.WriteLine($"  {functionBlocks.Count} function blocks for channel {channel.Name}");

        //foreach (var functionBlock in functionBlocks)
        //{
        //    Console.WriteLine($"  - '{functionBlock.Name}' ({functionBlock.LocalId})");
        //}
    }

    //[Test]
    public void FilterPropertyTest()
    {
    }

    //[Test]
    public void SensorPropertyTest()
    {
    }

    //[Test]
    public void EventTest()
    {
        //ToDo: how, "what" is using events?
    }

    [Test]
    public void GCTest()
    {
        for (int i = 0; i < 10; ++i)
        {
            GCTestFunction(i);
            Thread.Sleep(1000);
        }

        //do something (or just wait)
        Thread.Sleep(1000);
        //here the GC still has not done anything with the above objects, but who can tell for how long to wait?

        GC.Collect();
        GC.WaitForPendingFinalizers();

        //there should be no more open references here -> BaseTearDown() will tell
    }

    private void GCTestFunction(int i)
    {
        //here we instantiate a StringObject having a positive reference count in C++
        StringObject str = $"Test string {i}";
        Console.WriteLine((string)str);

        //the reference counter will not be reduced here automatically leaving the scope, depending on the non-deterministic garbage collector to call the finalizer

        //using GC.Collect() here will remove most of the references but not necessarily all (in time)

        // Check Signal Data Descriptor's Sample Type and name
    }
}
