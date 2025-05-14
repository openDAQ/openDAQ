/**
 * Part of the openDAQ stand-alone application quick start guide. Starts
 * an openDAQ server on localhost that contains a simulated device.
 */

using Daq.Core.Types;
using Daq.Core.Objects;
using Daq.Core.OpenDAQ;

// Create a simulation-device configuration object
PropertyObject config = CoreObjectsFactory.CreatePropertyObject();
config.AddProperty(CoreObjectsFactory.CreateStringProperty("Name", "Reference device simulator", visible: true));
config.AddProperty(CoreObjectsFactory.CreateStringProperty("LocalId", "RefDevSimulator", visible: true));
config.AddProperty(CoreObjectsFactory.CreateStringProperty("SerialNumber", "sim01", visible: true));

// Create an openDAQ(TM) instance builder to configure the instance
var instanceBuilder = OpenDAQFactory.InstanceBuilder();

// Add mDNS discovery service available for servers
instanceBuilder.AddDiscoveryServer("mdns");

// Set reference device as a root
instanceBuilder.SetRootDevice("daqref://device0", config);

// Creating a new instance from builder
var instance = instanceBuilder.Build();

// Start the openDAQ OPC UA and native streaming servers
var servers = instance.AddStandardServers();

foreach (var server in servers)
    server.EnableDiscovery();

Console.WriteLine();
Console.Write("Press a key to exit the application ...");
Console.ReadKey(intercept: true);
Console.WriteLine();
