//#define USE_LISTOBJECT
//#define USE_ITERATOR_NOT_IENUMERABLE

// Ignore Spelling: Opc Ua nullable daqref


using System.Collections;
using System.Diagnostics;

using Daq.Core.Objects;
using Daq.Core.OpenDAQ;
using Daq.Core.Types;


namespace openDaq.Net.Test;


public class OpenDaqGettingStartedGuidesTests : OpenDAQTestsBase
{
    // Corresponding document: Antora/modules/getting_started/pages/quick_start_application.adoc

    [Test]
    public void FullExampleCodeTest()
    {
        // Create a fresh openDAQ(TM) instance that we will use for all the interactions with the openDAQ(TM) SDK
        var instance = OpenDAQFactory.Instance();

        // Find and connect to a simulator device
        Device device = null;
        foreach (var deviceInfo in instance.AvailableDevices)
        {
            if (deviceInfo.ConnectionString.StartsWith("daq://openDAQ_"))
            {
                device = instance.AddDevice(deviceInfo.ConnectionString);
                break;
            }
        }

        if (device == null)
        {
            Console.WriteLine("No relevant device found!");
            return;
        }

        // Output the name of the added device
        Console.WriteLine(device.Info.Name);

        // Get the first channel and its signal
        var channel = device.GetChannels()[0];
        var signal = channel.GetSignals()[0];

        var reader = OpenDAQFactory.CreateStreamReader(signal); //defaults to CreateStreamReader<double, long>

        // Get the resolution and origin
        var descriptor = signal.DomainSignal.Descriptor;
        var resolution = descriptor.TickResolution;
        var origin = descriptor.Origin;
        var unitSymbol = descriptor.Unit.Symbol;

        Console.WriteLine($"Reading signal: {signal.Name}");
        Console.WriteLine($"Origin: {origin}");

        // Allocate buffer for reading double samples
        double[] samples = new double[100];
        long[] domainSamples = new long[100];
        int cnt = 0;
        while (cnt < 40)
        {
            Thread.Sleep(100);

            // Read up to 100 samples every 100ms, storing the amount read into `count`
            nuint count = 100;
            reader.ReadWithDomain(samples, domainSamples, ref count);
            if (count > 0)
            {
                double domainValue = (double)domainSamples[count - 1] * ((double)resolution.Numerator / resolution.Denominator);
                Console.WriteLine($"Value: {samples[count - 1]}, Domain: {domainValue}{unitSymbol}");
                cnt++;
            }
        }

        //TimeReader currently not available in .NET Bindings

        // Create an instance of the renderer function block
        var renderer = instance.AddFunctionBlock("ref_fb_module_renderer");

        // Connect the first output signal of the device to the renderer
        renderer.GetInputPorts()[0].Connect(signal);

        // Create an instance of the statistics function block
        var statistics = instance.AddFunctionBlock("ref_fb_module_statistics");

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

#if !documentation
        Stopwatch sw = Stopwatch.StartNew();
#endif

        // Modulate the signal amplitude by a step of 0.1 every 25ms.
        double amplStep = 0.1d;
        while (true)
        {
            Thread.Sleep(25);
            double ampl = channel.GetPropertyValue("Amplitude");
            if (9.95d < ampl || ampl < 1.05d)
                amplStep *= -1d;
            channel.SetPropertyValue("Amplitude", ampl + amplStep);

#if !documentation
            if (sw.Elapsed.TotalSeconds >= 3)
                break;
#endif
        }

#if !documentation
        sw.Stop();
#endif
    }
}
