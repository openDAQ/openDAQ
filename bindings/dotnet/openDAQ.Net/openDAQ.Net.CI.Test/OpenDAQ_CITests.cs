using Daq.Core;
using Daq.Core.Objects;
using Daq.Core.OpenDAQ;
using Daq.Core.Types;


namespace openDaq.Net.Test;


public class OpenDAQ_CITests : OpenDAQTestsBase
{
    [SetUp]
    public void Setup()
    {
        //leave GarbageCollector alone and don't check ref-count
        base.DontCollectAndFinalize();
        base.DontCheckAliveObjectCount();
    }

    private const string ConnectionProtocolDaqRef = "daqref://";


    private Device? ConnectFirstDaqRefDevice()
    {
        // Create an Instance, loading modules from default location
        var instance = OpenDAQFactory.Instance();

        var deviceInfos = instance.GetAvailableDevices();

        foreach (var deviceInfo in deviceInfos)
        {
            var deviceConnectionString = deviceInfo.GetConnectionString();

            //connectible device?
            if (!deviceConnectionString.StartsWith(ConnectionProtocolDaqRef, StringComparison.InvariantCultureIgnoreCase))
            {
                continue;
            }

            Console.WriteLine($"  - connecting {deviceConnectionString}");
            deviceInfo.Dispose(); //free as it's not needed anymore

            try
            {
                // Connect to device and store it in a list
                using var device = instance.AddDevice(deviceConnectionString); //when 'using' is missing, there's an access violation exception in C++ on GC.Collect()

                break;
            }
            catch (OpenDaqException ex)
            {
                Console.WriteLine($"  *** connection failed: {ex.GetType().Name} - {ex}");
            }
        }

        // Output the names and connection strings of all connected-to devices
        var devices = instance.GetDevices();

        return devices?.FirstOrDefault();
    }


    [Test]
    public void Test_0000_GetVersion()
    {
        Version coreTypesVersion = new();
        Version coreObjectsVersion = new();
        Version openDaqVersion = new();

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
    public void Test_0001_ListObjectDisposeTest()
    {
        Console.WriteLine("> creating...");
        var list = CoreTypesFactory.CreateList<BaseObject>();
        var obj = CoreTypesFactory.CreateBaseObject();

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
    public void Test_0101_InstanceManualDispose()
    {
        //instruct TearDown function not to collect and finalize managed objects explicitly
        base.DontCollectAndFinalize();

        Instance daqInstance = OpenDAQFactory.Instance(".");
        Assert.That(daqInstance.IsDisposed, Is.False);

        //can do something with 'daqInstance' here
        using var info = daqInstance.GetInfo();

        var name = info.GetName();
        Console.WriteLine($"daqInstance name = '{name}'");

        //finally free managed resources (release reference)
        daqInstance.Dispose();
        Assert.That(daqInstance.IsDisposed, Is.True);
    }

    [Test]
    public void Test_0102_InstanceAutoDispose1()
    {
        //instruct TearDown function not to collect and finalize managed objects explicitly
        base.DontCollectAndFinalize();

        Instance daqInstance;

        using (daqInstance = OpenDAQFactory.Instance("."))
        {
            Assert.That(daqInstance.IsDisposed, Is.False);

            //can do something with 'daqInstance' here
            using var info = daqInstance.GetInfo();

            var name = info.GetName();
            Console.WriteLine($"daqInstance name = '{name}'");
        } //losing scope here, automatically calling Dispose() and thus freeing managed resources (release reference)

        Assert.That(daqInstance.IsDisposed, Is.True);
    }

    [Test]
    public void Test_0103_InstanceAutoDispose2()
    {
        //instruct TearDown function not to collect and finalize managed objects explicitly
        base.DontCollectAndFinalize();

        using Instance daqInstance = OpenDAQFactory.Instance(".");

        Assert.That(daqInstance.IsDisposed, Is.False);

        //can do something with 'daqInstance' here
        using var info = daqInstance.GetInfo();

        var name = info.GetName();
        Console.WriteLine($"daqInstance name = '{name}'");

        //losing scope at the end of this method, automatically calling Dispose() and thus freeing managed resources (release reference)
    }


    [Test]
    public void Test_0201_GetAvailableDevices()
    {
        using Instance daqInstance = OpenDAQFactory.Instance(".");

        Console.WriteLine("> daqInstance.GetAvailableDevices()");
        var availableDevicesInfos = daqInstance.GetAvailableDevices();

        Console.WriteLine($"  {availableDevicesInfos.Count} devices available");

        //list all devices
        foreach (var deviceInfo in availableDevicesInfos)
        {
            var deviceName = deviceInfo.GetName();
            var connectionString = deviceInfo.GetConnectionString();
            string model = deviceInfo.GetPropertyValue("model");
            string deviceClass = deviceInfo.GetPropertyValue("deviceClass");
            string softwareRevision = deviceInfo.HasProperty("softwareRevision") ? deviceInfo.GetPropertyValue("softwareRevision") : "n/a";

            Console.WriteLine($"  - Name = '{deviceName}', Connection string = '{connectionString}'");
            Console.WriteLine($"    model = '{model}', deviceClass = '{deviceClass}', softwareRevision = '{softwareRevision}'");

            //ShowAllProperties(deviceInfo, nameof(deviceInfo));
        }
    }

    [Test]
    public void Test_0202_GetAndConnectDaqRefDevice()
    {
        // Create an Instance, loading modules from default location
        var instance = OpenDAQFactory.Instance();

        Console.WriteLine("instance.GetAvailableDevices()");
        var deviceInfos = instance.GetAvailableDevices();

        foreach (var deviceInfo in deviceInfos)
        {
            var deviceConnectionString = deviceInfo.GetConnectionString();

            //connectible device?
            if (!deviceConnectionString.StartsWith(ConnectionProtocolDaqRef, StringComparison.InvariantCultureIgnoreCase))
                continue;

            Console.WriteLine($"  - connecting {deviceConnectionString}");

            try
            {
                // Connect to device and store it in a list
                using var device = instance.AddDevice(deviceConnectionString); //when 'using' is missing, there's an access violation exception in C++ on GC.Collect()

                device.PrintReferenceCount();
            }
            catch (OpenDaqException ex)
            {
                Console.WriteLine($"  *** connection failed: {ex.GetType().Name} - {ex}");
            }

            deviceInfo.Dispose(); //free as it's not needed anymore
        }

        // Output the names and connection strings of all connected-to devices
        var devices = instance.GetDevices();

        Console.WriteLine($"{devices.Count} connected devices");

        foreach (var device in devices)
        {
            var info = device.GetInfo();
            Console.WriteLine($"  Name: '{info.GetName()}', Connection string: '{info.GetConnectionString()}'");

            device.PrintReferenceCount();
            //device.Dispose(); //because right now there is an issue with GC collecting all devices when collecting 'Instance'
        }
    }


    [Test]
    public void Test_0301_GetAvailableChannelsTest()
    {
        using Instance daqInstance = OpenDAQFactory.Instance(".");

        using var device = ConnectFirstDaqRefDevice();
        Assert.That(device, Is.Not.Null);

        Console.WriteLine("addedDevice.GetChannelsRecursive()");
        var channels = device.GetChannelsRecursive();

        ulong channelCount = (ulong)channels.Count;
        Console.WriteLine($"  found {channelCount} channels");
        Assert.That(channelCount, Is.GreaterThanOrEqualTo(1));

        using (var channelIterator = channels.GetEnumerator())
        {
            int i = 0;
            while (channelIterator.MoveNext() && (i++ < 50))
            {
                using var channel = channelIterator.Current;
                var channelName   = channel.GetName();
                var localId       = channel.GetLocalId();
                var globalId      = channel.GetGlobalId();

                Console.WriteLine($"  - {i,2}: {channelName} ({localId}) ({globalId})");
            }
        }
    }

    [Test]
    public void Test_0302_GetAvailableSignalsTest()
    {
        using Instance daqInstance = OpenDAQFactory.Instance(".");

        using var device = ConnectFirstDaqRefDevice();
        Assert.That(device, Is.Not.Null);

        Console.WriteLine("addedDevice.GetSignalsRecursive()");
        var signals = device.GetSignalsRecursive();

        int signalCount = signals.Count;
        Console.WriteLine($"  found {signalCount} signals");
        Assert.That(signalCount, Is.GreaterThanOrEqualTo(1));

        using (var signalIterator = signals.GetEnumerator())
        {
            int i = 0;
            while (signalIterator.MoveNext() && (i++ < 10))
            {
                using var signal = signalIterator.Current;
                var signalName   = signal.GetName();
                var localId      = signal.GetLocalId();
                var globalId     = signal.GetGlobalId();

                Console.WriteLine($"  - {i,2}: {signalName} ({localId}) ({globalId})");
            }
        }
    }

    [Test]
    public void Test_0303_GetAvailableFunctionBlocks()
    {
        using Instance daqInstance = OpenDAQFactory.Instance(".");

        using var device = ConnectFirstDaqRefDevice();
        Assert.That(device, Is.Not.Null);

        Console.WriteLine("daqInstance.GetAvailableFunctionBlocks()");
        var availableFunctionBlockInfos = daqInstance.GetAvailableFunctionBlockTypes();
        Assert.Multiple(() =>
        {
            Assert.That(availableFunctionBlockInfos, Is.Not.Null);
            Assert.That(availableFunctionBlockInfos.Count, Is.GreaterThanOrEqualTo(1));
        });

        Console.WriteLine($"  {availableFunctionBlockInfos.Count} function blocks available");

        foreach (var key in availableFunctionBlockInfos.Keys)
        {
            using var functionBlockInfo = availableFunctionBlockInfos[key];
            var functionBlockId         = functionBlockInfo.GetId();

            Console.WriteLine($"  - '{key}' ({functionBlockId})");
        }
    }


    [TestCase(int.MinValue)]
    [TestCase(ulong.MinValue)]
    [TestCase(double.MinValue)]
    public void Test_0401_StreamReaderTest<TValue>(TValue _)
        where TValue : struct
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        using var device = ConnectFirstDaqRefDevice();
        Assert.That(device, Is.Not.Null);

        Console.WriteLine("addedDevice.GetSignalsRecursive()");
        var signals = device.GetSignalsRecursive();

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
                var sigName = sig.GetName();

                Console.WriteLine($"  - {i,2}: {sigName}");

                ++signalNo;
                if ((sigName.Equals("AnalogValue") || sigName.Equals("ai0")) && (analogSignalNo == 0))
                {
                    analogSignalNo = signalNo;
                }
            }
        }
        Assert.That(analogSignalNo, Is.GreaterThanOrEqualTo(1));

        //take the first available signal
        using var signal = signals[analogSignalNo - 1];
        var signalName = signal.GetName();
        Console.WriteLine($"  using signal {analogSignalNo} '{signalName}'");

        Console.WriteLine("OpenDAQFactory.CreateStreamReader()");
        //here using-sub-scope to free resources asap
        nuint count = 500;
        TValue defaultValue = (TValue)Convert.ChangeType(99999, typeof(TValue));
        TValue[] samples = new TValue[count];
        Array.Fill(samples, defaultValue);
        nuint samplesCount = 0;
        using (var reader = OpenDAQFactory.CreateStreamReader<TValue>(signal))
        {
            Console.WriteLine($"  ValueReadType = {reader.GetValueReadType()}, DomainReadType = {reader.GetDomainReadType()}");

            for (int readBlockNo = 0; readBlockNo < 10; ++readBlockNo)
            {
                //ToDo: do we get buffer overrun when we just leave reader open for too long without reading?
                Thread.Sleep(25);

                //read up to 'count' samples, storing the amount read into array 'samples'
                count = (nuint)samples.Length; //reset to array size
                reader.Read(samples, ref count, 1000);
                samplesCount += count;

                string values = (count == 0) ? string.Empty : $"(0: {samples[0]:+0.000;-0.000} ... {count - 1}: {samples[count - 1]:+0.000;-0.000;±0.000})";
                Console.WriteLine($"  Block {readBlockNo + 1,2} read {count,3} values {values}");
            }
        }

        Assert.That(samplesCount, Is.GreaterThan((nuint)0), "*** No samples received.");
    }
}
