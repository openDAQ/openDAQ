#include <opendaq/opendaq.h>
#include <iostream>
#include <mutex>
#include <thread>

using namespace daq;
using namespace std::chrono_literals;

// Reads equal-rate signals through input ports from the onDataAvailable callback and outputs their average.
// The reader is the only listener of the ports; a consumer reads from the callback or in a loop, never both.
void readDataSameRatesPortsAndOutput(const ListPtr<ISignal>& signals)
{
    const auto signalsCount = signals.getCount();
    const auto context = signals[0].getContext();

    auto ports = List<IComponent>();
    for (size_t i = 0; i < signalsCount; ++i)
        ports.pushBack(InputPort(context, nullptr, "port" + std::to_string(i)));

    auto params = MultiReader2Params();
    params.setInputs(ports);
    params.setValueReadType(SampleType::Float64);
    auto reader = MultiReader2(params);

    SignalConfigPtr outputSignal;
    SignalConfigPtr outputDomainSignal;

    auto bufferSize = SizeT{0};
    auto dataBuffers = std::vector<std::vector<double>>(signalsCount);
    auto buffers = std::vector<void*>(signalsCount, nullptr);

    std::mutex mutex;
    bool running = true;

    // Reads until nothing is deliverable and nothing changed: the samples first, the status after
    auto drain = [&]
    {
        std::scoped_lock lock(mutex);
        if (!running)
            return;

        for (;;)
        {
            auto count = std::min(bufferSize, reader.getAvailableCount());
            SizeT offset = 0;
            const auto status = reader.read(count > 0 ? buffers.data() : nullptr, &count, offset);

            if (count > 0 && outputSignal.assigned())
            {
                auto domainPacket = DataPacket(outputDomainSignal.getDescriptor(), count, offset);
                auto valuePacket = DataPacketWithDomain(domainPacket, outputSignal.getDescriptor(), count);
                auto avgData = static_cast<double*>(valuePacket.getRawData());
                for (size_t i = 0; i < count; ++i)
                {
                    avgData[i] = 0;
                    for (const auto& buffer : dataBuffers)
                        avgData[i] += buffer[i];
                    avgData[i] /= static_cast<double>(signalsCount);
                }
                outputDomainSignal.sendPacket(domainPacket);
                outputSignal.sendPacket(valuePacket);
            }

            if (status.getHasChanges())
            {
                if (status.getValid() && status.getDomainDescriptorChanged())
                {
                    // Buffers hold 100 ms per signal; the output follows the main domain
                    const auto domainDescriptor = status.getDomainDescriptor();
                    bufferSize = static_cast<SizeT>(reader::getSampleRate(domainDescriptor) / 10);
                    for (size_t i = 0; i < signalsCount; ++i)
                    {
                        dataBuffers[i].assign(bufferSize, 0.0);
                        buffers[i] = dataBuffers[i].data();
                    }

                    if (!outputSignal.assigned())
                    {
                        outputSignal = SignalWithDescriptor(context, DataDescriptorBuilder().setSampleType(SampleType::Float64).build(), nullptr, "Avg");
                        outputDomainSignal = SignalWithDescriptor(context, domainDescriptor, nullptr, "AvgTime");
                        outputSignal.setDomainSignal(outputDomainSignal);
                    }
                    else
                    {
                        outputDomainSignal.setDescriptor(domainDescriptor);
                    }
                }
            }
            else if (count == 0)
            {
                return;
            }
        }
    };

    reader.getOnDataAvailable() += [&](InputPortPtr&, EventArgsPtr<>&) { drain(); };

    for (size_t i = 0; i < signalsCount; ++i)
        ports[i].asPtr<IInputPort>().connect(signals[i]);

    // The output exists once the first status arrived
    for (int i = 0; i < 100 && !outputSignal.assigned(); i++)
        std::this_thread::sleep_for(10ms);

    // Read the average with a stream reader, 100 ms at a time
    auto streamReader = StreamReader<double, int64_t>(outputSignal);
    double avgValues[100];
    for (size_t readCount = 0; readCount < 20; readCount++)
    {
        auto count = std::min<SizeT>(100, streamReader.getAvailableCount());
        streamReader.read(&avgValues, &count);
        if (count > 0)
            std::cout << "Avg data: " << avgValues[0] << "\n";
        std::this_thread::sleep_for(50ms);
    }

    {
        std::scoped_lock lock(mutex);
        running = false;
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

    std::cout << "Same rate data, using input ports, read in callbacks, data is output:\n";
    readDataSameRatesPortsAndOutput(signals);

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
