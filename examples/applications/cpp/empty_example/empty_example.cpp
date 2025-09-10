/**
 * Empty example
 */

#include <opendaq/opendaq.h>
#include <iostream>
#include <thread>

#include <chrono>
using namespace std::literals::chrono_literals;
using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules at MODULE_PATH
    const InstancePtr instance = Instance(MODULE_PATH);

    auto sig1Desc = DataDescriptorBuilder()
                        .setSampleType(SampleType::Int64)
                        .setOrigin("2000-01-01T00:00:00")
                        .setTickResolution(Ratio(1, 20'000'000))
                        .setRule(LinearDataRule(2, 0))
                        .setUnit(Unit("s", -1, "seconds", "time"))
                        .build();
    auto sig1Domain = SignalWithDescriptor(instance.getContext(), sig1Desc, nullptr, "sig1");

    auto sig2Desc = DataDescriptorBuilder()
                        .setSampleType(SampleType::Int64)
                        .setOrigin("2000-01-01T00:00:00")
                        .setTickResolution(Ratio(1, 10'000'000))
                        .setRule(LinearDataRule(1, 0))
                        .setUnit(Unit("s", -1, "seconds", "time"))
                        .build();
    auto sig2Domain = SignalWithDescriptor(instance.getContext(), sig2Desc, nullptr, "sig2");

    auto sig3Desc = DataDescriptorBuilder()
                        .setSampleType(SampleType::Int64)
                        .setOrigin("2000-01-01T00:00:00")
                        .setTickResolution(Ratio(1, 10'000'000))
                        .setRule(LinearDataRule(1, 0))
                        .setUnit(Unit("s", -1, "seconds", "time"))
                        .build();
    auto sig3Domain = SignalWithDescriptor(instance.getContext(), sig3Desc, nullptr, "sig3");

    auto signalDescriptorBuilder1 = DataDescriptorBuilder();
    signalDescriptorBuilder1.setSampleType(SampleType::Float32);
    signalDescriptorBuilder1.setValueRange(Range(0, 300));
    signalDescriptorBuilder1.setRule(ExplicitDataRule());
    auto signalDescriptor1 = signalDescriptorBuilder1.build();
    auto sig1 = SignalWithDescriptor(instance.getContext(), signalDescriptor1, nullptr, "signal1");
    sig1.setDomainSignal(sig1Domain);

    auto signalDescriptorBuilder2 = DataDescriptorBuilder();
    signalDescriptorBuilder2.setSampleType(SampleType::Float32);
    signalDescriptorBuilder2.setValueRange(Range(0, 300));
    signalDescriptorBuilder2.setRule(ExplicitDataRule());
    auto signalDescriptor2 = signalDescriptorBuilder2.build();
    auto sig2 = SignalWithDescriptor(instance.getContext(), signalDescriptor2, nullptr, "signal2");
    sig2.setDomainSignal(sig2Domain);

    auto signalDescriptorBuilder3 = DataDescriptorBuilder();
    signalDescriptorBuilder3.setSampleType(SampleType::Float32);
    signalDescriptorBuilder3.setValueRange(Range(0, 300));
    signalDescriptorBuilder3.setRule(ExplicitDataRule());
    auto signalDescriptor3 = signalDescriptorBuilder3.build();
    auto sig3 = SignalWithDescriptor(instance.getContext(), signalDescriptor3, nullptr, "signal3");
    sig3.setDomainSignal(sig3Domain);

    auto signals1 = List<ISignal>();
    signals1.pushBack(sig1);
    signals1.pushBack(sig2);
    signals1.pushBack(sig3);

    auto domainPacket1 = DataPacket(sig1Desc, 10'000'000, 0);
    auto domainPacket2 = DataPacket(sig2Desc, 10'000'000, 8'500'300);
    auto domainPacket3 = DataPacket(sig3Desc, 10'000'000, 37'007'025);

    auto packet1 = DataPacketWithDomain(domainPacket1, signalDescriptor1, 10'000'000);
    auto packet2 = DataPacketWithDomain(domainPacket2, signalDescriptor2, 10'000'000);
    auto packet3 = DataPacketWithDomain(domainPacket3, signalDescriptor3, 10'000'000);

    auto multiReader1 = MultiReaderEx(
        signals1, daq::SampleType::Undefined, daq::SampleType::Int64, ReadMode::Unscaled, daq::ReadTimeoutType::All, 10'000'000, False, 1);

    sig1.sendPacket(packet1);
    sig2.sendPacket(packet2);
    sig3.sendPacket(packet3);

    int count = 0;

    for (int i = 0; i < 20; ++i)
    {
        count++;

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

        daq::SizeT availableCount = multiReader1.getAvailableCount();
        std::cout << "MultiReaderImpl::isSynchronized: " << std::to_string(multiReader1.getIsSynchronized()) << std::endl;

        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        auto msecs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);

        std::cout << "MultiReaderImpl::getAvailableCount: " << availableCount << ", timed microseconds: " << msecs.count() << std::endl;
        if (availableCount == 0)
        {
            multiReader1.read(nullptr, &availableCount, 0);
        }
        else if (availableCount > 0)
        {
            std::cout << availableCount << "\n";
            multiReader1.skipSamples(&availableCount);
        }

        auto domainPacket4 = DataPacket(sig1Desc, 10'000'000, 20'000'000*count);
        auto domainPacket5 = DataPacket(sig2Desc, 10'000'000, 10'000'000*count+8'500'300);
        auto domainPacket6 = DataPacket(sig3Desc, 10'000'000, 10'000'000*count+7'007'025);

        auto packet4 = DataPacketWithDomain(domainPacket4, signalDescriptor1, 10'000'000);
        auto packet5 = DataPacketWithDomain(domainPacket5, signalDescriptor2, 10'000'000);
        auto packet6 = DataPacketWithDomain(domainPacket6, signalDescriptor3, 10'000'000);

        sig1.sendPacket(packet4);
        sig2.sendPacket(packet5);
        sig3.sendPacket(packet6);
    }


    //std::this_thread::sleep_for(25ms);
    //std::cout << "Available devices: " << std::endl;
    //for (auto device : instance.getAvailableDevices())
    //{
    //    std::cout << "Device: " << device.getName().toStdString() << std::endl;
    //    std::cout << "\tConn string: " << device.getConnectionString().toStdString() << std::endl;
    //}
    //// std::cin.get();
    //auto device = instance.addDevice("daqref://device0");
    //if (!device.assigned())
    //{
    //    std::cout << "Device not found!" << std::endl;
    //    std::cin.get();
    //    return 0;
    //}
    //auto signals = List<ISignal>();

    //auto deviceChannels = device.getChannels();

    //std::this_thread::sleep_for(25ms);
    //std::cout << "Number of channels: " << deviceChannels.getCount() << std::endl;
    //for (auto channel : deviceChannels)
    //{
    //    std::cout << "Channel: " << channel.getName().toStdString() << std::endl;
    //    auto chanSignals = channel.getSignals();

    //    std::cout << "\tNumber of signals: " << chanSignals.getCount() << std::endl;
    //    for (auto signal : chanSignals)
    //    {
    //        std::cout << "\tSignal: " << signal.getName().toStdString() << std::endl;
    //        signals.pushBack(signal);
    //    }
    //}
    //std::cin.get();
    //// fill signals array with synchronous channels (DataRuleType::Linear)
    //// Int deviceRate = device.getPropertyValue("SampleRate");
    //auto multiReader = MultiReaderEx(
    //    signals, daq::SampleType::Undefined, daq::SampleType::Int64, ReadMode::Unscaled, daq::ReadTimeoutType::All, 1500000, True, 1);

    //while (true)
    //{
    //    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    //    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    //    daq::SizeT availableCount = multiReader.getAvailableCount();

    //    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    //    auto msecs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);

    //    std::cout << "MultiReaderImpl::getAvailableCount: " << availableCount << ", timed microseconds: " << msecs.count() << std::endl;
    //    if (availableCount == 0)
    //    {
    //        multiReader.read(nullptr, &availableCount, 0);
    //    }
    //    else if (availableCount > 0)
    //    {
    //        std::cout << availableCount << "\n";
    //        multiReader.skipSamples(&availableCount);
    //    }
    //}

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
