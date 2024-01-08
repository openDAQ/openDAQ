#include <chrono>
#include <iostream>
#include <thread>
#include <opendaq/opendaq.h>

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create a new Instance that we will use for all the interactions with the SDK
    daq::InstancePtr instance = daq::Instance(MODULE_PATH);

    // Find and connect to a device hosting an OPC UA TMS server
    const auto availableDevices = instance.getAvailableDevices();
    daq::DevicePtr device;
    for (const auto& deviceInfo : availableDevices)
    {
        if (deviceInfo.getConnectionString().toView().find("daq.opcua://") != std::string::npos)
        {
            device = instance.addDevice(deviceInfo.getConnectionString());
            break;
        }
    }

    // Exit if no device is found
    if (!device.assigned())
    {
        std::cerr << "No relevant device found!" << std::endl;
        return 0;
    }

    // Output the name of the added device
    std::cout << device.getInfo().getName() << std::endl;

    // Output 10 samples using reader
    using namespace std::chrono_literals;

    // Get the first channel and its signal
    daq::ChannelPtr channel = device.getChannels()[0];
    daq::SignalPtr signal = channel.getSignals()[0];

    daq::StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal);

    // Get the resolution and origin
    daq::DataDescriptorPtr descriptor = signal.getDomainSignal().getDescriptor();
    daq::RatioPtr resolution = descriptor.getTickResolution();
    daq::StringPtr origin = descriptor.getOrigin();
    daq::StringPtr unitSymbol = descriptor.getUnit().getSymbol();

    std::cout << "Reading signal: " << signal.getName() << std::endl;
    std::cout << "Origin: " << origin << std::endl;

    // Allocate buffer for reading double samples
    double samples[100];
    uint64_t domainSamples[100];
    int cnt = 0;
    while (cnt < 40)
    {
        std::this_thread::sleep_for(100ms);

        // Read up to 100 samples every 25ms, storing the amount read into `count`
        daq::SizeT count = 100;
        reader.readWithDomain(samples, domainSamples, &count);
        if (count > 0)
        {
            daq::Float domainValue = (daq::Int) domainSamples[count - 1] * resolution;
            std::cout << "Value: " << samples[count - 1] << ", Domain: " << domainValue << unitSymbol << std::endl;
            cnt++;
        }
    }

    using namespace date;

    // From here on the reader returns system-clock time-points as a domain
    auto timeReader = daq::TimeReader(reader);

    // Allocate buffer for reading time-stamps
    std::chrono::system_clock::time_point timeStamps[100];
    cnt = 0;
    while (cnt < 40)
    {
        std::this_thread::sleep_for(100ms);

        // Read up to 100 samples every 25ms, storing the amount read into `count`
        daq::SizeT count = 100;
        reader.readWithDomain(samples, timeStamps, &count);
        if (count > 0)
        {
            std::cout << "Value: " << samples[count - 1] << ", Domain: " << timeStamps[count - 1] << std::endl;
            cnt++;
        }
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
    for (daq::PropertyPtr prop : channel.getVisibleProperties())
        std::cout << prop.getName() << std::endl;

    // Set the frequency to 5Hz
    channel.setPropertyValue("Frequency", 5);
    // Set the noise amplitude to 0.75
    channel.setPropertyValue("NoiseAmplitude", 0.75);

    // Modulate the signal amplitude by a step of 0.1 every 25ms. Modulate for 15 seconds.
    double amplStep = 0.1;
    for (cnt = 0; cnt < 400; ++cnt)
    {
        std::this_thread::sleep_for(25ms);

        const double ampl = channel.getPropertyValue("Amplitude");
        if (9.95 < ampl || ampl < 1.05)
            amplStep *= -1;

        channel.setPropertyValue("Amplitude", ampl + amplStep);
    }

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
