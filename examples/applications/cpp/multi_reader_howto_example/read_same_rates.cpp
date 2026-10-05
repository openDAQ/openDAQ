#include <opendaq/opendaq.h>
#include <iostream>
#include <thread>

using namespace daq;
using namespace std::chrono_literals;

// Reads equal-rate signals with MultiReader2 in a loop. Every read returns one status: the data belongs to the
// descriptors reported on earlier statuses, the changes apply from the next read on, so the loop processes the
// samples first and handles the status after. A read never waits for data.
void readDataSameRatesSignals(const ListPtr<ISignal>& signals)
{
    auto inputs = List<IComponent>();
    for (const auto& signal : signals)
        inputs.pushBack(signal);

    auto params = MultiReader2Params();
    params.setInputs(inputs);
    params.setValueReadType(SampleType::Float64);  // the default; Invalidate is the default error policy
    auto reader = MultiReader2(params);

    // One buffer per signal, 100 ms worth of samples once the rate is known
    const auto signalsCount = signals.getCount();
    auto bufferSize = SizeT{0};
    auto dataBuffers = std::vector<std::vector<double>>(signalsCount);
    auto buffers = std::vector<void*>(signalsCount, nullptr);

    for (size_t readCount = 0; readCount < 20; readCount++)
    {
        auto count = std::min(bufferSize, reader.getAvailableCount());
        SizeT offset = 0;
        const auto status = reader.read(count > 0 ? buffers.data() : nullptr, &count, offset);

        if (count > 0)
        {
            std::cout << "Data at " << offset << ": ";
            for (const auto& buffer : dataBuffers)
                std::cout << buffer[0] << "; ";
            std::cout << "\n";
        }

        if (status.getHasChanges())
        {
            if (!status.getValid())
            {
                for (const MultiReader2InputStatusPtr input : status.getInputs())
                    if (input.getError() != MultiReader2InputError::None)
                        std::cout << "Waiting for " << input.getInput().getGlobalId() << "\n";
            }
            else if (status.getDomainDescriptorChanged())
            {
                // Size the buffers from the main domain: 100 ms per signal
                const auto sampleRate = reader::getSampleRate(status.getDomainDescriptor());
                bufferSize = static_cast<SizeT>(sampleRate / 10);
                for (size_t i = 0; i < signalsCount; ++i)
                {
                    dataBuffers[i].assign(bufferSize, 0.0);
                    buffers[i] = dataBuffers[i].data();
                }
            }
        }

        std::this_thread::sleep_for(50ms);
    }
}

int main()
{
    auto instance = Instance();
    auto refDevice = instance.addDevice("daqref://device0");
    refDevice.setPropertyValue("NumberOfChannels", 4);
    auto signals = List<ISignal>();
    for (const auto& signal : refDevice.getSignalsRecursive())
        if (signal.getDomainSignal().assigned())
            signals.pushBack(signal);

    std::cout << "Same rate data, signals, read in a loop:\n";
    readDataSameRatesSignals(signals);

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
