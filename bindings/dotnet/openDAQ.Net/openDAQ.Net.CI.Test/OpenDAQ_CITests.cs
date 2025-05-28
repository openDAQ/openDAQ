//#define HBK_TEST

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Numerics;

using Daq.Core;
using Daq.Core.Objects;
using Daq.Core.OpenDAQ;
using Daq.Core.Types;

using Newtonsoft.Json.Linq;


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

    #region Helper functions

    private static void FillArray<TValueType>(TValueType[] array, TValueType value)
        where TValueType : struct
    {
        if (array == null)
            return;

        Array.Fill(array, value);
    }

    private static void FillArray<TValueType>(TValueType[][] arrays, TValueType value)
        where TValueType : struct
    {
        foreach (var array in arrays)
        {
            for (int index = 0; index < array.Length; index++)
                array[index] = value;
        }
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


    //signature of ProcCallDelegate
    ErrorCode MyNullCallbackProcedure(BaseObject? parameters)
    {
        Console.WriteLine($"in {nameof(MyNullCallbackProcedure)}()");

        //expecting no parameters
        if (parameters != null)
        {
            Console.WriteLine($"-> 'parameters' is not 'null'");

            //tell API that it was not OK
            return ErrorCode.OPENDAQ_ERR_INVALIDPARAMETER;
        }

        Console.WriteLine($"-> got 'null'");

        //tell API that everything was OK
        return ErrorCode.OPENDAQ_SUCCESS;
    }

    //signature of ProcCallDelegate
    ErrorCode MyBoolCallbackProcedure(BaseObject? parameters)
    {
        Console.WriteLine($"in {nameof(MyBoolCallbackProcedure)}()");

        if (parameters == null)
        {
            Console.WriteLine($"-> 'parameters' is 'null'");

            //tell API that it was not OK
            return ErrorCode.OPENDAQ_ERR_ARGUMENT_NULL;
        }

        //expecting one boolean parameter
        var boolObj = parameters.Cast<BoolObject>();
        if (boolObj == null)
        {
            Console.WriteLine($"-> 'parameters' is not a 'BoolObject'");

            //tell API that it was not OK
            return ErrorCode.OPENDAQ_ERR_INVALIDPARAMETER;
        }

        Console.WriteLine($"-> got {boolObj.Value}");

        //tell API that everything was OK
        return ErrorCode.OPENDAQ_SUCCESS;
    }

    //signature of ProcCallDelegate
    ErrorCode MyStringCallbackProcedure(BaseObject? parameters)
    {
        Console.WriteLine($"in {nameof(MyStringCallbackProcedure)}()");

        if (parameters == null)
        {
            Console.WriteLine($"-> 'parameters' is 'null'");

            //tell API that it was not OK
            return ErrorCode.OPENDAQ_ERR_ARGUMENT_NULL;
        }

        //expecting one string parameter
        var stringObj = parameters.Cast<StringObject>();
        if (stringObj == null)
        {
            Console.WriteLine($"-> 'parameters' is not a 'StringObject'");

            //tell API that it was not OK
            return ErrorCode.OPENDAQ_ERR_INVALIDPARAMETER;
        }

        Console.WriteLine($"-> got \"{stringObj.CharPtr}\"");

        //tell API that everything was OK
        return ErrorCode.OPENDAQ_SUCCESS;
    }

    //signature of ProcCallDelegate
    ErrorCode MyStringListCallbackProcedure(BaseObject? parameters)
    {
        Console.WriteLine($"in {nameof(MyStringListCallbackProcedure)}()");

        if (parameters == null)
        {
            Console.WriteLine($"-> 'parameters' is 'null'");

            //tell API that it was not OK
            return ErrorCode.OPENDAQ_ERR_ARGUMENT_NULL;
        }

        //expecting one string parameter
        var stringListObj = parameters.CastList<StringObject>();
        if (stringListObj == null)
        {
            Console.WriteLine($"-> 'parameters' is not a 'ListObject<StringObject>'");

            //tell API that it was not OK
            return ErrorCode.OPENDAQ_ERR_INVALIDPARAMETER;
        }

        Console.WriteLine($"-> got {stringListObj.Count} strings:");
        foreach (var stringObj in stringListObj)
            Console.WriteLine($"   - \"{stringObj}\"");

        //tell API that everything was OK
        return ErrorCode.OPENDAQ_SUCCESS;
    }

    //signature of FuncCallDelegate
    private ErrorCode MyBoolCallbackFunction(BaseObject? parameters, out BaseObject? result)
    {
        //initialize output
        result = null;

        Console.WriteLine($"in {nameof(MyBoolCallbackFunction)}()");

        if (parameters == null)
        {
            Console.WriteLine($"-> 'parameters' is 'null'");

            //tell API that it was not OK
            return ErrorCode.OPENDAQ_ERR_ARGUMENT_NULL;
        }

        //expecting one boolean parameter
        var boolObj = parameters.Cast<BoolObject>();
        if (boolObj == null)
        {
            Console.WriteLine($"-> 'parameters' is not a 'BoolObject'");

            //tell API that it was not OK
            return ErrorCode.OPENDAQ_ERR_INVALIDPARAMETER;
        }

        Console.WriteLine($"-> got {boolObj.Value}");

        //returning the given boolean parameter as result
        result = boolObj.Value;

        //tell API that everything was OK
        return ErrorCode.OPENDAQ_SUCCESS;
    }

    //signature of FuncCallDelegate
    ErrorCode MyIntegerToStringListCallbackFunction(BaseObject? parameters, out BaseObject? result)
    {
        //initialize output
        result = null;

        Console.WriteLine($"in {nameof(MyIntegerToStringListCallbackFunction)}()");

        if (parameters == null)
        {
            Console.WriteLine($"-> 'parameters' is 'null'");

            //tell API that it was not OK
            return ErrorCode.OPENDAQ_ERR_ARGUMENT_NULL;
        }

        //expecting one string parameter
        var integerListObj = parameters?.CastList<IntegerObject>();
        if (integerListObj == null)
        {
            Console.WriteLine($"-> 'parameters' is not a 'ListObject<IntegerObject>'");

            //tell API that it was not OK
            return ErrorCode.OPENDAQ_ERR_INVALIDPARAMETER;
        }

        Console.WriteLine($"-> got {integerListObj.Count} integers:");

        //create the result list
        var stringList = CoreTypesFactory.CreateList<StringObject>();
        foreach (var integerObj in integerListObj)
        {
            Console.WriteLine($"   - {integerObj}");
            stringList.Add($"string {integerObj}");
        }

        //returning the created string list
        result = (BaseObject)stringList;

        //tell API that everything was OK
        return ErrorCode.OPENDAQ_SUCCESS;
    }

    //signature of FuncCallDelegate
    ErrorCode MyNullToStringCallbackFunction(BaseObject? parameters, out BaseObject? result)
    {
        //in this callback we always return ErrorCode.OPENDAQ_SUCCESS

        Console.WriteLine($"in {nameof(MyIntegerToStringListCallbackFunction)}()");

        if (parameters != null)
        {
            Console.WriteLine($"-> 'parameters' is not 'null'");
            result = "'parameters' was not 'null'";
        }
        else
        {
            Console.WriteLine($"-> 'parameters' is 'null'");
            result = "'parameters' was 'null'";
        }

        //tell API that everything was OK
        return ErrorCode.OPENDAQ_SUCCESS;
    }

    #endregion Helper functions

    [Test]
    public void Test_0000_GetVersion()
    {
        Version coreTypesVersion   = new();
        Version coreObjectsVersion = new();
        Version openDaqVersion     = new();

        var sdkAssembly         = typeof(CoreTypesFactory).Assembly;
        var assemblyVersion     = sdkAssembly.GetName().Version;
        var assemblyInfoVersion = sdkAssembly.GetCustomAttributes(typeof(System.Reflection.AssemblyInformationalVersionAttribute), true)
                                  .Cast<System.Reflection.AssemblyInformationalVersionAttribute>()
                                  .Single()?.InformationalVersion;

        Assert.Multiple(() =>
        {
            Assert.DoesNotThrow(() => coreTypesVersion   = CoreTypesFactory.SdkVersion,   "CoreTypesFactory.GetSdkVersion() failed");
            Assert.DoesNotThrow(() => coreObjectsVersion = CoreObjectsFactory.SdkVersion, "CoreObjectsFactory.GetSdkVersion() failed");
            Assert.DoesNotThrow(() => openDaqVersion     = OpenDAQFactory.SdkVersion,     "OpenDAQFactory.GetSdkVersion() failed");
        });

        Console.WriteLine($"CoreTypes SDK version   = {coreTypesVersion} / .NET Bindings version = {CoreTypesDllInfo.Version}");
        Console.WriteLine($"CoreObjects SDK version = {coreObjectsVersion} / .NET Bindings version = {CoreObjectsDllInfo.Version}");
        Console.WriteLine($"openDAQ SDK version     = {openDaqVersion} / .NET Bindings version = {OpenDAQDllInfo.Version}");
        Console.WriteLine($"Assembly version        = {assemblyVersion}");
        Console.WriteLine($"Assembly info version   = {assemblyInfoVersion}");
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
    public void Test_0002_ListObjectWithParamsNoExceptionTest()
    {
        Console.WriteLine("> creating string-list with exception");

        Assert.DoesNotThrow(() =>
        {
            //<TValue> cannot be inferred from the items
            var list = CoreTypesFactory.CreateList<StringObject>("item 1", "item 2", "item 3", "item 4");

            Assert.That(list, Is.Not.Null);
            Assert.That(list.Count, Is.EqualTo(4));
            Assert.That(list[0].CanCastTo<StringObject>(), Is.True);
            Assert.Multiple(() =>
            {
                Assert.That((string)list[0], Is.EqualTo("item 1"));
                Assert.That((string)list[1], Is.EqualTo("item 2"));
                Assert.That((string)list[2], Is.EqualTo("item 3"));
                Assert.That((string)list[3], Is.EqualTo("item 4"));
            });
        });
    }

    [Test]
    public void Test_0003_ListObjectWithParamsNoErrorCodeTest()
    {
        Console.WriteLine("> creating string-list with error-code");

        //<TValue> can be inferred from the out-parameter
        ErrorCode errorcode = CoreTypesFactory.CreateList(out IListObject<StringObject> list,
                                                          "item 1", "item 2", "item 3", "item 4");

        Assert.That(errorcode, Is.EqualTo(ErrorCode.OPENDAQ_SUCCESS));
        Assert.That(list, Is.Not.Null);
        Assert.That(list.Count, Is.EqualTo(4));
        Assert.That(list[0].CanCastTo<StringObject>(), Is.True);
        Assert.Multiple(() =>
        {
            Assert.That((string)list[0], Is.EqualTo("item 1"));
            Assert.That((string)list[1], Is.EqualTo("item 2"));
            Assert.That((string)list[2], Is.EqualTo("item 3"));
            Assert.That((string)list[3], Is.EqualTo("item 4"));
        });
    }

    [Test]
    public void Test_0004_ListObjectWithParamsArrayTest()
    {
        Console.WriteLine("> creating string-list with array");

        //create StringObject array (no automatic cast)
        StringObject[] items = new[] { (StringObject)"item 1", (StringObject)"item 2", (StringObject)"item 3", (StringObject)"item 4" };

        Assert.DoesNotThrow(() =>
        {
            //<TValue> can be inferred from the array
            var list = CoreTypesFactory.CreateList(items);

            Assert.That(list, Is.Not.Null);
            Assert.That(list.Count, Is.EqualTo(4));
            Assert.That(list[0].CanCastTo<StringObject>(), Is.True);
            Assert.Multiple(() =>
            {
                Assert.That((string)list[0], Is.EqualTo("item 1"));
                Assert.That((string)list[1], Is.EqualTo("item 2"));
                Assert.That((string)list[2], Is.EqualTo("item 3"));
                Assert.That((string)list[3], Is.EqualTo("item 4"));
            });
        });
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
        int loopCount    = 10;
        nuint maxCount   = 150;
        TValue[] samples = new TValue[maxCount];
        FillArray(samples, (TValue)Convert.ChangeType(99999, typeof(TValue)));
        int sleepTime          = 25;
        nuint readSamplesCount = 0;
        //here using-sub-scope to free resources asap
        using (var reader = OpenDAQFactory.CreateStreamReader<TValue>(signal))
        {
            Console.WriteLine($"  ValueReadType = {reader.ValueReadType}, DomainReadType = {reader.DomainReadType}");
            Console.WriteLine($"  Reading {loopCount} times {maxCount} values (waiting {sleepTime}ms before reading)");

            Stopwatch sw = new Stopwatch();

            for (int loopNo = 0; loopNo < loopCount; ++loopNo)
            {
                //ToDo: do we get buffer overrun when we just leave reader open for too long without reading?

                sw.Restart();

                nuint count = (nuint)samples.Length; //reset to array size

                //just wait a bit
                Thread.Sleep(sleepTime); //StreamReader does not need to wait for 'AvailableCount == count' as it has 'timeoutMs'

                nuint availableCount = reader.AvailableCount;
                Debug.Print($"+++> {loopNo + 1,2}: AvailableCount={availableCount,-3}");

                //read up to 'count' samples, storing the amount read into array 'samples'
                reader.Read(samples, ref count, timeoutMs: 5000);
                readSamplesCount += count;

                sw.Stop();

                string valueString = (count == 0) ? string.Empty : $"(0: {samples[0]:+0.000;-0.000; 0.000} ... {count - 1,3}: {samples[count - 1]:+0.000;-0.000; 0.000})";
                Console.WriteLine($"  Loop {loopNo + 1,2} {sw.Elapsed.TotalMilliseconds,7:0.000}ms before AvailableCount={availableCount,-3} but read {count,3} values {valueString}");
            }
        }

        Assert.That(readSamplesCount, Is.GreaterThan((nuint)0), "*** No samples received.");
    }

    [TestCase(int.MinValue)]
    [TestCase(ulong.MinValue)]
    [TestCase(double.MinValue)]
    public void Test_0402_TailReaderTest<TValue>(TValue _)
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

        Console.WriteLine("OpenDAQFactory.CreateTailReader()");
        int loopCount    = 10;
        nuint maxCount   = 150;
        TValue[] samples = new TValue[maxCount];
        FillArray(samples, (TValue)Convert.ChangeType(99999, typeof(TValue)));
        int sleepTime          = 25;
        nuint readSamplesCount = 0;
        //here using-sub-scope to free resources asap
        using (var reader = OpenDAQFactory.CreateTailReader<TValue>(signal, maxCount))
        {
            Console.WriteLine($"  ValueReadType = {reader.ValueReadType}, DomainReadType = {reader.DomainReadType}");
            Console.WriteLine($"  Reading {loopCount} times {maxCount} values (waiting {sleepTime}ms before reading)");

            Stopwatch sw = new Stopwatch();

            for (int loopNo = 0; loopNo < loopCount; ++loopNo)
            {
                //ToDo: do we get buffer overrun when we just leave reader open for too long without reading?

                sw.Restart();

                nuint count = (nuint)samples.Length; //reset to array size

                //just wait a bit
                Thread.Sleep(sleepTime); //TailReader does not need to wait for 'AvailableCount == count'

                nuint availableCount = reader.AvailableCount;
                Debug.Print($"+++>  {loopNo + 1,2} : AvailableCount={availableCount,-3}");

                //read up to 'count' samples, storing the amount read into array 'samples'
                reader.Read(samples, ref count);
                readSamplesCount += count;

                sw.Stop();

                string valueString = (count == 0) ? string.Empty : $"(0: {samples[0]:+0.000;-0.000; 0.000} ... {count - 1,3}: {samples[count - 1]:+0.000;-0.000; 0.000})";
                Console.WriteLine($"  Loop {loopNo + 1,2} {sw.Elapsed.TotalMilliseconds,7:0.000}ms before AvailableCount={availableCount,-3} but read {count,3} values {valueString}");
            }
        }

        Assert.That(readSamplesCount, Is.GreaterThan((nuint)0), "*** No samples received.");
    }

    [TestCase(int.MinValue)]
    [TestCase(ulong.MinValue)]
    [TestCase(double.MinValue)]
    public void Test_0403_BlockReaderTest<TValue>(TValue _)
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

        Console.WriteLine("OpenDAQFactory.CreateBlockReader()");
        int loopCount    = 10;
        nuint blockSize  = 30;
        nuint maxCount   = 10;
        TValue[] samples = new TValue[blockSize * maxCount];
        FillArray(samples, (TValue)Convert.ChangeType(99999, typeof(TValue)));
        int sleepTime        = 25;
        nuint readBlockCount = 0;
        //here using-sub-scope to free resources asap
        using (var reader = OpenDAQFactory.CreateBlockReader<TValue>(signal, blockSize))
        {
            Console.WriteLine($"  ValueReadType = {reader.ValueReadType}, DomainReadType = {reader.DomainReadType}");
            Console.WriteLine($"  Reading {loopCount} times {maxCount} blocks � {blockSize} values (waiting {sleepTime}ms before reading)");

            Stopwatch sw = new Stopwatch();

            for (int loopNo = 0; loopNo < loopCount; ++loopNo)
            {
                //ToDo: do we get buffer overrun when we just leave reader open for too long without reading?

                sw.Restart();

                nuint count = (nuint)samples.Length / blockSize; //reset to array size

                //just wait a bit
                Thread.Sleep(sleepTime); //BlockReader does not need to wait for 'AvailableCount == count' as it has 'timeoutMs'

                nuint availableCount = reader.AvailableCount;

                //read up to 'count' blocks, storing the amount read into array 'samples'
                reader.Read(samples, ref count, 5000);
                readBlockCount += count;
                Debug.Print($"+++>     read {count} blocks");

                sw.Stop();

                nuint valueCount = count * blockSize;
                string valueString = (count == 0) ? string.Empty : $"(0: {samples[0]:+0.000;-0.000; 0.000} ... {valueCount - 1}: {samples[valueCount - 1]:+0.000;-0.000; 0.000})";
                Console.WriteLine($"  Loop {loopNo + 1,2} {sw.Elapsed.TotalMilliseconds,7:0.000}ms before AvailableCount={availableCount,-3} but read {count,2} blocks ({valueCount,3} values) {valueString}");
            }
        }

        Assert.That(readBlockCount, Is.GreaterThan((nuint)0), "*** No samples received.");
    }

    [TestCase(int.MinValue)]
    [TestCase(ulong.MinValue)]
    [TestCase(double.MinValue)]
    public void Test_0404_MultiReaderTest<TValue>(TValue _)
        where TValue : struct
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        using var device = ConnectFirstDaqRefDevice(daqInstance);
        Assert.That(device, Is.Not.Null);

        Console.WriteLine("addedDevice.GetSignalsRecursive()");
        var signals = device.GetSignalsRecursive();

        int signalCount = signals.Count;
        Console.WriteLine($"  found {signalCount} signals");
        Assert.That(signalCount, Is.GreaterThanOrEqualTo(2));

        //take the first two signals
        using var signalList = CoreTypesFactory.CreateList<Signal>();
        signalList.Add(signals[0]);
        signalList.Add(signals[1]);

        Console.WriteLine($"  using signal 0 '{signals[0].Name}'");
        Console.WriteLine($"  using signal 1 '{signals[1].Name}'");

        Console.WriteLine("OpenDAQFactory.CreateMultiReader()");
        int loopCount     = 10;
        nuint maxCount    = 200;
        TValue[][] samples = new TValue[2][] { new TValue[maxCount], new TValue[maxCount] };
        FillArray(samples, (TValue)Convert.ChangeType(99999, typeof(TValue)));
        int sleepTime          = 25;
        nuint readSamplesCount = 0;
        //here using-sub-scope to free resources asap
        using (var reader = OpenDAQFactory.CreateMultiReader<TValue>(signalList))
        {
            Console.WriteLine($"  ValueReadType = {reader.ValueReadType}, DomainReadType = {reader.DomainReadType}");
            Console.WriteLine($"  Reading {loopCount} times {maxCount} values (waiting {sleepTime}ms before reading)");

            Stopwatch sw = new Stopwatch();

            for (int loopNo = 0; loopNo < loopCount; ++loopNo)
            {
                //ToDo: do we get buffer overrun when we just leave reader open for too long without reading?

                sw.Restart();

                nuint count = maxCount; //reset to array size

                //just wait a bit
                Thread.Sleep(sleepTime); //MultiReader does not need to wait for 'AvailableCount == count' as it has 'timeoutMs'

                nuint availableCount = reader.AvailableCount;
                Debug.Print($"+++> {loopNo + 1,2}: AvailableCount={availableCount,-3}");

                //read up to 'count' samples, storing the amount read into array 'samples'
                reader.Read(samples, ref count/*, timeoutMs: 5000*/);
                readSamplesCount += count;

                sw.Stop();

                string valueString  = (count == 0) ? string.Empty : $"(0: {samples[0][0]:+0.000;-0.000; 0.000} ... {count - 1,3}: {samples[0][count - 1]:+0.000;-0.000; 0.000})";
                string valueString2 = (count == 0) ? string.Empty : $"(0: {samples[1][0]:+0.000;-0.000; 0.000} ... {count - 1,3}: {samples[1][count - 1]:+0.000;-0.000; 0.000})";
                Console.WriteLine($"  Loop {loopNo + 1,2} {sw.Elapsed.TotalMilliseconds,7:0.000}ms before AvailableCount={availableCount,-3} but read {count,3} values {valueString}");
                Console.WriteLine($"                                                       {valueString2}");
            }
        }

        Assert.That(readSamplesCount, Is.GreaterThan((nuint)0), "*** No samples received.");
    }


    [Test]
    public void Test_0411_StreamReaderWithDomainTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        using var device = ConnectFirstDaqRefDevice(daqInstance);
        Assert.That(device, Is.Not.Null);

        Console.WriteLine("addedDevice.GetSignalsRecursive()");
        var signals = device.GetSignalsRecursive();

        ulong signalCount = (ulong)signals.Count;
        Console.WriteLine($"  found {signalCount} signals");
        Assert.That(signalCount, Is.GreaterThanOrEqualTo(1));

        //take the first available signal
        using var signal = signals[0];
        var signalName   = signal.Name;
        Console.WriteLine($"  using signal 0 '{signalName}'");

        using Ratio tickResolution = signals[0].DomainSignal.Descriptor.TickResolution;
        double factor = (double)tickResolution.Numerator / tickResolution.Denominator;
        Console.WriteLine($"  tick resolution: {tickResolution.Numerator} / {tickResolution.Denominator} = {factor}");

        Console.WriteLine("OpenDAQFactory.CreateStreamReader()");
        int loopCount     = 10;
        nuint maxCount    = 150;
        double[] samples  = new double[maxCount];
        long[] timeStamps = new long[maxCount];
        FillArray(samples, 99999D);
        FillArray(timeStamps, 99999L);
        int sleepTime          = 25;
        nuint readSamplesCount = 0;
        //here using-sub-scope to free resources asap
        using (var reader = OpenDAQFactory.CreateStreamReader<double>(signal))
        {
            Console.WriteLine($"  ValueReadType = {reader.ValueReadType}, DomainReadType = {reader.DomainReadType}");
            Console.WriteLine($"  Reading {loopCount} times {maxCount} values (waiting {sleepTime}ms before reading)");

            Stopwatch sw = new Stopwatch();

            for (int loopNo = 0; loopNo < loopCount; ++loopNo)
            {
                //ToDo: do we get buffer overrun when we just leave reader open for too long without reading?

                sw.Restart();

                nuint count = (nuint)samples.Length; //reset to array size

                //just wait a bit
                Thread.Sleep(sleepTime); //StreamReader does not need to wait for 'AvailableCount == count' as it has 'timeoutMs'

                nuint availableCount = reader.AvailableCount;
                Debug.Print($"+++> {loopNo + 1,2}: AvailableCount={availableCount,-3}");

                //read up to 'count' samples, storing the amount read into arrays 'samples' and 'timeStamps'
                reader.ReadWithDomain(samples, timeStamps, ref count, timeoutMs: 5000);
                readSamplesCount += count;

                sw.Stop();

                string valueString = (count == 0) ? string.Empty : $"(0: {samples[0]:+0.000;-0.000; 0.000} ... {count - 1,3}: {samples[count - 1]:+0.000;-0.000; 0.000} @ {factor * timeStamps[0]:0.0000000} ... {factor * timeStamps[count - 1]:0.0000000})";
                Console.WriteLine($"  Loop {loopNo + 1,2} {sw.Elapsed.TotalMilliseconds,7:0.000}ms before AvailableCount={availableCount,-3} but read {count,3} values {valueString}");
            }
        }

        Assert.That(readSamplesCount, Is.GreaterThan((nuint)0), "*** No samples received.");
    }

    [Test]
    public void Test_0412_TailReaderWithDomainTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        using var device = ConnectFirstDaqRefDevice(daqInstance);
        Assert.That(device, Is.Not.Null);

        Console.WriteLine("addedDevice.GetSignalsRecursive()");
        var signals = device.GetSignalsRecursive();

        ulong signalCount = (ulong)signals.Count;
        Console.WriteLine($"  found {signalCount} signals");
        Assert.That(signalCount, Is.GreaterThanOrEqualTo(1));

        //take the first available signal
        using var signal = signals[0];
        var signalName   = signal.Name;
        Console.WriteLine($"  using signal 0 '{signalName}'");

        using Ratio tickResolution = signals[0].DomainSignal.Descriptor.TickResolution;
        double factor = (double)tickResolution.Numerator / tickResolution.Denominator;
        Console.WriteLine($"  tick resolution: {tickResolution.Numerator} / {tickResolution.Denominator} = {factor}");

        Console.WriteLine("OpenDAQFactory.CreateTailReader()");
        int loopCount     = 10;
        nuint maxCount    = 150;
        double[] samples  = new double[maxCount];
        long[] timeStamps = new long[maxCount];
        FillArray(samples, 99999D);
        FillArray(timeStamps, 99999L);
        int sleepTime          = 25;
        nuint readSamplesCount = 0;
        //here using-sub-scope to free resources asap
        using (var reader = OpenDAQFactory.CreateTailReader<double>(signal, maxCount))
        {
            Console.WriteLine($"  ValueReadType = {reader.ValueReadType}, DomainReadType = {reader.DomainReadType}");
            Console.WriteLine($"  Reading {loopCount} times {maxCount} values (waiting {sleepTime}ms before reading)");

            Stopwatch sw = new Stopwatch();

            for (int loopNo = 0; loopNo < loopCount; ++loopNo)
            {
                //ToDo: do we get buffer overrun when we just leave reader open for too long without reading?

                sw.Restart();

                nuint count = (nuint)samples.Length; //reset to array size

                //just wait a bit
                Thread.Sleep(sleepTime); //TailReader does not need to wait for 'AvailableCount == count'

                nuint availableCount = reader.AvailableCount;
                Debug.Print($"+++>  {loopNo + 1,2} : AvailableCount={availableCount,-3}");

                //read up to 'count' samples, storing the amount read into arrays 'samples' and 'timeStamps'
                reader.ReadWithDomain(samples, timeStamps, ref count);
                readSamplesCount += count;
                sw.Stop();
                string valueString = (count == 0) ? string.Empty : $"(0: {samples[0]:+0.000;-0.000; 0.000} ... {count - 1,3}: {samples[count - 1]:+0.000;-0.000; 0.000} @ {factor * timeStamps[0]:0.0000000} ... {factor * timeStamps[count - 1]:0.0000000})";
                Console.WriteLine($"  Loop {loopNo + 1,2} {sw.Elapsed.TotalMilliseconds,7:0.000}ms before AvailableCount={availableCount,-3} but read {count,3} values {valueString}");
            }
        }

        Assert.That(readSamplesCount, Is.GreaterThan((nuint)0), "*** No samples received.");
    }

    [Test]
    public void Test_0413_BlockReaderWithDomainTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        using var device = ConnectFirstDaqRefDevice(daqInstance);
        Assert.That(device, Is.Not.Null);

        Console.WriteLine("addedDevice.GetSignalsRecursive()");
        var signals = device.GetSignalsRecursive();

        ulong signalCount = (ulong)signals.Count;
        Console.WriteLine($"  found {signalCount} signals");
        Assert.That(signalCount, Is.GreaterThanOrEqualTo(1));

        //take the first available signal
        using var signal = signals[0];
        var signalName   = signal.Name;
        Console.WriteLine($"  using signal 0 '{signalName}'");

        using Ratio tickResolution = signals[0].DomainSignal.Descriptor.TickResolution;
        double factor = (double)tickResolution.Numerator / tickResolution.Denominator;
        Console.WriteLine($"  tick resolution: {tickResolution.Numerator} / {tickResolution.Denominator} = {factor}");

        Console.WriteLine("OpenDAQFactory.CreateBlockReader()");
        int loopCount     = 10;
        nuint blockSize   = 30;
        nuint maxCount    = 10;
        double[] samples  = new double[blockSize * maxCount];
        long[] timeStamps = new long[blockSize * maxCount];
        FillArray(samples, 99999D);
        FillArray(timeStamps, 99999L);
        int sleepTime        = 25;
        nuint readBlockCount = 0;
        //here using-sub-scope to free resources asap
        using (var reader = OpenDAQFactory.CreateBlockReader<double>(signal, blockSize))
        {
            Console.WriteLine($"  ValueReadType = {reader.ValueReadType}, DomainReadType = {reader.DomainReadType}");
            Console.WriteLine($"  Reading {loopCount} times {maxCount} blocks � {blockSize} values (waiting {sleepTime}ms before reading)");

            Stopwatch sw = new Stopwatch();

            for (int loopNo = 0; loopNo < loopCount; ++loopNo)
            {
                //ToDo: do we get buffer overrun when we just leave reader open for too long without reading?

                sw.Restart();

                nuint count = (nuint)samples.Length / blockSize; //reset to array size

                //just wait a bit
                Thread.Sleep(sleepTime); //BlockReader does not need to wait for 'AvailableCount == count' as it has 'timeoutMs'

                nuint availableCount = reader.AvailableCount;
                Debug.Print($"+++> {loopNo + 1,2}: AvailableCount={availableCount,-3}");

                //read up to 'count' blocks, storing the amount read into arrays 'samples' and 'timeStamps'
                reader.ReadWithDomain(samples, timeStamps, ref count, 5000);
                readBlockCount += count;
                Debug.Print($"+++>     read {count} blocks");

                sw.Stop();

                nuint valueCount = count * blockSize;
                string valueString = (count == 0) ? string.Empty : $"(0: {samples[0]:+0.000;-0.000; 0.000} ... {valueCount - 1}: {samples[valueCount - 1]:+0.000;-0.000; 0.000} @ {factor * timeStamps[0]:0.0000000} ... {factor * timeStamps[count - 1]:0.0000000})";
                Console.WriteLine($"  Loop {loopNo + 1,2} {sw.Elapsed.TotalMilliseconds,7:0.000}ms before AvailableCount={availableCount,-3} but read {count,2} blocks ({valueCount,3} values) {valueString}");
            }
        }

        Assert.That(readBlockCount, Is.GreaterThan((nuint)0), "*** No samples received.");
    }

    [Test]
    public void Test_0414_MultiReaderWithDomainTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        using var device = ConnectFirstDaqRefDevice(daqInstance);
        Assert.That(device, Is.Not.Null);

        Console.WriteLine("addedDevice.GetSignalsRecursive()");
        var signals = device.GetSignalsRecursive();

        int signalCount = signals.Count;
        Console.WriteLine($"  found {signalCount} signals");
        Assert.That(signalCount, Is.GreaterThanOrEqualTo(2));

        //take the first two signals
        using var signalList = CoreTypesFactory.CreateList<Signal>();
        signalList.Add(signals[0]);
        signalList.Add(signals[1]);

        Console.WriteLine($"  using signal 0 '{signals[0].Name}'");
        Console.WriteLine($"  using signal 1 '{signals[1].Name}'");

        using Ratio tickResolution = signals[0].DomainSignal.Descriptor.TickResolution;
        double factor = (double)tickResolution.Numerator / tickResolution.Denominator;
        Console.WriteLine($"  tick resolution: {tickResolution.Numerator} / {tickResolution.Denominator} = {factor}");

        Console.WriteLine("OpenDAQFactory.CreateMultiReader()");
        int loopCount      = 10;
        nuint maxCount     = 150;
        double[][] samples  = new double[2][] { new double[maxCount], new double[maxCount] };
        long[][] timeStamps = new long[2][]   { new long[maxCount],   new long[maxCount] };
        FillArray(samples, 99999D);
        FillArray(timeStamps, 99999L);
        int sleepTime          = 25;
        nuint readSamplesCount = 0;
        //here using-sub-scope to free resources asap
        using (var reader = OpenDAQFactory.CreateMultiReader<double>(signalList))
        {
            Console.WriteLine($"  ValueReadType = {reader.ValueReadType}, DomainReadType = {reader.DomainReadType}");
            Console.WriteLine($"  Reading {loopCount} times {maxCount} values (waiting {sleepTime}ms before reading)");

            Stopwatch sw = new Stopwatch();

            for (int loopNo = 0; loopNo < loopCount; ++loopNo)
            {
                //ToDo: do we get buffer overrun when we just leave reader open for too long without reading?

                sw.Restart();

                nuint count = maxCount; //reset to array size

                //just wait a bit
                Thread.Sleep(sleepTime); //MultiReader does not need to wait for 'AvailableCount == count' as it has 'timeoutMs'

                nuint availableCount = reader.AvailableCount;
                Debug.Print($"+++> {loopNo + 1,2}: AvailableCount={availableCount,-3}");

                //read up to 'count' samples, storing the amount read into arrays 'samples' and 'timeStamps'
                //reader.Read(samples, ref count, timeoutMs: 5000);
                reader.ReadWithDomain(samples, timeStamps, ref count, timeoutMs: 5000);
                readSamplesCount += count;

                sw.Stop();

                string valueString  = (count == 0) ? string.Empty : $"(0: {samples[0][0]:+0.000;-0.000; 0.000} ... {count - 1,3}: {samples[0][count - 1]:+0.000;-0.000; 0.000} @ {factor * timeStamps[0][0]:0.0000000} ... {factor * timeStamps[0][count - 1]:0.0000000})";
                string valueString2 = (count == 0) ? string.Empty : $"(0: {samples[1][0]:+0.000;-0.000; 0.000} ... {count - 1,3}: {samples[1][count - 1]:+0.000;-0.000; 0.000} @ {factor * timeStamps[1][0]:0.0000000} ... {factor * timeStamps[1][count - 1]:0.0000000})";
                Console.WriteLine($"  Loop {loopNo + 1,2} {sw.Elapsed.TotalMilliseconds,7:0.000}ms before AvailableCount={availableCount,-3} but read {count,3} values {valueString}");
                Console.WriteLine($"                                                       {valueString2}");
            }
        }

        Assert.That(readSamplesCount, Is.GreaterThan((nuint)0), "*** No samples received.");
    }


    [TestCase(eDesiredReader.Stream, double.MinValue)]
    [TestCase(eDesiredReader.Tail, double.MinValue)]
    [TestCase(eDesiredReader.Block, double.MinValue)]
    [TestCase(eDesiredReader.Multi, double.MinValue)]
    public void Test_0451_TimeReaderTest<TValueType>(eDesiredReader desiredReader, TValueType _)
        where TValueType : struct
    {
        //Hint: below it makes no sense checking objects for null since the functions throw 'OpenDaqException' in case of an error ( Result.Failed(errorCode) )
        //Hint: using statements are used below that the objects get dereferenced properly when losing scope (calling Dispose() for each "using" object at the end of this method)

        using var daqInstance = OpenDAQFactory.Instance(".");

        using var addedDevice = ConnectFirstDaqRefDevice(daqInstance);
        Assert.That(addedDevice, Is.Not.Null);

        Console.WriteLine("addedDevice.GetSignalsRecursive()");
        using var signals      = addedDevice.GetSignalsRecursive(); //Hint: here addedDevice.GetSignals() returns an empty list
        using var signal0      = signals[0];
        using var signal1      = signals[1];
        using var signalsMulti = CoreTypesFactory.CreateList<Signal>();
        if (desiredReader == eDesiredReader.Multi)
        {
            signalsMulti.Add(signal0);
            signalsMulti.Add(signal1);
        }

        Console.WriteLine($"  using signal 0 '{signal0.Name}'");
        if (desiredReader == eDesiredReader.Multi) Console.WriteLine($"  using signal 1 '{signal1.Name}'");

        var tickResolution = signal0.DomainSignal.Descriptor.TickResolution;
        Console.WriteLine($"  domain signal tick-resolution = '{tickResolution.Numerator}/{tickResolution.Denominator:N0}'");

        nuint maxCount    = 100;
        nuint blockSize   = 1;        //10 for BlockReader (see switch/case below)
        nuint historySize = maxCount; //only for TailReader; must not be lower than count, otherwise there will always status=fail and count=0

        TValueType[]?   samples         = (desiredReader != eDesiredReader.Multi) ? new TValueType[maxCount] : null;
        DateTime[]?     timeStamps      = (desiredReader != eDesiredReader.Multi) ? new DateTime[maxCount]   : null;
        TValueType[][]? samplesMulti    = (desiredReader == eDesiredReader.Multi) ? new TValueType[2][] { new TValueType[maxCount], new TValueType[maxCount] } : null;
        DateTime[][]?   timeStampsMulti = (desiredReader == eDesiredReader.Multi) ? new DateTime[2][]   { new DateTime[maxCount],   new DateTime[maxCount] }   : null;

        TValueType defaultValue = (TValueType)Convert.ChangeType(99999, typeof(TValueType));
        if (samples != null)
        {
            FillArray(samples, defaultValue);
            FillArray(timeStamps!, DateTime.MinValue);
        }
        else if (samplesMulti != null)
        {
            FillArray(samplesMulti, defaultValue);
            FillArray(timeStampsMulti!, DateTime.MinValue);
        }

        SampleReader? sampleReader = null;

        Console.WriteLine($"OpenDAQFactory.Create{desiredReader}Reader()");
        switch (desiredReader)
        {
            case eDesiredReader.Block:
                blockSize = 10;
                Console.WriteLine($"  buffer size = {maxCount}");
                Console.WriteLine($"  block  size = {blockSize}");
                sampleReader = OpenDAQFactory.CreateBlockReader<TValueType, Int64>(signal0, blockSize); //not working when data not available (native exception)
                break;

            case eDesiredReader.Multi:
                sampleReader = OpenDAQFactory.CreateMultiReader<TValueType, Int64>(signalsMulti);
                break;

            case eDesiredReader.Stream:
                Console.WriteLine($"  buffer size = {maxCount}");
                sampleReader = OpenDAQFactory.CreateStreamReader<TValueType, Int64>(signal0);
                break;

            case eDesiredReader.Tail:
                Console.WriteLine($"  buffer  size = {maxCount}");
                Console.WriteLine($"  history size = {historySize}");
                sampleReader = OpenDAQFactory.CreateTailReader<TValueType, Int64>(signal0, historySize); //not working when historySize < count (always reading count=0)
                break;

            default:
                Assert.Fail($"Desired reader not (yet) supported: {nameof(eDesiredReader)}.{desiredReader}");
                break;
        }

        Assert.That(sampleReader, Is.Not.Null, "*** No SampleReader instance available.");

        Console.WriteLine("OpenDAQFactory.CreateTimeReader()");
        TimeReader timeReader = OpenDAQFactory.CreateTimeReader(sampleReader, signal0);
        int readFailures = 0;

        Console.WriteLine($"  ValueReadType = {sampleReader.ValueReadType}, DomainReadType = {sampleReader.DomainReadType}");

        nuint readSamplesCount = 0;

        Console.WriteLine("looping timeReader.ReadWithDomain()");
        const int sleepTime = 25;
        for (int loopNo = 0; loopNo < 10; ++loopNo)
        {
            //read up to 'count' samples or blocks, storing the amount read into arrays 'samples' and 'timeStamps'
            nuint count = maxCount / blockSize; //reset to array size or block count

            //ToDo: do we get buffer overrun when we just leave reader open for too long without reading?

            //just wait a bit
            Thread.Sleep(sleepTime);

            nuint samplesOrBlocksCountAvailable = sampleReader.AvailableCount;

            using var status = (samples != null)
                               ? timeReader.ReadWithDomain(samples, timeStamps, ref count, 1000)
                               : timeReader.ReadWithDomain(samplesMulti, timeStampsMulti, ref count, 1000);

            nuint samplesCount = count * blockSize; //recalculate for BlockReader (otherwise x1)
            readSamplesCount += samplesCount;

            if (status?.ReadStatus == ReadStatus.Ok)
            {
                string valueString  = string.Empty;
                string valueString2 = string.Empty;

                if (samplesCount > 0)
                {
                    if ((samples != null) && (timeStamps != null))
                    {
                        valueString = $"(0: {samples[0]:+0.000;-0.000; 0.000} ... {samplesCount - 1}: {samples[samplesCount - 1]:+0.000;-0.000; 0.000} @ {timeStamps[0]:yyyy-MM-dd HH:mm:ss.fffffff} ... {timeStamps[samplesCount - 1]:HH:mm:ss.fffffff})";
                    }
                    else if ((samplesMulti != null) && (timeStampsMulti != null))
                    {
                        valueString  = $"(0: {samplesMulti[0][0]:+0.000;-0.000; 0.000} ... {samplesCount - 1}: {samplesMulti[0][samplesCount - 1]:+0.000;-0.000; 0.000} @ {timeStampsMulti[0][0]:yyyy-MM-dd HH:mm:ss.fffffff} ... {timeStampsMulti[0][samplesCount - 1]:HH:mm:ss.fffffff})";
                        valueString2 = $"(0: {samplesMulti[1][0]:+0.000;-0.000; 0.000} ... {samplesCount - 1}: {samplesMulti[1][samplesCount - 1]:+0.000;-0.000; 0.000} @ {timeStampsMulti[1][0]:yyyy-MM-dd HH:mm:ss.fffffff} ... {timeStampsMulti[1][samplesCount - 1]:HH:mm:ss.fffffff})";
                    }
                }

                Console.WriteLine($"            read {samplesCount,3} values {valueString}");
                if (!string.IsNullOrEmpty(valueString2))
                    Console.WriteLine($"                            {valueString2}");
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

        Assert.Multiple(() =>
        {
            Assert.That(readFailures, Is.EqualTo(0), "*** There have been read failures.");
            Assert.That(readSamplesCount, Is.GreaterThan((nuint)0), "*** No samples received.");
        });
    }


    [Test]
    public void Test_0501_ProcedureFailTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        Console.WriteLine("CreateProcedure");
        var procedure = CoreTypesFactory.CreateProcedure(MyBoolCallbackProcedure);

        Console.WriteLine("Dispatch(\"this is not a boolean\")");
        Assert.Throws<OpenDaqException>(() => procedure.Dispatch("this is not a boolean"));
    }

    [Test]
    public void Test_0502_ProcedureFailTest2()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        Console.WriteLine("CreateProcedure");
        var procedure = CoreTypesFactory.CreateProcedure(MyBoolCallbackProcedure);

        Console.WriteLine("Dispatch(null)");
        Assert.Throws<OpenDaqException>(() => procedure.Dispatch(null));
    }

    [Test]
    public void Test_0503_ProcedureNullFailTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        Console.WriteLine("CreateProcedure");
        var procedure = CoreTypesFactory.CreateProcedure(MyNullCallbackProcedure);

        Console.WriteLine("Dispatch(\"this is not null\")");
        Assert.Throws<OpenDaqException>(() => procedure.Dispatch("this is not null"));
    }


    [Test]
    public void Test_0511_ProcedureTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        Console.WriteLine("CreateProcedure");
        var procedure = CoreTypesFactory.CreateProcedure(MyBoolCallbackProcedure);

        Console.WriteLine("Dispatch(false)");
        procedure.Dispatch(false);

        Console.WriteLine("Dispatch(true)");
        procedure.Dispatch(true);
    }

    [Test]
    public void Test_0512_ProcedureMultiTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        Console.WriteLine("2x CreateProcedure");
        var procedure1 = CoreTypesFactory.CreateProcedure(MyBoolCallbackProcedure);
        var procedure2 = CoreTypesFactory.CreateProcedure(MyStringCallbackProcedure);

        Console.WriteLine("1: Dispatch(false)");
        procedure1.Dispatch(false);

        Console.WriteLine("2: Dispatch(\"Hello\")");
        procedure2.Dispatch("Hello");

        Console.WriteLine("1: Dispatch(true)");
        procedure1.Dispatch(true);

        Console.WriteLine("2: Dispatch(\"World\")");
        procedure2.Dispatch("World");
    }

    [Test]
    public void Test_0513_ProcedureListArgTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        Console.WriteLine("CreateProcedure");
        var procedure = CoreTypesFactory.CreateProcedure(MyStringListCallbackProcedure);

        Console.WriteLine("CreateList");
        var stringList = CoreTypesFactory.CreateList<StringObject>();

        stringList.Add("string 1");
        stringList.Add("string 2");
        stringList.Add("string 3");

        Console.WriteLine("Dispatch(stringList)");
        procedure.Dispatch((ListObject<StringObject>)stringList);
    }

    [Test]
    public void Test_0514_ProcedureNullTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        Console.WriteLine("CreateProcedure");
        var procedure = CoreTypesFactory.CreateProcedure(MyNullCallbackProcedure);

        Console.WriteLine("Dispatch(null)");
        procedure.Dispatch(null);
    }


    [Test]
    public void Test_0551_FunctionFailTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        Console.WriteLine("CreateFunction");
        var function = CoreTypesFactory.CreateFunction(MyBoolCallbackFunction);

        Console.WriteLine("Call(\"this is not a boolean\")");
        Assert.Throws<OpenDaqException>(() => function.Call("this is not a boolean"));
    }


    [Test]
    public void Test_0561_FunctionBoolToBoolTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        Console.WriteLine("CreateFunction");
        var function = CoreTypesFactory.CreateFunction(MyBoolCallbackFunction);

        Console.WriteLine("Call(false)");
        var result = function.Call(false)?.Cast<BoolObject>()?.Value;
        Assert.That(result, Is.Not.Null, "*** Unexpected call result type.");
        Assert.That(result, Is.False, "*** Unexpected call result for parameter 'false'.");

        Console.WriteLine("Call(true)");
        result = function.Call(true)?.Cast<BoolObject>()?.Value;
        Assert.That(result, Is.Not.Null, "*** Unexpected call result type.");
        Assert.That(result, Is.True, "*** Unexpected call result for parameter 'true'.");
    }

    [Test]
    public void Test_0563_FunctionIntListToStringListTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        Console.WriteLine("CreateFunction");
        var function = CoreTypesFactory.CreateFunction(MyIntegerToStringListCallbackFunction);

        Console.WriteLine("CreateList");
        var integerList = CoreTypesFactory.CreateList<IntegerObject>();

        integerList.Add(1);
        integerList.Add(2);
        integerList.Add(3);
        integerList.Add(4);

        Console.WriteLine("Call(integerList)");
        var result = function.Call((ListObject<IntegerObject>)integerList)?.CastList<StringObject>();
        Assert.That(result, Is.Not.Null, "*** Unexpected call result type.");
        Assert.That(result, Has.Count.EqualTo(integerList.Count), "*** Unexpected call result.");

        Console.WriteLine($"-> returned {result.Count} strings:");
        foreach (var stringObj in result)
            Console.WriteLine($"   - {stringObj}");
    }

    [Test]
    public void Test_0563_FunctionNullToStringTest()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");

        Console.WriteLine("CreateFunction");
        var function = CoreTypesFactory.CreateFunction(MyNullToStringCallbackFunction);

        Console.WriteLine("Call(null)");
        var result = function.Call(null)?.Cast<StringObject>();
        Assert.That(result, Is.Not.Null, "*** Unexpected call result type.");

        Console.WriteLine($"-> returned \"{result}\"");

        Console.WriteLine("Call(\"not null\")");
        result = function.Call("not null")?.Cast<StringObject>();
        Assert.That(result, Is.Not.Null, "*** Unexpected call result type.");

        Console.WriteLine($"-> returned \"{result}\"");
    }

    [Test]
    public void Test_0564_Search()
    {
        using var daqInstance = OpenDAQFactory.Instance(".");
        using var device = ConnectFirstDaqRefDevice(daqInstance);
        Assert.That(device, Is.Not.Null);

        using var componentFilter = CoreTypesFactory.CreateRecursiveSearchFilter(OpenDAQFactory.CreateLocalIdSearchFilter("RefCh0"));
        Assert.That(componentFilter, Is.Not.Null);

        using var propertyFilter = CoreObjectsFactory.CreateNamePropertyFilter("^Amplitude$");
        Assert.That(propertyFilter, Is.Not.Null);

        var properties = device.FindProperties(propertyFilter, componentFilter);
        ulong propertyCount = (ulong)properties.Count;
        Console.WriteLine($"  found {propertyCount} properties");
        Assert.That(propertyCount, Is.EqualTo(1));

        using var property = properties[0];
        Console.WriteLine($"Channel found property {property.Name} original value: {property.Value}");
        property.Value = 7;

        var components = device.GetItems(componentFilter);
        ulong componentCount = (ulong)components.Count;
        Console.WriteLine($"  found {componentCount} components");
        Assert.That(componentCount, Is.EqualTo(1));

        using var component = components[0];
        Console.WriteLine($"Found component Amplitude property modified value: {component.GetPropertyValue("Amplitude")}");
    }
}
