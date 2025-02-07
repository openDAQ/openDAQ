#include <opendaq/event_packet_params.h>
#include <opendaq/opendaq.h>
#include <iostream>

using namespace daq;
using namespace std::chrono_literals;

void readDataSameRatesSignals(const ListPtr<ISignal>& signals)
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

    // read data every 50ms, up to a maximum of kBufferSize samples
    for (size_t readCount = 0; readCount < 20; readCount++)
    {
        auto dataAvailable = multiReader.getAvailableCount();
        auto count = std::min(kBufferSize, dataAvailable);
        auto status = multiReader.readWithDomain(dataBuffers.data(), domainBuffers.data(), &count);

        if (status.getReadStatus() == ReadStatus::Event)
        {
            // Set buffer size based on sample rate, allocate buffers
            // Buffers have 100ms worth of memory for each signal
            auto sampleRate = reader::getSampleRate(
                status.getMainDescriptor().getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR));
            kBufferSize = sampleRate / 10;

            for (size_t i = 0; i < signalsCount; ++i)
            {
                dataBuffers[i] = std::calloc(kBufferSize, getSampleSize(SampleType::Float64));
                domainBuffers[i] = std::calloc(kBufferSize, getSampleSize(SampleType::Int64));
            }
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

    std::cout << "Same rate data, signals, read in a loop:\n";
    readDataSameRatesSignals(signals);
    
    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
