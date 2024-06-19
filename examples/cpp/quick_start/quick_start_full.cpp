#include <opendaq/opendaq.h>
#include <iostream>
#include <chrono>
#include <thread>

using namespace std::literals::chrono_literals;
using namespace date;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create a fresh openDAQ(TM) instance that we will use for all the interactions with the openDAQ(TM) SDK
    daq::InstancePtr instance = daq::Instance(MODULE_PATH);

    // Find and connect to a simulator device
    const auto availableDevices = instance.getAvailableDevices();
    daq::DevicePtr device;
    for (const auto& deviceInfo : availableDevices)
    {
        if (deviceInfo.getName() == "Reference device simulator")
        {
            device = instance.addDevice(deviceInfo.getConnectionString());
            break; 
        }        
    }

    // Exit if no device is found
    if (!device.assigned())
        return 0;

    // Output the name of the added device
    std::cout << device.getInfo().getName() << std::endl;
	
    // Get the first signal of the first device's channel
    daq::ChannelPtr channel = device.getChannels()[0];
    daq::SignalPtr signal = channel.getSignals()[0];

    // Print out the last value of the signal
    std::cout << signal.getLastValue() << std::endl;
	    
	// Output 40 samples using reader
    daq::StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal);

    // Allocate buffer for reading double samples
    double samples[100];
    
    for (int i = 0; i < 40; ++i)
    {
        std::this_thread::sleep_for(25ms);

        // Read up to 100 samples, storing the amount read into `count`
        daq::SizeT count = 100;
        reader.read(samples, &count);
        if (count > 0)
            std::cout << samples[count - 1] << std::endl;
    }

    // Get the resolution and origin
    daq::DataDescriptorPtr descriptor = signal.getDomainSignal().getDescriptor();
    daq::RatioPtr resolution = descriptor.getTickResolution();
    daq::StringPtr origin = descriptor.getOrigin();
    daq::StringPtr unitSymbol = descriptor.getUnit().getSymbol();

    std::cout << "Origin: " << origin << std::endl;

    // Allocate buffer for reading domain samples
    uint64_t domainSamples[100];

    for (int i = 0; i < 40; ++i)
    {
        std::this_thread::sleep_for(25ms);

        // Read up to 100 samples, storing the amount read into `count`
        daq::SizeT count = 100;
        reader.readWithDomain(samples, domainSamples, &count);
        if (count > 0)
        {
            // Scale the domain value to the Signal unit (seconds)
            daq::Float domainValue = (daq::Int) domainSamples[count - 1] * resolution;
            std::cout << "Value: " << samples[count - 1] << ", Domain: " << domainValue << unitSymbol << std::endl;
        }
    }

    // From here on the reader returns system-clock time-points for the domain values
    auto timeReader = daq::TimeReader(reader);

    // Allocate buffer for reading domain samples
    std::chrono::system_clock::time_point timeStamps[100];

    for (int i = 0; i < 40; ++i)
    {
        std::this_thread::sleep_for(25ms);

        // Read up to 100 samples, storing the amount read into `count`
        daq::SizeT count = 100;
        timeReader.readWithDomain(samples, timeStamps, &count);
        if (count > 0)
            std::cout << "Value: " << samples[count - 1] << ", Time: " << timeStamps[count - 1] << std::endl;
    }

    // Create an instance of the renderer function block
    daq::FunctionBlockPtr renderer = instance.addFunctionBlock("ref_fb_module_renderer");

    // Connect the first output signal of the device to the renderer
    renderer.getInputPorts()[0].connect(signal);

    // Create an instance of the statistics function block
    daq::FunctionBlockPtr statistics = instance.addFunctionBlock("ref_fb_module_statistics");

    // Connect the first output signal of the device to the statistics
    statistics.getInputPorts()[0].connect(signal);

    // Connect the first output signal of the statistics to the renderer
    renderer.getInputPorts()[1].connect(statistics.getSignals()[0]);

    // List the names of all properties
    for (const daq::PropertyPtr& prop : channel.getVisibleProperties())
        std::cout << prop.getName() << std::endl;

    // Set the frequency to 5 Hz
    channel.setPropertyValue("Frequency", 5);
    // Set the noise amplitude to 0.75
    channel.setPropertyValue("NoiseAmplitude", 0.75);

    // Modulate the signal amplitude by a step of 0.1 every 25ms.
    double amplStep = 0.1;
    for (int i = 0; i < 200; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        const double ampl = channel.getPropertyValue("Amplitude");
        if (9.95 < ampl || ampl < 1.05)
            amplStep *= -1;
        channel.setPropertyValue("Amplitude", ampl + amplStep);
    }

    return 0;
}
