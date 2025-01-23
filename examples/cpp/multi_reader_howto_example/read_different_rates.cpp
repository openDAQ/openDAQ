#include <opendaq/event_packet_params.h>
#include <opendaq/opendaq.h>
#include <iostream>

using namespace daq;
using namespace std::chrono_literals;

void readDataDifferentRates(const ListPtr<ISignal>& signals)
{
    // Create reader that converts values to `double` and time data to `int64`
    auto multiReaderBuilder = MultiReaderBuilder().setValueReadType(SampleType::Float64).setDomainReadType(SampleType::Int64);
    for (const auto& signal : signals)
        multiReaderBuilder.addSignal(signal);
    auto multiReader = multiReaderBuilder.build();

    // Allocate buffers for each signal
    auto signalsCount = signals.getCount();
    auto kBufferSize = SizeT{0};
    auto domainBuffers = std::vector<void*>(signalsCount, nullptr);
    auto dataBuffers = std::vector<void*>(signalsCount, nullptr);
    std::vector<size_t> dividers;

    // read data every 50ms, up to a maximum of kBufferSize samples
    for (size_t readCount = 0; readCount < 20; readCount++)
    {
        auto dataAvailable = multiReader.getAvailableCount();
        auto count = std::min(kBufferSize, dataAvailable);
        auto status = multiReader.readWithDomain(dataBuffers.data(), domainBuffers.data(), &count);

        if (status.getReadStatus() == ReadStatus::Event)
        {
            auto packets = status.getEventPackets();
            if (!(packets.getValueList()[0].getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED))
                continue;

            // SRDiv calculation
            size_t commonSampleRate = multiReader.getCommonSampleRate();
            dividers.clear();
            std::cout << "Dividers: ";
            for (const auto& [_, eventPacket] : status.getEventPackets())
            {
                auto descriptor = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
                auto sampleRate = reader::getSampleRate(descriptor);
                dividers.push_back(commonSampleRate / sampleRate);
                std::cout << dividers.back() << ", ";
            }
            std::cout << "\n";

            // Allocate buffers for 100ms according to commonSampleRate
            size_t lcm = 1;
            for (const auto& div : dividers)
                lcm = std::lcm<std::size_t>(lcm, div);

            // Calculate k as the minimum number of LCM-size blocks to read ~100ms of data
            size_t k = std::max(commonSampleRate / lcm / 10, static_cast<size_t>(1));
            kBufferSize = k * lcm;
            
            std::cout << "Buffer sizes: ";
            for (size_t i = 0; i < signalsCount; ++i)
            {
                dataBuffers[i] = std::calloc(kBufferSize / dividers[i], getSampleSize(SampleType::Float64));
                domainBuffers[i] = std::calloc(kBufferSize / dividers[i], getSampleSize(SampleType::Int64));
                std::cout << kBufferSize / dividers[i] << ", ";
            }
            std::cout << "\n";
        }
        else if (status.getReadStatus() == ReadStatus::Ok && count > 0)
        {
            std::cout << "Data: ";
            for (const auto& buf : dataBuffers)
                std::cout << std::to_string(static_cast<double*>(buf)[0]) << "; ";
            std::cout << "\n";
        }

        std::this_thread::sleep_for(50ms);
    }

    for (size_t i = 0; i < signalsCount; ++i)
    {
        free(dataBuffers[i]);
        free(domainBuffers[i]);
    }
}

int main()
{
    auto instance = Instance();
    auto refDevice = instance.addDevice("daqref://device0");
    refDevice.setPropertyValue("NumberOfChannels", 4);
    auto signals = refDevice.getSignalsRecursive();
	
    const auto channels = refDevice.getChannelsRecursive();
    channels[0].setPropertyValue("UseGlobalSampleRate", false);
    channels[0].setPropertyValue("SampleRate", 100);
    channels[1].setPropertyValue("UseGlobalSampleRate", false);
    channels[1].setPropertyValue("SampleRate", 200);
    channels[2].setPropertyValue("UseGlobalSampleRate", false);
    channels[2].setPropertyValue("SampleRate", 500);
	
    std::cout << "Different rate data:\n";
    readDataDifferentRates(signals);

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
