#include <chrono>
#include <iostream>
#include <thread>
#include <opendaq/opendaq.h>

void readSamples(const daq::MirroredSignalConfigPtr signal)
{
    using namespace std::chrono_literals;
    daq::StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal);

    // Get the resolution and origin
    daq::DataDescriptorPtr descriptor = signal.getDomainSignal().getDescriptor();
    daq::RatioPtr resolution = descriptor.getTickResolution();
    daq::StringPtr origin = descriptor.getOrigin();
    daq::StringPtr unitSymbol = descriptor.getUnit().getSymbol();

    std::cout << "\nReading signal: " << signal.getName()
              << "; active streaming source: " << signal.getActiveStreamingSource() << std::endl;
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
}

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create a new Instance that we will use for all the interactions with the SDK
    daq::InstancePtr instance = daq::Instance(MODULE_PATH);

    // Get the default configuration Property object for OPC UA enabled device type
    daq::PropertyObjectPtr deviceConfig = instance.getAvailableDeviceTypes().get("opendaq_opcua_config").createDefaultConfig();

    // Allow multiple streaming protocols by device configuration
    deviceConfig.setPropertyValue("AllowedStreamingProtocols", daq::List<daq::IString>("opendaq_native_streaming", "opendaq_lt_streaming"));

    // Set websocket streaming protocol as primary by device configuration
    deviceConfig.setPropertyValue("PrimaryStreamingProtocol", "opendaq_lt_streaming");

    // Disregard direct streaming connections for nested devices,
    // establish the minimum number of streaming connections possible.
    deviceConfig.setPropertyValue("StreamingConnectionHeuristic", 0);

    // Find and connect to a device hosting an OPC UA TMS server
    const auto availableDevices = instance.getAvailableDevices();
    daq::DevicePtr device;
    for (const auto& deviceInfo : availableDevices)
    {
        for (const auto & capability : deviceInfo.getServerCapabilities())
        {
            if (capability.getProtocolName() == "openDAQ OpcUa")
            {
                device = instance.addDevice(capability.getPrimaryConnectionString(), deviceConfig);
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

    // Find the AI signal
    auto signals = device.getSignals(daq::search::Recursive(daq::search::Visible()));

    daq::ChannelPtr channel;
    daq::MirroredSignalConfigPtr signal;
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

    // Set the mirrored DI signal to use on AI
    channel.setPropertyValue("InputMux", 1);

    // Find and output the streaming sources of signal
    daq::StringPtr nativeStreamingSource;
    daq::StringPtr websocketStreamingSource;
    std::cout << "AI signal has " << signal.getStreamingSources().getCount()
              << " streaming sources:" << std::endl;
    for (const auto& source : signal.getStreamingSources())
    {
        std::cout << source << std::endl;
        if (source.toView().find("daq.ns://") != std::string::npos)
            nativeStreamingSource = source;
        if (source.toView().find("daq.lt://") != std::string::npos)
            websocketStreamingSource = source;
    }

    // Check the active streaming source of signal
    if (signal.getActiveStreamingSource() != websocketStreamingSource)
    {
        std::cerr << "Wrong active streaming source of AI signal" << std::endl;
        return 1;
    }
    // Output samples using reader with websocket streaming
    readSamples(signal);

    // Change the active streaming source of signal
    signal.setActiveStreamingSource(nativeStreamingSource);
    // Output samples using reader with native streaming
    readSamples(signal);

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
