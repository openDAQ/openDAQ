using System;
using System.Threading;

using Daq.Core.Types;
using Daq.Core.OpenDAQ;


// Create a fresh openDAQ(TM) instance that we will use for all the interactions with the openDAQ(TM) SDK
var daqInstance = OpenDAQFactory.Instance();

Console.WriteLine("\nget the first reference device information from device discovery...");

// Take the first reference device info
var daqrefDeviceInfo = daqInstance.AvailableDevices
                       .FirstOrDefault(deviceInfo => deviceInfo.ConnectionString.StartsWith("daqref:"));

if (daqrefDeviceInfo == null)
{
  Console.WriteLine("*** No reference device found.");
  return;
}

Console.WriteLine($"found '{daqrefDeviceInfo.Name}' ({daqrefDeviceInfo.ConnectionString})");
Console.WriteLine("connecting...");

// Connect the device
var daqrefDevice = daqInstance.AddDevice(daqrefDeviceInfo.ConnectionString);

var signals = daqrefDevice.GetSignalsRecursive();

// Take the first two signals
var signalList = CoreTypesFactory.CreateList<Signal>();
signalList.Add(signals[0]);
signalList.Add(signals[1]);

Console.WriteLine($"  using signal 0 '{signals[0].Name}'");
Console.WriteLine($"  using signal 1 '{signals[1].Name}'");

// Prepare the data structures for the samples
int        loopCount        = 10;
nuint      maxCount         = 500;
double[][] samples          = [new double[maxCount], new double[maxCount]];
int        sleepTime        = 25;
nuint      readSamplesCount = 0;

Console.WriteLine("creating MultiReader...");

// Create a MultiReader for the two signals
var reader = OpenDAQFactory.CreateMultiReader<double>(signalList);

Console.WriteLine($"  ValueReadType = {reader.ValueReadType}, DomainReadType = {reader.DomainReadType}");
Console.WriteLine($"  Reading {loopCount} times {maxCount} values (waiting {sleepTime}ms before reading)");

Console.WriteLine("reading blocks of measurement values...");

for (int loopNo = 0; loopNo < loopCount; ++loopNo)
{
  nuint count = maxCount; //reset to array size

  // Just wait a bit for some samples to be available
  Thread.Sleep(sleepTime);

  nuint availableCount = reader.AvailableCount;

  //read up to 'count' samples, storing the amount read into array 'samples'
  reader.Read(samples, ref count/*, timeoutMs: 5000*/);
  readSamplesCount += count;

  string valueString  = (count == 0) ? string.Empty : $"(signal 0 - 0: {samples[0][0]:+0.000;-0.000; 0.000} ... {count - 1,3}: {samples[0][count - 1]:+0.000;-0.000; 0.000})";
  string valueString2 = (count == 0) ? string.Empty : $"(signal 1 - 0: {samples[1][0]:+0.000;-0.000; 0.000} ... {count - 1,3}: {samples[1][count - 1]:+0.000;-0.000; 0.000})";
  Console.WriteLine($"  Loop {loopNo + 1,2}: before Read() AvailableCount was {availableCount,4} but read {count,4} values {valueString}");
  Console.WriteLine($"{" ",70}{valueString2}");
}

if (readSamplesCount == 0)
  Console.WriteLine("*** No samples received.");

Console.WriteLine();
Console.Write("Press a key to exit the application ...");
Console.ReadKey(intercept: true);
