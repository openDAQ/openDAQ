/*
 * Part of the openSDK stand-alone application quick start guide. Contains
 * the full example that user should obtain at the end of the guide.
 */

using Daq.Core.Types;
using Daq.Core.Objects;
using Daq.Core.OpenDAQ;

// Create a fresh openDAQ(TM) instance, which acts as the entry point into our application
var instance = OpenDAQFactory.Instance();

// Find and connect to a device hosting a WebSocket server
var availableDevices = instance.AvailableDevices;
Device device = null;
foreach (var deviceInfo in availableDevices)
{
    foreach (var capability in deviceInfo.ServerCapabilities)
    {
        if (capability.ProtocolName == "OpenDAQLTStreaming")
        {
            device = instance.AddDevice(capability.ConnectionString);
            break;
        }
    }
}

// Exit if no device is found
if (device == null)
{
    Console.WriteLine("*** No relevant device found!");
    return;
}

// Output the name of the added device
Console.WriteLine(device.Info.Name);

// Output 10 samples using reader

var signals = device.GetSignals();

Signal signal = null;
foreach (var sig in signals)
{
    var signalDescriptor = sig.Descriptor;
    if (signalDescriptor == null)
        continue;

    var descName = signalDescriptor.Name;

    if (descName == "AI")
    {
        signal = sig;
        break;
    }
}

if (signal == null)
{
    Console.WriteLine("*** No AI signal found!");
    return;
}

var reader = OpenDAQFactory.CreateStreamReader<double, UInt64>(signal);

// Get the resolution and origin
var descriptor = signal.DomainSignal.Descriptor;
var resolution = descriptor.TickResolution;
var origin = descriptor.Origin;
var unitSymbol = descriptor.Unit.Symbol;

Console.WriteLine($"Origin: {origin}");

// Allocate buffer for reading double samples
var samples = new double[100];
var domainSamples = new UInt64[100];
for (int i = 0; i < 40; ++i)
{
    System.Threading.Thread.Sleep(25);

    // Read up to 100 samples every 25ms, storing the amount read into `count`
    nuint count = 100;
    reader.ReadWithDomain(samples, domainSamples, ref count);
    if (count > 0)
    {
        double domainValue = (long)domainSamples[count - 1] * resolution;
        Console.WriteLine($"Value: {samples[count - 1]}, Domain: {domainValue}{unitSymbol}");
    }
}

// Wrap the reader to return time-points as a domain
var timeReader = OpenDAQFactory.CreateTimeReader(reader, signal);

// Allocate buffer for reading time-stamps
var timeStamps = new DateTime[100];
for (int i = 0; i < 40; ++i)
{
    System.Threading.Thread.Sleep(25);

    // Read up to 100 samples every 25ms, storing the amount read into `count`
    nuint count = 100;
    timeReader.ReadWithDomain(samples, timeStamps, ref count);
    if (count > 0)
    {
        Console.WriteLine($"Value: {samples[count - 1]}, Domain: {timeStamps[count - 1]}");
    }
}

// Create an instance of the renderer function block
var renderer = instance.AddFunctionBlock("RefFBModuleRenderer");

// Connect the first output signal of the device to the renderer
renderer.GetInputPorts()[0].Connect(signal);
// Connect the second output signal of the device to the renderer
renderer.GetInputPorts()[1].Connect(device.GetSignals()[2]);

Console.WriteLine();
Console.Write("Press a key to exit the application ...");
Console.ReadKey(intercept: true);
