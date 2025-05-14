//#define USE_LISTOBJECT
//#define USE_ITERATOR_NOT_IENUMERABLE

// Ignore Spelling: Opc Ua nullable daqref


using Daq.Core.Types;
using Daq.Core.Objects;
using Daq.Core.OpenDAQ;


namespace openDaq.Net.Test;


public class OpenDaqGettingStartedGuidesTests : OpenDAQTestsBase
{
    [SetUp]
    public void Setup()
    {
        base.DontWarn();
        base.DontCollectAndFinalize();
        base.DontCheckAliveObjectCount();
    }

    //[TearDown]
    //public void TearDown()
    //{
    //}

#pragma warning disable CS8600 // Converting null literal or possible null value to non-nullable type.

    // Corresponding document: Antora/modules/getting_started/pages/quick_start_application.adoc
    [Test]
    public void FullExampleCodeTest()
    {
        // Create a fresh openDAQ(TM) instance that we will use for all the interactions with the openDAQ(TM) SDK
        var instance = OpenDAQFactory.Instance();

        // Find the simulator device
        var deviceInfo = instance.AvailableDevices.FirstOrDefault(devInfo => devInfo.Name == "Reference device simulator");
        if (deviceInfo == null)
        {
            Console.WriteLine("No relevant device found!");
            return;
        }

        // Connect to the simulator device
        var device = instance.AddDevice(deviceInfo.ConnectionString);
        if (device == null)
        {
            Console.WriteLine("Device connection failed!");
            return;
        }

        // Output the name of the added device
        Console.WriteLine(device.Info.Name);

        // Get the first signal of the first device's channel
        var channel = device.GetChannels()[0];
        var signal  = channel.GetSignals()[0];

        // Print out the last value of the signal
        Console.WriteLine($"Using signal: '{signal.Name}'");
        Console.WriteLine($"Last value of the signal: {signal.LastValue}");

        var reader = OpenDAQFactory.CreateStreamReader(signal); //defaults to CreateStreamReader<double, long>

        // Allocate buffer for reading double samples
        double[] samples = new double[100];

        for (int i = 0; i < 40; ++i)
        {
            Thread.Sleep(25);

            // Read up to 100 samples, storing the amount read into `count`
            nuint count = 100;
            reader.Read(samples, ref count);

            // The call to Read() might return count==0 (explained in the how-to guides)
            if (count > 0)
                Console.WriteLine($"Last value of read block {i+1,2}: {samples[count - 1]}");
        }

        // Get the resolution and origin
        var descriptor = signal.DomainSignal.Descriptor;
        var resolution = descriptor.TickResolution;
        var origin     = descriptor.Origin;
        var unitSymbol = descriptor.Unit.Symbol;

        Console.WriteLine($"Domain origin: {origin}");

        // Allocate buffer for reading domain samples
        long[] domainSamples = new long[100];

        for (int i = 0; i < 40; i++)
        {
            Thread.Sleep(100);

            // Read up to 100 samples every 100ms, storing the amount read into `count`
            nuint count = 100;
            reader.ReadWithDomain(samples, domainSamples, ref count);

            // The call to ReadWithDomain() might return count==0 (explained in the how-to guides)
            if (count > 0)
            {
                // Scale the domain value to the Signal unit (seconds)
                double domainValue = (double)domainSamples[count - 1] * ((double)resolution.Numerator / resolution.Denominator);
                Console.WriteLine($"Last value of read block {i + 1,2}: {samples[count - 1]}, Domain: {domainValue}{unitSymbol}");
            }
        }

        // In contrast to C++, the time reader in .NET does not change the domain signal type of the stream reader

        // Create a time reader which uses the previously created stream reader
        var timeReader = OpenDAQFactory.CreateTimeReader(reader, signal);

        // Allocate buffer for reading timestamps
        DateTime[] timeStamps = new DateTime[100];

        for (int i = 0; i < 40; i++)
        {
            Thread.Sleep(25);

            // Read up to 100 samples, storing the amount read into `count`
            nuint count = 100;
            timeReader.ReadWithDomain(samples, timeStamps, ref count);

            // The call to ReadWithDomain() might return count==0 (explained in the how-to guides)
            if (count > 0)
                Console.WriteLine($"Last value of read block {i + 1,2}: {samples[count - 1]}, Time: {timeStamps[count - 1]:yyyy-MM-dd HH:mm:ss.fffffff}");
        }

        // Create an instance of the renderer function block
        var renderer = instance.AddFunctionBlock("RefFBModuleRenderer");

        // Connect the first output signal of the device to the renderer
        renderer.GetInputPorts()[0].Connect(signal);

        // Create an instance of the statistics function block
        var statistics = instance.AddFunctionBlock("RefFBModuleStatistics");

        // Connect the first output signal of the device to the statistics
        statistics.GetInputPorts()[0].Connect(signal);

        // Connect the first output signal of the statistics to the renderer
        renderer.GetInputPorts()[1].Connect(statistics.GetSignals()[0]);

        // List the names of all properties
        foreach (var prop in channel.VisibleProperties)
            Console.WriteLine(prop.Name);

        // Set the frequency to 5 Hz
        channel.SetPropertyValue("Frequency", 5);
        // Set the noise amplitude to 0.75
        channel.SetPropertyValue("NoiseAmplitude", 0.75d);

        // Modulate the signal amplitude by a step of 0.1 every 25ms.
        double amplStep = 0.1d;
        for (int i = 0; i < 200; ++i)
        {
            Thread.Sleep(25);
            double ampl = channel.GetPropertyValue("Amplitude");
            if ((9.95d < ampl) || (ampl < 1.05d))
                amplStep *= -1d;
            channel.SetPropertyValue("Amplitude", ampl + amplStep);
        }
    }

#pragma warning restore CS8600 // Converting null literal or possible null value to non-nullable type.
}
