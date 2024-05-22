#include <opendaq/opendaq.h>
#include <chrono>
#include <iostream>
#include <thread>

using namespace daq;

void readSamples(const MirroredSignalConfigPtr signal)
{
    using namespace std::chrono_literals;
    StreamReaderPtr reader = StreamReader<double, uint64_t>(signal);

    // Get the resolution and origin
    DataDescriptorPtr descriptor = signal.getDomainSignal().getDescriptor();
    RatioPtr resolution = descriptor.getTickResolution();
    StringPtr origin = descriptor.getOrigin();
    StringPtr unitSymbol = descriptor.getUnit().getSymbol();

    std::cout << "\nReading signal: " << signal.getName() << "; active Streaming source: " << signal.getActiveStreamingSource()
              << std::endl;
    std::cout << "Origin: " << origin << std::endl;

    // Allocate buffer for reading double samples
    double samples[100];
    uint64_t domainSamples[100];
    for (int i = 0; i < 40; ++i)
    {
        std::this_thread::sleep_for(25ms);

        // Read up to 100 samples every 25 ms, storing the amount read into `count`
        SizeT count = 100;
        reader.readWithDomain(samples, domainSamples, &count);
        if (count > 0)
        {
            Float domainValue = (Int) domainSamples[count - 1] * resolution;
            std::cout << "Value: " << samples[count - 1] << ", Domain: " << domainValue << unitSymbol << std::endl;
        }
    }
}

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create a new Instance that we will use for all the interactions with the SDK
    InstancePtr instance = Instance();

    // Create an empty Property object
    PropertyObjectPtr deviceConfig = PropertyObject();

    // Add property to allow multiple Streaming protocols with native protocol having the first priority
    auto prioritizedStreamingProtocols = List<IString>("opendaq_native_streaming", "opendaq_lt_streaming");
    deviceConfig.addProperty(ListProperty("PrioritizedStreamingProtocols", prioritizedStreamingProtocols));

    // Set property to disregard direct Streaming connections for nested Devices,
    // and establish the minimum number of streaming connections possible.
    const auto streamingConnectionHeuristicProp =  SelectionProperty("StreamingConnectionHeuristic",
                                                                    List<IString>("MinConnections",
                                                                                  "MinHops",
                                                                                  "NotConnected"),
                                                                    0);
    deviceConfig.addProperty(streamingConnectionHeuristicProp);

    // Find and connect to a device using device info connection string
    const auto availableDevices = instance.getAvailableDevices();
    DevicePtr device;
    for (const auto& deviceInfo : availableDevices)
    {
        if (deviceInfo.getConnectionString().toView().find("daq://") != std::string::npos)
        {
            device = instance.addDevice(deviceInfo.getConnectionString(), deviceConfig);
            break;
        }
    }

    // Exit if no Device is found
    if (!device.assigned())
    {
        std::cerr << "No relevant Device found!" << std::endl;
        return 0;
    }

    // Output the name of the added Device
    std::cout << device.getInfo().getName() << std::endl;

    // Find the AI Signal
    auto signals = device.getSignalsRecursive();

    ChannelPtr channel;
    MirroredSignalConfigPtr signal;
    for (const auto& sig : signals)
    {
        auto name = sig.getDescriptor().getName();

        if (name.toView().find("AI") != std::string_view::npos)
        {
            signal = sig;
            channel = signal.getParent().getParent();
            break;
        }
    }

    if (!signal.assigned())
    {
        std::cerr << "No AI signal found!" << std::endl;
        return 1;
    }

    // Find and output the Streaming sources of Signal
    StringPtr nativeStreamingSource;
    StringPtr websocketStreamingSource;
    std::cout << "AI signal has " << signal.getStreamingSources().getCount() << " Streaming sources:" << std::endl;
    for (const auto& source : signal.getStreamingSources())
    {
        std::cout << source << std::endl;
        if (source.toView().find("daq.ns://") != std::string::npos)
            nativeStreamingSource = source;
        if (source.toView().find("daq.lt://") != std::string::npos)
            websocketStreamingSource = source;
    }

    // Check the active Streaming source of Signal
    if (signal.getActiveStreamingSource() != websocketStreamingSource)
    {
        std::cerr << "Wrong active Streaming source of AI signal" << std::endl;
        return 1;
    }
    // Output samples using Reader with websocket Streaming
    readSamples(signal);

    // Change the active Streaming source of Signal
    signal.setActiveStreamingSource(nativeStreamingSource);
    // Output samples using Reader with native Streaming
    readSamples(signal);

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
