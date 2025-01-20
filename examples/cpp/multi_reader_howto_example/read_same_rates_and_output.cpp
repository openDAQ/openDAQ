#include <opendaq/event_packet_params.h>
#include <opendaq/opendaq.h>
#include <iostream>

using namespace daq;
using namespace std::chrono_literals;

void readDataSameRatesPortsAndOutput(const ListPtr<ISignal>& signals)
{
    ListPtr<IInputPort> ports = List<IInputPort>();
    auto signalsCount = signals.getCount();
    auto context = signals[0].getContext();
    for (size_t i = 0; i < signalsCount; ++i)
    {
        const auto port = InputPort(context, nullptr, "port" + std::to_string(i));
        port.setNotificationMethod(PacketReadyNotification::SameThread);
        ports.pushBack(port);
    }

    // Create reader that converts values to `double` and time data to `int64`
    auto multiReaderBuilder = MultiReaderBuilder().setValueReadType(SampleType::Float64).setDomainReadType(SampleType::Int64);
    for (const auto& port : ports)
        multiReaderBuilder.addInputPort(port);
    auto multiReader = multiReaderBuilder.build();

    SignalConfigPtr outputSignal;
    SignalConfigPtr outputDomainSignal;

    // Allocate buffers for each signal
    auto kBufferSize = SizeT{0};
    auto domainBuffers = std::vector<void*>(signalsCount, nullptr);
    auto dataBuffers = std::vector<void*>(signalsCount, nullptr);

    std::mutex mutex;
    bool running = true;

    // read data a maximum of kBufferSize samples
    auto readData = [&]
    {
        std::scoped_lock lock(mutex);
        if (!running)
            return;

        auto dataAvailable = multiReader.getAvailableCount();
        auto count = std::min(kBufferSize, dataAvailable);
        auto status = multiReader.readWithDomain(dataBuffers.data(), domainBuffers.data(), &count);

        if (status.getReadStatus() == ReadStatus::Event)
        {
            // Set buffer size based on sample rate, allocate buffers
            // Buffers have 100ms worth of memory for each signal
            auto domainDataDescriptor = status.getMainDescriptor().getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
            auto sampleRate = reader::getSampleRate(domainDataDescriptor);
            kBufferSize = sampleRate / 10;

            for (size_t i = 0; i < signalsCount; ++i)
            {
                dataBuffers[i] = std::calloc(kBufferSize, getSampleSize(SampleType::Float64));
                domainBuffers[i] = std::calloc(kBufferSize, getSampleSize(SampleType::Int64));
            }

            // Configure output signals
            outputSignal = SignalWithDescriptor(context, DataDescriptorBuilder().setSampleType(SampleType::Float64).build(), nullptr, "Avg");
            outputDomainSignal = SignalWithDescriptor(context, domainDataDescriptor, nullptr, "AvgTime");
            outputSignal.setDomainSignal(outputDomainSignal);
        }
        else if (status.getReadStatus() == ReadStatus::Ok && count > 0)
        {
            auto domainPacket = DataPacket(outputDomainSignal.getDescriptor(), count, status.getOffset());
            auto valuePacket = DataPacketWithDomain(domainPacket, outputSignal.getDescriptor(), count);

            // Average all signals and send output
            double* avgData = static_cast<double*>(valuePacket.getRawData());
            for (size_t i = 0; i < count; ++i)
            {
                avgData[i] = 0;
                for (const auto& buf : dataBuffers)
                    avgData[i] += static_cast<double*>(buf)[i];

                avgData[i] /= static_cast<double>(signalsCount);
            }

            outputSignal.sendPacket(valuePacket);
            outputDomainSignal.sendPacket(domainPacket);
        }
    };
    
    // Set read callback
    multiReader.setOnDataAvailable(readData);
    

    // Connect signals to ports
    for (size_t i = 0; i < signalsCount; ++i)
        ports[i].connect(signals[i]);
    
    // Read avg data with stream reader, pre-allocate 100ms of data, assuming 1KHz rates
    auto streamReader = StreamReader<double, int64_t>(outputSignal);
    double avgValues[100];

    for (size_t readCount = 0; readCount < 20; readCount++)
    {
        auto count = streamReader.getAvailableCount();
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
    auto signals = refDevice.getSignalsRecursive();

    std::cout << "Same rate data, using input ports, read in callbacks, data is output:\n";
    readDataSameRatesPortsAndOutput(signals);
}
