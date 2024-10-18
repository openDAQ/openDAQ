/**
 * Part of the openDAQ stand-alone application quick start guide. Starts
 * an openDAQ server on localhost that contains a simulated device.
 */

using Daq.Core.Types;
using Daq.Core.Objects;
using Daq.Core.OpenDAQ;

PropertyObject config = CoreObjectsFactory.CreatePropertyObject();
config.AddProperty(CoreObjectsFactory.CreateStringProperty("Name", "Reference device simulator", visible: true));
config.AddProperty(CoreObjectsFactory.CreateStringProperty("LocalId", "RefDevSimulator", visible: true));
config.AddProperty(CoreObjectsFactory.CreateStringProperty("SerialNumber", "sim01", visible: true));

var instanceBuilder = OpenDAQFactory.InstanceBuilder();
instanceBuilder.AddModulePath(".");
instanceBuilder.AddDiscoveryServer("mdns");
instanceBuilder.SetRootDevice("daqref://device1", config);
var instance = instanceBuilder.Build();

var servers = instance.AddStandardServers();

foreach (var server in servers)
    server.EnableDiscovery();

Console.WriteLine();
Console.Write("Press a key to exit the application ...");
Console.ReadKey(intercept: true);
