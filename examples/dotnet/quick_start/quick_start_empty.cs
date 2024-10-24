/*
 * Part of the openDAQ stand-alone application quick start guide. The full
 * example can be found in quick_start_full.cs
 */

using Daq.Core.OpenDAQ;

// Create a fresh openDAQ(TM) instance, which acts as the entry point into our application
Console.WriteLine("create instance...");
var instance = OpenDAQFactory.Instance();

Console.WriteLine();
Console.WriteLine($"successfully created the instance '{instance.Name}'");

Console.WriteLine();
Console.Write("Press a key to exit the application ...");
Console.ReadKey(intercept: true);
