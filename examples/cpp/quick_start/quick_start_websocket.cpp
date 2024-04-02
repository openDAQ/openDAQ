/**
 * Part of the openSDK stand-alone application quick start guide. Contains
 * the full example that user should obtain at the end of the guide.
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <opendaq/opendaq.h>

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create a new Instance that we will use for all the interactions with the SDK
    daq::InstancePtr instance = daq::Instance(MODULE_PATH);

    // Find and connect to a device hosting a WebSocket server
    const auto availableDevices = instance.getAvailableDevices();
    daq::DevicePtr device;
    for (const auto& deviceInfo : availableDevices)
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (capability.getProtocolName() == "openDAQ WebsocketTcp Streaming")
            {
                device = instance.addDevice(capability.getPrimaryConnectionString());
                break;
            }
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

    auto signals = device.getSignals();

    daq::SignalPtr signal;
    for (const auto& sig : signals)
    {
        auto descriptor = sig.getDescriptor();
        if (descriptor.assigned())
        {
            auto descName = descriptor.getName();

            if (descName == "AI")
            {
                signal = sig;
                break;
            }
        }
    }

    if (!signal.assigned())
    {
        std::cerr << "No AI signal found!" << std::endl;
        return 1;
    }

    // Set the mirrored DI signal to use on AI
    // channel.setPropertyValue("InputMux", 1);

    daq::StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal);

    // Get the resolution and origin
    daq::DataDescriptorPtr descriptor = signal.getDomainSignal().getDescriptor();
    daq::RatioPtr resolution = descriptor.getTickResolution();
    daq::StringPtr origin = descriptor.getOrigin();
    daq::StringPtr unitSymbol = descriptor.getUnit().getSymbol();

    std::cout << "Origin: " << origin << std::endl;

    // Allocate buffer for reading double samples
    double samples[100];
    uint64_t domainSamples[100];
    for (int i = 0; i < 40; ++i)
    {
        std::this_thread::sleep_for(25ms);

        // Read up to 100 samples every 25ms, storing the amount read into `count`
        daq::SizeT count = 100;
        reader.readWithDomain(samples, domainSamples, &count);
        if (count > 0)
        {
            daq::Float domainValue = (daq::Int) domainSamples[count - 1] * resolution;
            std::cout << "Value: " << samples[count - 1] << ", Domain: " << domainValue << unitSymbol << std::endl;
        }
    }

    using namespace date;

    // From here on the reader returns system-clock time-points as a domain
    auto timeReader = daq::TimeReader(reader);

    // Allocate buffer for reading time-stamps
    std::chrono::system_clock::time_point timeStamps[100];
    for (int i = 0; i < 40; ++i)
    {
        std::this_thread::sleep_for(25ms);

        // Read up to 100 samples every 25ms, storing the amount read into `count`
        daq::SizeT count = 100;
        reader.readWithDomain(samples, timeStamps, &count);
        if (count > 0)
        {
            std::cout << "Value: " << samples[count - 1] << ", Domain: " << timeStamps[count - 1] << std::endl;
        }
    }

    // Create an instance of the renderer function block
    daq::FunctionBlockPtr renderer = instance.addFunctionBlock("ref_fb_module_renderer");

    // Connect the first output signal of the device to the renderer
    renderer.getInputPorts()[0].connect(signal);
    // Connect the second output signal of the device to the renderer
    renderer.getInputPorts()[1].connect(device.getSignals()[2]);

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
