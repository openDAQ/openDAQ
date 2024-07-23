#include <opendaq/opendaq.h>
#include <iostream>
#include <chrono>
#include <thread>

using namespace std::literals::chrono_literals;
using namespace date;
using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create a fresh openDAQ(TM) instance that we will use for all the interactions with the openDAQ(TM) SDK
    daq::InstancePtr instance = daq::Instance(MODULE_PATH);
    auto device = instance.addDevice("daqref://device0");
	
    // Get the first signal of the first device's channel 
    daq::SignalPtr signalA0 = device.getChannels()[0].getSignals()[0];
    daq::SignalPtr signalA1 = device.getChannels()[1].getSignals()[0];

    std::cout << "Signal A0: " << signalA0.getName() << std::endl;
    std::cout << "Signal A1: " << signalA1.getName() << std::endl;


    auto signal = device.getSignalsRecursive()[0];

    size_t count = 100;
    std::vector<double> samples(count);
    std::vector<int64_t> domainSamples(count);

    auto reader = StreamReader<double, int64_t>(signal);

    SizeT readSamplesCount = 0;

    for (int i = 0; i < 10; ++i)
    {
        count = 100;
        int loopCount = 40;
        do 
        {
            std::this_thread::sleep_for(25ms);
        } while (reader.getEmpty() && (--loopCount > 0));
      
        SizeT availableSamples = reader.getAvailableCount();
        std::cout << "Available samples: " << availableSamples << std::endl;

        auto status = reader.readWithDomain(samples.data(), domainSamples.data(), &count, 1000);
        readSamplesCount += count;

        if (status.getReadStatus() == ReadStatus::Ok)
        {
            std::cout << "Read " << count << " samples" << std::endl;
        }
        else if (status.getReadStatus() == ReadStatus::Event)
        {
            std::cout << "Event occurred" << std::endl;
        }
        else
        {
            std::cout << "Error" << std::endl;
        }
    }

	std::cout << "Read " << readSamplesCount << " samples" << std::endl;

    return 0;
}
