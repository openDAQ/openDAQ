/**
 * Adds a reference device, outputting its last signal value every second for 10s.
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <opendaq/opendaq.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace std::chrono_literals;

    // Create an Instance, loading modules at MODULE_PATH
    const InstancePtr instance = Instance(MODULE_PATH);

    // Add a reference device and set it as root
    auto device = instance.addDevice("daqref://device0");
    auto signal = device.getSignalsRecursive()[0];

    // Create reader used to read signal data
    auto reader = StreamReader<double, uint64_t>(signal);

    daq::DataDescriptorPtr domainDataDescriptor = signal.getDomainSignal().getDescriptor();
    daq::RatioPtr resolution = domainDataDescriptor.getTickResolution();
    daq::StringPtr origin = domainDataDescriptor.getOrigin();
    daq::StringPtr unitSymbol = domainDataDescriptor.getUnit().getSymbol();

    std::cout << "Origin: " << origin << std::endl;

    // Read last sample approximately every second, for 10s
    double samples[5000];
    uint64_t domainSamples[5000];
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(100ms);

        // Read up to 100 samples, storing the amount read into `count`
        daq::SizeT count = 5000;
        reader.readWithDomain(samples, domainSamples, &count);
        if (count > 0)
        {
            daq::Float domainValue = (daq::Int) domainSamples[count - 1] * resolution;
            std::cout << "Value: " << samples[count - 1] << " Domain: " << domainValue << unitSymbol << std::endl;
        }
    }

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
