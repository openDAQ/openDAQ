// See https://aka.ms/new-console-template for more information

using Daq.Core.Types;
using Daq.Core.Objects;
using Daq.Core.OpenDAQ;


Console.WriteLine("create instance...");
var instance = OpenDAQFactory.Instance();

Console.WriteLine("scan for devices...");
var availableDevices = instance.GetAvailableDevices();

Console.WriteLine($"> found {availableDevices.Count} devices:");
foreach (var device in availableDevices)
    Console.WriteLine($"  - {device.GetName()} ({device.GetConnectionString()})");

//ToDo: your code goes here
