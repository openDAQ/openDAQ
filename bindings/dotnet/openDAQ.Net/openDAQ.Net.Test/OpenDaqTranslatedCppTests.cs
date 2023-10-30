//#define USE_LISTOBJECT
//#define USE_ITERATOR_NOT_IENUMERABLE

using System.Diagnostics;

using Daq.Core.Objects;
using Daq.Core.OpenDAQ;
using Daq.Core.Types;


namespace openDaq.Net.Test;


public class OpenDaqTranslatedCppTests : CoreTypesTestsBase
{
    private const string ConnectionProtocol = "daq.opcua://";
    private const string ConnectionProtocol2 = "daq.ws://";
    private const string ConnectionProtocol3 = "daqref://";
    private const string MODULE_PATH = ".";

    //[SetUp]
    //public void Setup()
    //{
    //}

    //[TearDown]
    //public void TearDown()
    //{
    //}


    [Test]
    public void ClientDiscoveryWithIteratorTest()
    {
        /*
            // Create an Instance, loading modules at MODULE_PATH
            const InstancePtr instance = Instance(MODULE_PATH);

            // Discover all available devices, filter out all of which connection strings
            // do not start with "daq.opcua://" or "daq.ws://"
            const auto deviceInfo = instance.getAvailableDevices();
            auto devices = List<IDevice>();
            for (auto info : deviceInfo)
            {
                auto connectionString = info.getConnectionString();
                if (connectionString.toStdString().find("daq.opcua://") != std::string::npos)
                {
                    // Connect to device and store it in a list
                    auto device = instance.addDevice(connectionString);
                    devices.pushBack(device);
                }
                if (connectionString.toStdString().find("daq.ws://") != std::string::npos)
                {
                    // Connect to device and store it in a list
                    auto device = instance.addDevice(connectionString);
                    devices.pushBack(device);
                }
            }

            // Output the names and connection strings of all connected-to devices
            std::cout << "Connected devices:" << std::endl;
            for (auto device : devices)
            {
                auto info = device.getInfo();
                std::cout << "Name: " << info.getName() << ", Connection string: " << info.getConnectionString() << std::endl;
            }
         */

        // Create an Instance, loading modules at MODULE_PATH
        var instance = OpenDAQFactory.Instance(MODULE_PATH);

        Console.WriteLine("instance.GetAvailableDevices()");
        // Discover all available devices, filter out all of which connection strings
        // do not start with "daq.opcua://" or "daq.ws://"
        System.Diagnostics.Stopwatch sw = System.Diagnostics.Stopwatch.StartNew();
        var deviceInfo = instance.GetAvailableDevices();
        sw.Stop();
        Console.WriteLine($"  {deviceInfo.Count} devices available - elapsed in {sw.Elapsed.TotalMilliseconds} ms");
        sw.Reset();

        Debug.Print("+++> connecting...");
#if USE_LISTOBJECT
        var devices = CoreTypesFactory.CreateList<Device>(); //no-go, somehow with this the devices are being collected AFTER the instance and it crashes
#endif

#if USE_ITERATOR_NOT_IENUMERABLE
        var deviceInfoIterator = deviceInfo.GetEnumerator();
        while (deviceInfoIterator.MoveNext())
        {
            /*using*/ var info = deviceInfoIterator.Current;
#else
        foreach (var info in deviceInfo)
        {
#endif
            var connectionString = info.GetConnectionString();
            if (connectionString.StartsWith(ConnectionProtocol)
                || connectionString.StartsWith(ConnectionProtocol2)
               //|| connectionString.StartsWith(ConnectionProtocol3)
               )
            {
                sw.Start();

                // Connect to device and store it in a list
                using var device = instance.AddDevice(connectionString); //when 'using' is missing, there's an access violation exception in C++ on GC.Collect()

#if USE_LISTOBJECT
                devices.PushBack(device);
#endif

                sw.Stop();

                device.PrintReferenceCount();
            }
            Console.WriteLine($"  - {connectionString}");
        }

        // Output the names and connection strings of all connected-to devices
#if !USE_LISTOBJECT
        var devices = instance.GetDevices();
#endif
        Debug.Print($"+++> connected {devices.Count} devices");
        Console.WriteLine($"{devices.Count} connected devices - elapsed in {sw.Elapsed.TotalMilliseconds} ms");
#if USE_ITERATOR_NOT_IENUMERABLE
        var deviceIterator = devices.GetEnumerator();
        while (deviceIterator.MoveNext())
        {
            /*using*/ var device = deviceIterator.Current;
#else
            foreach (var device in devices)
        {
#endif
            var info = device.GetInfo();
            Console.WriteLine($"  Name: {info.GetName()}, Connection string: {info.GetConnectionString()}");

            device.PrintReferenceCount();
            //device.Dispose();
        }

        //cleanup (not in C++ code example due to Smart-Pointers)
//#if USE_ITERATOR_NOT_IENUMERABLE
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

    [Test]
    public void ClientDiscoveryWithDotNetListTest()
    {
        /*
            // Create an Instance, loading modules at MODULE_PATH
            const InstancePtr instance = Instance(MODULE_PATH);

            // Discover all available devices, filter out all of which connection strings
            // do not start with "daq.opcua://" or "daq.ws://"
            const auto deviceInfo = instance.getAvailableDevices();
            auto devices = List<IDevice>();
            for (auto info : deviceInfo)
            {
                auto connectionString = info.getConnectionString();
                if (connectionString.toStdString().find("daq.opcua://") != std::string::npos)
                {
                    // Connect to device and store it in a list
                    auto device = instance.addDevice(connectionString);
                    devices.pushBack(device);
                }
                if (connectionString.toStdString().find("daq.ws://") != std::string::npos)
                {
                    // Connect to device and store it in a list
                    auto device = instance.addDevice(connectionString);
                    devices.pushBack(device);
                }
            }

            // Output the names and connection strings of all connected-to devices
            std::cout << "Connected devices:" << std::endl;
            for (auto device : devices)
            {
                auto info = device.getInfo();
                std::cout << "Name: " << info.getName() << ", Connection string: " << info.getConnectionString() << std::endl;
            }
         */

        // Create an Instance, loading modules at MODULE_PATH
        var instance = OpenDAQFactory.Instance(MODULE_PATH);

        Console.WriteLine("instance.GetAvailableDevices()");
        // Discover all available devices, filter out all of which connection strings
        // do not start with "daq.opcua://" or "daq.ws://"
        System.Diagnostics.Stopwatch sw = System.Diagnostics.Stopwatch.StartNew();
        var deviceInfo = instance.GetAvailableDevices();
        sw.Stop();
        Console.WriteLine($"  {deviceInfo.Count} devices available - elapsed in {sw.Elapsed.TotalMilliseconds} ms");
        sw.Reset();

        //use local list
        var devices = new List<Device>(); //not using Daq.Core.Types.ListObject

#if USE_ITERATOR_NOT_IENUMERABLE
        var deviceInfoIterator = deviceInfo.GetEnumerator();
        while (deviceInfoIterator.MoveNext())
        {
            var info = deviceInfoIterator.Current;
#else
        foreach (var info in deviceInfo)
        {
#endif
            var connectionString = info.GetConnectionString();
            if (connectionString.StartsWith(ConnectionProtocol)
                || connectionString.StartsWith(ConnectionProtocol2)
               //|| connectionString.StartsWith(ConnectionProtocol3)
               )
            {
                sw.Start();

                // Connect to device and store it in a list
                var device = instance.AddDevice(connectionString);
                devices.Add(device);

                sw.Stop();

                device.PrintReferenceCount();
            }
            Console.WriteLine($"  - {connectionString}");
        }

        // Output the names and connection strings of all connected-to devices
        Console.WriteLine($"{devices.Count} connected devices - elapsed in {sw.Elapsed.TotalMilliseconds} ms");
        foreach (var device in devices)
        {
            var info = device.GetInfo();
            Console.WriteLine($"  Name: {info.GetName()}, Connection string: {info.GetConnectionString()}");

            device.PrintReferenceCount();
        }

        //cleanup (not in C++ code example due to Smart-Pointers)
        foreach (var device in devices)
        {
            instance.RemoveDevice(device);
//            device.Dispose();
        }
        devices.Clear();
    }

    [Test]
    public void ClientLocalTest()
    {
        /*
            // Create an Instance, loading modules at MODULE_PATH
            const InstancePtr instance = Instance(MODULE_PATH);

            // Connect to device on localhost
            auto device = instance.addDevice("daq.opcua://127.0.0.1");

            // Output the name and connection string of connected-to device
            std::cout << "Connected device:" << std::endl;
            auto info = device.getInfo();
            std::cout << "Name: " << info.getName() << ", Connection string: " << info.getConnectionString() << std::endl;
         */

        // Create an Instance, loading modules at MODULE_PATH
        var instance = OpenDAQFactory.Instance(MODULE_PATH);

        // Connect to device on localhost
        var device = instance.AddDevice("daq.opcua://127.0.0.1");

        // Output the name and connection string of connected-to device
        Console.WriteLine("Connected device:");
        var info = device.GetInfo();
        Console.WriteLine($"  Name: {info.GetName()}, Connection string: {info.GetConnectionString()}");
    }

    [Test]
    public void FunctionBlockExampleTest()
    {
        /*
            // Create an Instance, loading modules at MODULE_PATH
            const InstancePtr instance = Instance(MODULE_PATH);

            // Add a reference device and set it as root
            auto device = instance.addDevice("daqref://device0");

            // Add statistics and renderer function block
            FunctionBlockPtr statistics = instance.addFunctionBlock("ref_fb_module_statistics");
            FunctionBlockPtr renderer = instance.addFunctionBlock("ref_fb_module_renderer");

            // Set renderer to draw 2.5s of data
            renderer.setPropertyValue("Duration", 2.5);

            // Get channel and signal of reference device
            const auto sineChannel = device.getChannels()[0];
            const auto sineSignal = sineChannel.getSignals()[0];

            // Add noise to the sine wave
            sineChannel.setPropertyValue("NoiseAmplitude", 1);

            // Connect the signals to the renderer and statistics
            statistics.getInputPorts()[0].connect(sineSignal);
            const auto averagedSine = statistics.getSignalsRecursive()[0];

            renderer.getInputPorts()[0].connect(sineSignal);
            renderer.getInputPorts()[1].connect(averagedSine);

            // Process and render data for 10s, modulating the amplitude
            double ampl_step = 0.1;
            for (int i = 0; i < 400; ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
                const double ampl = sineChannel.getPropertyValue("Amplitude");
                if (9.95 < ampl || ampl < 3.05)
                    ampl_step *= -1;
                sineChannel.setPropertyValue("Amplitude", ampl + ampl_step);
            }
         */

        // Create an Instance, loading modules at MODULE_PATH
        var instance = OpenDAQFactory.Instance(MODULE_PATH);

        // Add a reference device and set it as root
        var device = instance.AddDevice("daqref://device0");

        // Add statistics and renderer function block
        var statistics = instance.AddFunctionBlock("ref_fb_module_statistics");
        var renderer = instance.AddFunctionBlock("ref_fb_module_renderer");

        // Set renderer to draw 2.5s of data
        renderer.SetPropertyValue("Duration", 2.5);

        // Get channel and signal of reference device
        var sineChannel = device.GetChannels()[0];
        var sineSignal = sineChannel.GetSignals()[0];

        // Add noise to the sine wave
        sineChannel.SetPropertyValue("NoiseAmplitude", 1);

        // Connect the signals to the renderer and statistics
        statistics.GetInputPorts()[0].Connect(sineSignal);
        var averagedSine = statistics.GetSignalsRecursive()[0];

        renderer.GetInputPorts()[0].Connect(sineSignal);
        renderer.GetInputPorts()[1].Connect(averagedSine);

        // Process and render data for 10s, modulating the amplitude
        double ampl_step = 0.1;
        for (int i = 0; i < 400; ++i)
        {
            Thread.Sleep(25);
            double ampl = sineChannel.GetPropertyValue("Amplitude");
            if (9.95 < ampl || ampl < 3.05)
                ampl_step *= -1;
            sineChannel.SetPropertyValue("Amplitude", ampl + ampl_step);
        }
    }
}
