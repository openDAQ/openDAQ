/**
 * Part of the openDAQ howto guides.
 * Starts an openDAQ server on localhost that contains a simulated device.
 */

using Daq.Core.Types;
using Daq.Core.Objects;
using Daq.Core.OpenDAQ;

// Make the static members and nested types of factories directly accessible (usage like in C++)
using static Daq.Core.Types.CoreTypesFactory;
using static Daq.Core.Objects.CoreObjectsFactory;
using static Daq.Core.OpenDAQ.OpenDAQFactory;


// Create a simulation-device configuration object
PropertyObject config = CreatePropertyObject();
config.AddProperty(CreateStringProperty("Name", "Reference device simulator", visible: true));
config.AddProperty(CreateStringProperty("LocalId", "RefDevSimulator", visible: true));
config.AddProperty(CreateStringProperty("SerialNumber", "sim01", visible: true));

// Create an openDAQ(TM) instance builder to configure the instance
var instanceBuilder = InstanceBuilder();

// Add mDNS discovery service available for servers
instanceBuilder.AddDiscoveryServer("mdns");

// Set reference device as a root
instanceBuilder.SetRootDevice("daqref://device1", config);

// Creating a new instance from builder
var instance = instanceBuilder.Build();

// Creates and registers a Server capability with the ID `OpenDAQLTStreaming` and the default port number 7414
var ltsServer = instance.AddServer("OpenDAQLTStreaming", null);
Console.WriteLine($"Enabling discovery of {ltsServer.Name} ...");
ltsServer.EnableDiscovery();

// Start the openDAQ OPC UA and native streaming servers
var servers = instance.AddStandardServers();

foreach (var server in servers)
{
    Console.WriteLine($"Enabling discovery of {server.Name} ...");
    server.EnableDiscovery();
}

Console.WriteLine();
Console.Write("Press a key to exit the application ...");
Console.ReadKey(intercept: true);
Console.WriteLine();
