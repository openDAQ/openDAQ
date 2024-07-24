using System.Collections.Generic;

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

    public enum eDesiredReader
    {
        Stream = 0,
        Tail,
        Block,
        Multi,
        //Packet
    }


    private Device? ConnectFirstDaqRefDevice(Instance daqInstance)
    {
        Console.WriteLine($"  {nameof(ConnectFirstDaqRefDevice)}...");

        // Get the list of available devices
        var deviceInfos = daqInstance.AvailableDevices;

        Assert.That(deviceInfos.Count, Is.GreaterThan(0));

        foreach (var deviceInfo in deviceInfos)
        {
            var deviceConnectionString = deviceInfo.ConnectionString;

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
                using var device = daqInstance.AddDevice(deviceConnectionString); //when 'using' is missing, there's an access violation exception in C++ on GC.Collect()

                break;
            }
            catch (OpenDaqException ex)
            {
                Console.WriteLine($"  *** connection failed: {ex.GetType().Name} - {ex}");
            }
        }

        // Get the list of connected devices
        var devices = daqInstance.GetDevices();

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
        Console.WriteLine($"openDAQ SDK version     = {openDaqVersion} / .NET Bindings version = {OpenDAQDllInfo.Version}");
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
        using var info = daqInstance.Info;

        var name = info.Name;
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
            using var info = daqInstance.Info;

            var name = info.Name;
            Console.WriteLine($"daqInstance name = '{name}'");
        } //losing scope here, automatically calling Dispose() and thus freeing managed resources (release reference)

        Assert.That(daqInstance.IsDisposed, Is.True);
    }

    [Test]
    public void Test_0103_InstanceAutoDispose2()
    {
        //instruct TearDown function not to collect and finalize managed objects explicitly
        base.DontCollectAndFinalize();

        using var daqInstance = OpenDAQFactory.Instance(".");

        Assert.That(daqInstance.IsDisposed, Is.False);

        //can do something with 'daqInstance' here
        using var info = daqInstance.Info;

        var name = info.Name;
        Console.WriteLine($"daqInstance name = '{name}'");

        //losing scope at the end of this method, automatically calling Dispose() and thus freeing managed resources (release reference)
    }


    [Test]
    public void Test_0201_AvailableDevicesProperty()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        Console.WriteLine("> daqInstance.AvailableDevices");
        var availableDevicesInfos = daqInstance.AvailableDevices;

        Console.WriteLine($"  {availableDevicesInfos.Count} devices available");

        //list all devices
        foreach (var deviceInfo in availableDevicesInfos)
        {
            var deviceName = deviceInfo.Name;
            var connectionString = deviceInfo.ConnectionString;
            string model = deviceInfo.GetPropertyValue("model");
            string deviceClass = deviceInfo.GetPropertyValue("deviceClass");
            string softwareRevision = deviceInfo.HasProperty("softwareRevision") ? deviceInfo.GetPropertyValue("softwareRevision") : "n/a";

            Console.WriteLine($"  - Name = '{deviceName}', Connection String = '{connectionString}'");
            Console.WriteLine($"    Model = '{model}', DeviceClass = '{deviceClass}', Software Revision = '{softwareRevision}'");

            //ShowAllProperties(deviceInfo, nameof(deviceInfo));
        }
    }

    [Test]
    public void Test_0202_GetAndConnectDaqRefDevice()
    {
        // Create an Instance, loading modules from default location
        using var instance = OpenDAQFactory.Instance();

        Console.WriteLine("instance.AvailableDevices");
        var deviceInfos = instance.AvailableDevices;

        foreach (var deviceInfo in deviceInfos)
        {
            var deviceConnectionString = deviceInfo.ConnectionString;

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
            var info = device.Info;
            Console.WriteLine($"  Name: '{info.Name}', Connection string: '{info.ConnectionString}'");

            device.PrintReferenceCount();
            //device.Dispose(); //because right now there is an issue with GC collecting all devices when collecting 'Instance'
        }
    }


    [Test]
    public void Test_0301_GetAvailableChannelsTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        using var device = ConnectFirstDaqRefDevice(daqInstance);
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
                var channelName   = channel.Name;
                var localId       = channel.LocalId;
                var globalId      = channel.GlobalId;

                Console.WriteLine($"  - {i,2}: {channelName} ({localId}) ({globalId})");
            }
        }
    }

    [Test]
    public void Test_0302_GetAvailableSignalsTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        using var device = ConnectFirstDaqRefDevice(daqInstance);
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
                var signalName   = signal.Name;
                var localId      = signal.LocalId;
                var globalId     = signal.GlobalId;

                Console.WriteLine($"  - {i,2}: {signalName} ({localId}) ({globalId})");
            }
        }
    }

    [Test]
    public void Test_0303_GetAvailableFunctionBlocks()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        using var device = ConnectFirstDaqRefDevice(daqInstance);
        Assert.That(device, Is.Not.Null);

        Console.WriteLine("daqInstance.GetAvailableFunctionBlocks()");
        var availableFunctionBlockInfos = daqInstance.AvailableFunctionBlockTypes;
        Assert.Multiple(() =>
        {
            Assert.That(availableFunctionBlockInfos, Is.Not.Null);
            Assert.That(availableFunctionBlockInfos.Count, Is.GreaterThanOrEqualTo(1));
        });

        Console.WriteLine($"  {availableFunctionBlockInfos.Count} function blocks available");

        foreach (var key in availableFunctionBlockInfos.Keys)
        {
            using var functionBlockInfo = availableFunctionBlockInfos[key];
            var functionBlockId         = functionBlockInfo.Id;

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

        using var device = ConnectFirstDaqRefDevice(daqInstance);
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
                var sigName = sig.Name;

                Console.WriteLine($"  - {i,2}: {sigName}");

                ++signalNo;
                if ((sigName.Equals("AnalogValue") || sigName.Equals("AI0")) && (analogSignalNo == 0))
                {
                    analogSignalNo = signalNo;
                }
            }
        }
        Assert.That(analogSignalNo, Is.GreaterThanOrEqualTo(1));

        //take the first available signal
        using var signal = signals[analogSignalNo - 1];
        var signalName = signal.Name;
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
            Console.WriteLine($"  ValueReadType = {reader.ValueReadType}, DomainReadType = {reader.DomainReadType}");

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

    [TestCase(eDesiredReader.Stream, double.MinValue)]
    [TestCase(eDesiredReader.Tail, double.MinValue)]
    [TestCase(eDesiredReader.Block, double.MinValue)]
    //[TestCase(eDesiredReader.Multi, double.MinValue)]
    public void Test_0411_TimeReaderTest<TValueType>(eDesiredReader desiredReader, TValueType _)
        where TValueType : struct
    {
        //Hint: below it makes no sense checking objects for null since the functions throw 'OpenDaqException' in case of an error ( Result.Failed(errorCode) )
        //Hint: using statements are used below that the objects get dereferenced properly when losing scope (calling Dispose() for each "using" object at the end of this method)

        using var daqInstance = OpenDAQFactory.Instance(".");

        using var addedDevice = ConnectFirstDaqRefDevice(daqInstance);
        Assert.That(addedDevice, Is.Not.Null);

        Console.WriteLine("addedDevice.GetSignalsRecursive()");
        using var signal = addedDevice.GetSignalsRecursive()[0]; //Hint: here addedDevice.GetSignals() returns an empty list

        var signalName = signal.Name;
        var tickResolution = signal.DomainSignal.Descriptor.TickResolution;
        Console.WriteLine($"  using signal 0 '{signalName}'");
        Console.WriteLine($"  domain signal tick-resolution = '{tickResolution.Numerator}/{tickResolution.Denominator:N0}'");

        nuint count = 100;
        nuint blockSize = 1;     //10 for BlockReader (see switch/case below)
        nuint historySize = count; //only for TailReader; must not be lower than count, otherwise there will always status=fail and count=0

        TValueType[] samples = new TValueType[count];
        DateTime[] timeStamps = new DateTime[count];

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
            while ((sampleReader.Empty) && (--loopCount > 0));

            nuint samplesOrBlocksCountAvailable = sampleReader.AvailableCount;

            Console.WriteLine($"  Block {readBlockNo + 1,2}: waited {1000 - (loopCount * sleepTime)}ms -> {samplesOrBlocksCountAvailable} of {count} available");

            Assert.That(!sampleReader.Empty, "*** No data available."); //somehow using Is.GreaterThan((nuint)0) is giving a runtime error here

            using var status = timeReader.ReadWithDomain(samples, timeStamps, ref count, 1000);

            nuint samplesCount = count * blockSize; //recalculate for BlockReader (otherwise x1)
            readSamplesCount += samplesCount;

            if (status?.ReadStatus == ReadStatus.Ok)
            {
                string valueString = (samplesCount == 0)
                                     ? string.Empty
                                     : $"(0: {samples[0]:+0.000;-0.000} ... {samplesCount - 1}: {samples[samplesCount - 1]:+0.000;-0.000;�0.000} @ {timeStamps[0]:yyyy-MM-dd HH:mm:ss.fff} ... {timeStamps[samplesCount - 1]:HH:mm:ss.fff})";

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
}
