#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>
#include <ref_fb_module/module_dll.h>
#include "testutils/memcheck_listener.h"
using namespace daq;

template <typename T>
using vecvec = std::vector<std::vector<T> >;

class StatisticsTest : public testing::Test
{
};

template <typename T>
class StatisticsTestHelper
{
public:
    StatisticsTestHelper(DataRulePtr rule,
                         vecvec<Float> expectedAvg,
                         vecvec<Float> expectedRms,
                         vecvec<Int> expectedDomain,
                         SampleType sampleType,
                         vecvec<T> mockPackets)
    {
        // Create logger, context and module
        auto logger = Logger();
        context = Context(Scheduler(logger), logger, nullptr, nullptr);
        createModule(&module, context);

        this->rule = rule;
        this->expectedAvg = expectedAvg;
        this->expectedRms = expectedRms;
        this->expectedDomain = expectedDomain;
        this->sampleType = sampleType;
        this->mockPackets = mockPackets;
    }

    void run()
    {
        createDomainSignal();
        createDomainPackets();
        createSignal();
        createFunctionBlock();
        sendPackets();
        receivePacketsAndCheckBoth();
    }

private:
    ContextPtr context;
    ModulePtr module;

    DataRulePtr rule;
    vecvec<Float> expectedAvg;
    vecvec<Float> expectedRms;
    vecvec<Int> expectedDomain;
    SampleType sampleType;
    vecvec<T> mockPackets;

    DataDescriptorPtr domainSignalDescriptor;
    SignalConfigPtr domainSignal;
    std::vector<DataPacketPtr> domainPackets;
    DataDescriptorPtr signalDescriptor;
    SignalConfigPtr signal;
    FunctionBlockPtr fb;  // TODO use for changing properties
    PacketReaderPtr readerAvg;
    PacketReaderPtr readerRms;

    void createDomainSignal()
    {
        // Create domain signal with descriptor
        auto domainSignalDescriptorBuilder = DataDescriptorBuilder();
        domainSignalDescriptorBuilder.setUnit(Unit("s", -1, "seconds", "Time"));
        domainSignalDescriptorBuilder.setSampleType(SampleType::Int64);
        domainSignalDescriptorBuilder.setRule(rule);
        domainSignalDescriptorBuilder.setOrigin("1970");
        domainSignalDescriptorBuilder.setTickResolution(Ratio(1, 1000));
        domainSignalDescriptor = domainSignalDescriptorBuilder.build();
        domainSignal = SignalWithDescriptor(context, domainSignalDescriptor, nullptr, "domain_signal");
    }

    void createDomainPackets()
    {
        // Linear creation of domain packets
        for (size_t i = 0; i < mockPackets.size(); i++)
        {
            auto offset = 0;
            for (size_t ii = 0; ii < i; ii++)
            {
                Int delta = rule.getParameters().get("delta");
                offset = offset + mockPackets[ii].size() * delta;
            }

            auto domainPacket = DataPacket(domainSignalDescriptor, mockPackets[i].size(), offset);
            domainPackets.push_back(domainPacket);
        }
    }

    void createSignal()
    {
        // Create signal with descriptor
        auto signalDescriptorBuilder = DataDescriptorBuilder();
        signalDescriptorBuilder.setSampleType(sampleType);
        signalDescriptorBuilder.setValueRange(Range(0, 300));
        signalDescriptorBuilder.setRule(ExplicitDataRule());
        signalDescriptor = signalDescriptorBuilder.build();
        signal = SignalWithDescriptor(context, signalDescriptor, nullptr, "signal");

        // Set domain signal of signal
        signal.setDomainSignal(domainSignal);
    }

    void createFunctionBlock()
    {
        // Create function block
        fb = module.createFunctionBlock("ref_fb_module_statistics", nullptr, "fb");

        // Set input (port) and outputs (signals) of the function block
        fb.getInputPorts()[0].connect(signal);
        readerAvg = PacketReader(fb.getSignals()[0]);
        readerRms = PacketReader(fb.getSignals()[1]);
    }

    void sendPackets()
    {
        // For each packet
        for (size_t i = 0; i < mockPackets.size(); i++)
        {
            // Create data packet
            auto dataPacket = DataPacketWithDomain(domainPackets[i], signalDescriptor, mockPackets[i].size());
            auto packetData = static_cast<T*>(dataPacket.getData());
            for (size_t ii = 0; ii < mockPackets[i].size(); ii++)
                *packetData++ = static_cast<T>(mockPackets[i][ii]);

            // Send packet
            domainSignal.sendPacket(domainPackets[i]);
            signal.sendPacket(dataPacket);
        }
    }

    void receivePacketsAndCheck(vecvec<Float> expected, PacketReaderPtr reader)
    {
        // For each input data packet
        for (size_t i = 0; i < mockPackets.size(); i++)
        {
            // Receive data packets that come from single input data packet
            std::vector<DataPacketPtr> receivedPacketVector;

            // Receive until you get all expected packets
            while (receivedPacketVector.size() < expected[i].size())
            {
                auto receivedPacket = reader.read();
                // Ignore nullptr and PacketType::Event
                if (receivedPacket != nullptr && receivedPacket.getType() == PacketType::Data)
                {
                    receivedPacketVector.push_back(receivedPacket);
                }
            }

            // Check packet(s) contents
            for (size_t ii = 0; ii < expected[i].size(); ii++)
            {
                auto data = static_cast<Float*>(receivedPacketVector[ii].getData());
                const size_t sampleCount = receivedPacketVector[ii].getSampleCount();
                auto dataSample = data[0];

                auto domainData = static_cast<Int*>(receivedPacketVector[ii].getDomainPacket().getData());
                const size_t domainSampleCount = receivedPacketVector[ii].getDomainPacket().getSampleCount();
                auto domainDataSample = domainData[0];

                // Assert that packet has one sample
                ASSERT_EQ(sampleCount, 1);
                // Assert that domain packet has one sample
                ASSERT_EQ(domainSampleCount, 1);

                // Assert that first sample equals expected value
                ASSERT_EQ(dataSample, expected[i][ii]);

                // Assert that first domain sample equals expected value
                ASSERT_EQ(domainDataSample, expectedDomain[i][ii]);
            }
        }
    }

    void receivePacketsAndCheckBoth()
    {
        receivePacketsAndCheck(expectedAvg, readerAvg);
        receivePacketsAndCheck(expectedRms, readerRms);
    }
};

TEST_F(StatisticsTest, StatisticsTestBasic)
{
    // TODO Test smaller, bigger, not size 10, etc.
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0}};
    vecvec<Float> expectedAvg{{0.55}, {1.55}, {2.55}};
    vecvec<Float> expectedRms{{0.62048368229954287}, {1.5763882770434448}, {2.5661254840712679}};
    vecvec<Int> expectedDomain{{3}, {23}, {43}};  // TODO Legit?

    auto helper = StatisticsTestHelper(
        LinearDataRule(2, 3), expectedAvg, expectedRms, expectedDomain, SampleTypeFromType<Float>().SampleType, mockPackets);
    helper.run();
}
