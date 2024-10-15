using Daq.Core.Types;
using Daq.Core.Objects;
using Daq.Core.OpenDAQ;

// Create a fresh openDAQ(TM) instance that we will use for all the interactions with the openDAQ(TM) SDK
var instance = OpenDAQFactory.Instance();

// Find and connect to a simulator device
var availableDevices = instance.AvailableDevices;
Device device = null;
foreach (var deviceInfo in availableDevices)
{
    if (deviceInfo.Name == "Reference device simulator")
    {
        device = instance.AddDevice(deviceInfo.ConnectionString);
        break;
    }
}

// Exit if no device is found
if (device == null)
    return;

// Output the name of the added device
Console.WriteLine(device.Info.Name);

// Get the first signal of the first device's channel
var channel = device.GetChannels()[0];
var signal = channel.GetSignals()[0];

// Print out the last value of the signal
Console.WriteLine((double)signal.LastValue);

// Output 40 samples using reader
var reader = OpenDAQFactory.CreateStreamReader<double, UInt64>(signal);

// Allocate buffer for reading double samples
var samples = new double[100];

for (int i = 0; i < 40; ++i)
{
    System.Threading.Thread.Sleep(25);

    // Read up to 100 samples, storing the amount read into `count`
    nuint count = 100;
    reader.Read(samples, ref count);
    if (count > 0)
        Console.WriteLine(samples[count - 1]);
}

// Get the resolution and origin
var descriptor = signal.DomainSignal.Descriptor;
var resolution = descriptor.TickResolution;
var origin = descriptor.Origin;
var unitSymbol = descriptor.Unit.Symbol;

Console.WriteLine($"Origin: {origin}");

// Allocate buffer for reading domain samples
var domainSamples = new UInt64[100];

for (int i = 0; i < 40; ++i)
{
    System.Threading.Thread.Sleep(25);

    // Read up to 100 samples, storing the amount read into `count`
    nuint count = 100;
    reader.ReadWithDomain(samples, domainSamples, ref count);
    if (count > 0)
    {
        // Scale the domain value to the Signal unit (seconds)
        double domainValue = (long)domainSamples[count - 1] * resolution;
        Console.WriteLine($"Value: {samples[count - 1]}, Domain: {domainValue}{unitSymbol}");
    }
}

// Wrap the reader to return time-points as a domain
var timeReader = OpenDAQFactory.CreateTimeReader(reader, signal);

// Allocate buffer for reading domain samples
var timeStamps = new DateTime[100];

for (int i = 0; i < 40; ++i)
{
    System.Threading.Thread.Sleep(25);

    // Read up to 100 samples, storing the amount read into `count`
    nuint count = 100;
    timeReader.ReadWithDomain(samples, timeStamps, ref count);
    if (count > 0)
        Console.WriteLine($"Value: {samples[count - 1]}, Time: {timeStamps[count - 1]}");
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
channel.SetPropertyValue("NoiseAmplitude", 0.75);

// Modulate the signal amplitude by a step of 0.1 every 25ms.
double amplStep = 0.1;
for (int i = 0; i < 200; ++i)
{
    System.Threading.Thread.Sleep(25);
    double ampl = channel.GetPropertyValue("Amplitude");
    if (9.95 < ampl || ampl < 1.05)
        amplStep *= -1;
    channel.SetPropertyValue("Amplitude", ampl + amplStep);
}
