#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>
#include <ref_fb_module/module_dll.h>
#include "testutils/memcheck_listener.h"
using namespace daq;

template <typename T>
using vecvec = std::vector<std::vector<T> >;

class TriggerTest : public testing::Test
{
};

template <typename T>
class TriggerTestHelper
{
public:
    TriggerTestHelper(DataRulePtr rule,
                      vecvec<Bool> expectedData,
                      vecvec<Int> expectedDomain,
                      SampleType sampleType,
                      vecvec<T> mockPackets,
                      std::vector<Int> thresholdChangesAfterPackets,
                      vecvec<Int> mockDomainPackets = {},
                      std::vector<Float> newThresholds = {})
    {
        // Create logger, context and module
        auto logger = Logger();
        context = Context(Scheduler(logger), logger, nullptr, nullptr);
        createModule(&module, context);

        this->rule = rule;
        this->expectedData = expectedData;
        this->expectedDomain = expectedDomain;
        this->sampleType = sampleType;
        this->mockPackets = mockPackets;
        this->thresholdChangesAfterPackets = thresholdChangesAfterPackets;
        this->mockDomainPackets = mockDomainPackets;
        this->newThresholds = newThresholds;
    }

    void run()
    {
        createDomainSignal();
        createDomainPackets();
        createSignal();
        createFunctionBlock();
        sendPacketsAndChangeThreshold();
        receivePacketsAndCheck();

        // TODO Temporay fix so PacketReadyNotification::Scheduler works
        context.getScheduler().stop();
    }

private:
    ContextPtr context;
    ModulePtr module;

    DataRulePtr rule;
    vecvec<Bool> expectedData;
    vecvec<Int> expectedDomain;
    SampleType sampleType;
    vecvec<T> mockPackets;
    std::vector<Int> thresholdChangesAfterPackets;
    vecvec<Int> mockDomainPackets;
    std::vector<Float> newThresholds;

    DataDescriptorPtr domainSignalDescriptor;
    SignalConfigPtr domainSignal;
    std::vector<DataPacketPtr> domainPackets;
    DataDescriptorPtr signalDescriptor;
    SignalConfigPtr signal;
    FunctionBlockPtr fb;
    PacketReaderPtr reader;

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
        // Create domain packets
        if (rule.getType() == DataRuleType::Explicit)
        {
            for (size_t i = 0; i < mockPackets.size(); i++)
            {
                // Explicit creation of one domain packet
                auto domainPacket = DataPacket(domainSignalDescriptor, mockPackets[i].size());
                auto domainPacketData = static_cast<Int*>(domainPacket.getData());
                for (size_t ii = 0; ii < mockDomainPackets[i].size(); ii++)
                    *domainPacketData++ = static_cast<Int>(mockDomainPackets[i][ii]);
                domainPackets.push_back(domainPacket);
            }
        }
        else
        {
            for (size_t i = 0; i < mockPackets.size(); i++)
            {
                // Linear creation of one domain packet
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
        fb = module.createFunctionBlock("ref_fb_module_trigger", nullptr, "fb");

        // Set input (port) and output (signal) of the function block
        fb.getInputPorts()[0].connect(signal);
        reader = PacketReader(fb.getSignals()[0]);
    }

    void sendPacketsAndChangeThreshold()
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

            // Check if we should change the threshold after sending packet
            auto foundAt = std::find(thresholdChangesAfterPackets.begin(), thresholdChangesAfterPackets.end(), i);
            if (foundAt != thresholdChangesAfterPackets.end())
            {
                // Change the threshold if appropriate
                fb.setPropertyValue("Threshold", newThresholds[foundAt - thresholdChangesAfterPackets.begin()]);
            }
        }
    }

    void receivePacketsAndCheck()
    {
        // For each input data packet
        for (size_t i = 0; i < mockPackets.size(); i++)
        {
            // Receive data packets that come from single input data packet
            std::vector<DataPacketPtr> receivedPacketVector;
            // Receive until you get all expected packets
            while (receivedPacketVector.size() < expectedData[i].size())
            {
                auto receivedPacket = reader.read();
                // Ignore nullptr and PacketType::Event
                if (receivedPacket != nullptr && receivedPacket.getType() == PacketType::Data)
                {
                    receivedPacketVector.push_back(receivedPacket);
                }
            }

            // Check packet(s) contents
            for (size_t ii = 0; ii < expectedData[i].size(); ii++)
            {
                auto data = static_cast<Bool*>(receivedPacketVector[ii].getData());
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
                ASSERT_EQ(dataSample, expectedData[i][ii]);
                // Assert that first domain sample equals expected value
                ASSERT_EQ(domainDataSample, expectedDomain[i][ii]);
            }
        }
    }
};

TEST_F(TriggerTest, TriggerTestFloatExplicit)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 2, 3, 4, 4.5, 0.2, 0.1, 0, 5, 6, 7}, {6, 0.1, 0, 6, 7}, {7, 8, 0.1}, {0.1, 0.2, 0.6, 0.8}};
    vecvec<Int> mockDomainPackets{{3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27}, {29, 31, 33, 35, 37}, {39, 41, 43}, {45, 47, 49, 51}};
    vecvec<Bool> expectedData{{true, false, true}, {false, true}, {false}, {true}};
    vecvec<Int> expectedDomain{{9, 17, 23}, {31, 35}, {43}, {49}};

    auto helper = TriggerTestHelper(
        ExplicitDataRule(), expectedData, expectedDomain, SampleTypeFromType<Float>().SampleType, mockPackets, {}, mockDomainPackets);
    helper.run();
}

TEST_F(TriggerTest, TriggerTestFloatLinear)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 2, 3, 4, 4.5, 0.2, 0.1, 0, 5, 6, 7}, {6, 0.1, 0, 6, 7}, {7, 8, 0.1}, {0.1, 0.2, 0.6, 0.8}};
    vecvec<Bool> expectedData{{true, false, true}, {false, true}, {false}, {true}};
    vecvec<Int> expectedDomain{{9, 17, 23}, {31, 35}, {43}, {49}};

    auto helper =
        TriggerTestHelper(LinearDataRule(2, 3), expectedData, expectedDomain, SampleTypeFromType<Float>().SampleType, mockPackets, {});
    helper.run();
}

TEST_F(TriggerTest, TriggerTestFloatExplicitThresholdChanged)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 2, 3, 4, 4.5, 0.2, 0.1, 0, 5, 6, 7}, {6, 0.1, 0, 6, 7}, {7, 8, 0.1}, {0.1, 0.2, 0.6, 0.8}};
    vecvec<Int> mockDomainPackets{{3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27}, {29, 31, 33, 35, 37}, {39, 41, 43}, {45, 47, 49, 51}};
    vecvec<Bool> expectedData{{true, false, true}, {false, true}, {false}, {true}};
    vecvec<Int> expectedDomain{{9, 17, 23}, {31, 35}, {43}, {45}};
    std::vector<Int> thresholdChangesAfterPackets{2};
    std::vector<Float> newThresholds{0.1};

    auto helper = TriggerTestHelper(ExplicitDataRule(),
                                    expectedData,
                                    expectedDomain,
                                    SampleTypeFromType<Float>().SampleType,
                                    mockPackets,
                                    thresholdChangesAfterPackets,
                                    mockDomainPackets,
                                    newThresholds);
    helper.run();
}

TEST_F(TriggerTest, TriggerTestFloatLinearThresholdChanged)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 2, 3, 4, 4.5, 0.2, 0.1, 0, 5, 6, 7}, {6, 0.1, 0, 6, 7}, {7, 8, 0.1}, {0.1, 0.2, 0.6, 0.8}};
    vecvec<Bool> expectedData{{true, false, true}, {false, true}, {false}, {true}};
    vecvec<Int> expectedDomain{{9, 17, 23}, {31, 35}, {43}, {45}};
    std::vector<Int> thresholdChangesAfterPackets{2};
    std::vector<Float> newThresholds{0.1};

    auto helper = TriggerTestHelper(LinearDataRule(2, 3),
                                    expectedData,
                                    expectedDomain,
                                    SampleTypeFromType<Float>().SampleType,
                                    mockPackets,
                                    thresholdChangesAfterPackets,
                                    {},
                                    newThresholds);
    helper.run();
}

TEST_F(TriggerTest, TriggerTestIntExplicit)
{
    vecvec<Int> mockPackets{{0, 0, 0, 289, 289, 289, 289, 0, 0, 0, 289, 289, 289}, {289, 0, 0, 289, 289}, {289, 289, 0}, {0, 0, 289, 289}};
    vecvec<Int> mockDomainPackets{{3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27}, {29, 31, 33, 35, 37}, {39, 41, 43}, {45, 47, 49, 51}};
    vecvec<Bool> expectedData{{true, false, true}, {false, true}, {false}, {true}};
    vecvec<Int> expectedDomain{{9, 17, 23}, {31, 35}, {43}, {49}};

    auto helper = TriggerTestHelper(
        ExplicitDataRule(), expectedData, expectedDomain, SampleTypeFromType<Int>().SampleType, mockPackets, {}, mockDomainPackets);
    helper.run();
}

TEST_F(TriggerTest, TriggerTestIntExplicitThresholdChanged)
{
    vecvec<Int> mockPackets{{0, 0, 0, 289, 289, 289, 289, 0, 0, 0, 289, 289, 289}, {289, 0, 0, 289, 289}, {289, 289, 0}, {0, 0, 289, 289}};
    vecvec<Int> mockDomainPackets{{3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27}, {29, 31, 33, 35, 37}, {39, 41, 43}, {45, 47, 49, 51}};
    vecvec<Bool> expectedData{{true, false, true}, {false, true}, {false}, {true}};
    vecvec<Int> expectedDomain{{9, 17, 23}, {31, 35}, {43}, {49}};
    std::vector<Int> thresholdChangesAfterPackets{2};
    std::vector<Float> newThresholds{200};

    auto helper = TriggerTestHelper(ExplicitDataRule(),
                                    expectedData,
                                    expectedDomain,
                                    SampleTypeFromType<Int>().SampleType,
                                    mockPackets,
                                    thresholdChangesAfterPackets,
                                    mockDomainPackets,
                                    newThresholds);
    helper.run();
}

TEST_F(TriggerTest, TriggerTestIntLinear)
{
    vecvec<Int> mockPackets{{0, 0, 0, 289, 289, 289, 289, 0, 0, 0, 289, 289, 289}, {289, 0, 0, 289, 289}, {289, 289, 0}, {0, 0, 289, 289}};
    vecvec<Bool> expectedData{{true, false, true}, {false, true}, {false}, {true}};
    vecvec<Int> expectedDomain{{9, 17, 23}, {31, 35}, {43}, {49}};

    auto helper =
        TriggerTestHelper(LinearDataRule(2, 3), expectedData, expectedDomain, SampleTypeFromType<Int>().SampleType, mockPackets, {});
    helper.run();
}

TEST_F(TriggerTest, TriggerTestIntLinearThresholdChanged)
{
    vecvec<Int> mockPackets{
        {0, 0, 0, 289, 289, 289, 289, 0, 0, 0, 289, 289, 289}, {289, 0, 0, 289, 289}, {289, 289, 0}, {199, 201, 199, 289}};
    vecvec<Bool> expectedData{{true, false, true}, {false, true}, {false}, {true, false}};
    vecvec<Int> expectedDomain{{9, 17, 23}, {31, 35}, {43}, {47, 49}};
    std::vector<Int> thresholdChangesAfterPackets{2};
    std::vector<Float> newThresholds{200};

    auto helper = TriggerTestHelper(LinearDataRule(2, 3),
                                    expectedData,
                                    expectedDomain,
                                    SampleTypeFromType<Int>().SampleType,
                                    mockPackets,
                                    thresholdChangesAfterPackets,
                                    {},
                                    newThresholds);
    helper.run();
}
