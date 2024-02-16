#include <opendaq/context_internal_ptr.h>
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

struct RangeData
{
    Int low;
    Int high;
};

template <typename T>
class StatisticsTestHelper
{
public:
    StatisticsTestHelper(DataRulePtr rule,
                         Int initialOutputDomain,
                         vecvec<Float> expectedAvg,
                         vecvec<Float> expectedRms,
                         vecvec<Int> expectedDomain,
                         SampleType sampleType,
                         vecvec<T> mockPackets,
                         vecvec<Int> expectedDomainEnd = {},
                         std::vector<Int> blockSizeChangesAfterPackets = {},
                         std::vector<size_t> newBlockSizes = {},
                         std::vector<Int> outputDomainChangesAfterPackets = {},
                         std::vector<Int> newOutputDomain = {},
                         std::vector<Int> triggerModeChangesAfterPackets = {},
                         std::vector<Bool> newTriggerMode = {},
                         vecvec<Float> mockTriggerPackets = {},
                         vecvec<Float> mockTriggerDomainPackets = {})
    {
        // Create logger, module manager, context and module
        const auto logger = Logger();
        moduleManager = ModuleManager("[[none]]");
        context = Context(Scheduler(logger), logger, nullptr, moduleManager);
        createModule(&module, context);
        moduleManager.addModule(module);
        moduleManager = context.asPtr<IContextInternal>().moveModuleManager();

        this->rule = rule;
        this->initialOutputDomain = initialOutputDomain;
        this->expectedAvg = expectedAvg;
        this->expectedRms = expectedRms;
        this->expectedDomain = expectedDomain;
        this->sampleType = sampleType;
        this->mockPackets = mockPackets;
        this->expectedDomainEnd = expectedDomainEnd;
        this->blockSizeChangesAfterPackets = blockSizeChangesAfterPackets;
        this->newBlockSizes = newBlockSizes;
        this->outputDomainChangesAfterPackets = outputDomainChangesAfterPackets;
        this->newOutputDomain = newOutputDomain;
        this->triggerModeChangesAfterPackets = triggerModeChangesAfterPackets;
        this->newTriggerMode = newTriggerMode;
        this->mockTriggerPackets = mockTriggerPackets;
        this->mockTriggerDomainPackets = mockTriggerDomainPackets;
    }

    void run()
    {
        createDomainSignals();
        createDomainPackets();
        createSignals();
        createFunctionBlock();
        sendPacketsAndChangeProperties();
        receivePacketsAndCheckBoth();

        // TODO Temporary fix so PacketReadyNotification::Scheduler works
        context.getScheduler().stop();
    }

private:
    ContextPtr context;
    ModulePtr module;

    DataRulePtr rule;
    Int initialOutputDomain;
    vecvec<Float> expectedAvg;
    vecvec<Float> expectedRms;
    vecvec<Int> expectedDomain;
    SampleType sampleType;
    vecvec<T> mockPackets;
    vecvec<Int> expectedDomainEnd;
    std::vector<Int> blockSizeChangesAfterPackets;
    std::vector<size_t> newBlockSizes;
    std::vector<Int> outputDomainChangesAfterPackets;
    std::vector<Int> newOutputDomain;
    std::vector<Int> triggerModeChangesAfterPackets;
    std::vector<Bool> newTriggerMode;
    vecvec<Float> mockTriggerPackets;
    vecvec<Float> mockTriggerDomainPackets;
    ModuleManagerPtr moduleManager;

    DataDescriptorPtr domainSignalDescriptor;
    DataDescriptorPtr triggerDomainSignalDescriptor;
    SignalConfigPtr domainSignal;
    SignalConfigPtr triggerDomainSignal;
    std::vector<DataPacketPtr> domainPackets;
    std::vector<DataPacketPtr> triggerDomainPackets;
    DataDescriptorPtr signalDescriptor;
    DataDescriptorPtr triggerSignalDescriptor;
    SignalConfigPtr signal;
    SignalConfigPtr triggerSignal;
    FunctionBlockPtr fb;
    PacketReaderPtr readerAvg;
    PacketReaderPtr readerRms;

    void createDomainSignals()
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

        // Similar, but for trigger
        auto triggerDomainSignalDescriptorBuilder = DataDescriptorBuilder();
        triggerDomainSignalDescriptorBuilder.setUnit(Unit("s", -1, "seconds", "Time"));
        triggerDomainSignalDescriptorBuilder.setSampleType(SampleType::Int64);
        triggerDomainSignalDescriptorBuilder.setRule(ExplicitDataRule());  // Always Explicit
        triggerDomainSignalDescriptorBuilder.setOrigin("1970");
        triggerDomainSignalDescriptorBuilder.setTickResolution(Ratio(1, 1000));
        triggerDomainSignalDescriptor = triggerDomainSignalDescriptorBuilder.build();
        triggerDomainSignal = SignalWithDescriptor(context, triggerDomainSignalDescriptor, nullptr, "trigger_domain_signal");
    }

    void createDomainPackets()
    {
        // Linear creation of domain packets
        Int delta = rule.getParameters().get("delta");
        for (size_t i = 0; i < mockPackets.size(); i++)
        {
            auto offset = 0;
            for (size_t ii = 0; ii < i; ii++)
            {
                offset += mockPackets[ii].size() * delta;
            }

            auto domainPacket = DataPacket(domainSignalDescriptor, mockPackets[i].size(), offset);
            domainPackets.push_back(domainPacket);
        }

        // For trigger, it's Explicit
        for (size_t i = 0; i < mockTriggerPackets.size(); i++)
        {
            auto domainPacket = DataPacket(triggerDomainSignalDescriptor, mockTriggerPackets[i].size());
            auto domainPacketData = static_cast<Int*>(domainPacket.getData());
            for (size_t ii = 0; ii < mockTriggerDomainPackets[i].size(); ii++)
                *domainPacketData++ = static_cast<Int>(mockTriggerDomainPackets[i][ii]);
            triggerDomainPackets.push_back(domainPacket);
        }
    }

    void createSignals()
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

        // Create signal with descriptor for trigger
        auto triggerSignalDescriptorBuilder = DataDescriptorBuilder();
        triggerSignalDescriptorBuilder.setSampleType(SampleTypeFromType<Int>::SampleType);
        triggerSignalDescriptorBuilder.setValueRange(Range(0, 300));
        triggerSignalDescriptorBuilder.setRule(ExplicitDataRule());
        triggerSignalDescriptor = triggerSignalDescriptorBuilder.build();
        triggerSignal = SignalWithDescriptor(context, triggerSignalDescriptor, nullptr, "trigger_signal");

        // Set domain signal of signal for trigger
        triggerSignal.setDomainSignal(triggerDomainSignal);
    }

    void createFunctionBlock()
    {
        // TODO Temporary fix
        PropertyObjectPtr config = module.getAvailableFunctionBlockTypes().get("ref_fb_module_statistics").createDefaultConfig();
        config.setPropertyValue("UseMultiThreadedScheduler", false);

        // Create function block
        fb = module.createFunctionBlock("ref_fb_module_statistics", nullptr, "fb", config);

        // Set input (port) and outputs (signals) of the function block
        fb.getInputPorts()[0].connect(signal);
        readerAvg = PacketReader(fb.getSignals()[0]);
        readerRms = PacketReader(fb.getSignals()[1]);

        fb.setPropertyValue("DomainSignalType", initialOutputDomain);
    }

    void sendPacketsAndChangeProperties()
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

            if (mockTriggerPackets.size() > 0 && mockTriggerPackets[i].size() > 0)
            {
                // Create data packet for trigger
                auto triggerDataPacket =
                    DataPacketWithDomain(triggerDomainPackets[i], triggerSignalDescriptor, mockTriggerPackets[i].size());
                auto triggerPacketData = static_cast<Float*>(triggerDataPacket.getData());  // TODO use templated type
                for (size_t ii = 0; ii < mockTriggerPackets[i].size(); ii++)
                    *triggerPacketData++ = static_cast<Float>(mockTriggerPackets[i][ii]);

                auto fbs = fb.getFunctionBlocks();
                if (fbs.getCount() > 0)  // TODO fix condition
                {
                    // Nested trigger connect signal
                    fbs[0].getInputPorts()[0].connect(triggerSignal);

                    // Send packet for trigger
                    triggerDomainSignal.sendPacket(triggerDomainPackets[i]);
                    triggerSignal.sendPacket(triggerDataPacket);
                }
            }

            // Check if we should change block size after sending packet
            auto blockSizeChangeFoundAt = std::find(blockSizeChangesAfterPackets.begin(), blockSizeChangesAfterPackets.end(), static_cast<Int>(i));
            if (blockSizeChangeFoundAt != blockSizeChangesAfterPackets.end())
            {
                // Change block size if appropriate
                fb.setPropertyValue("BlockSize", newBlockSizes[blockSizeChangeFoundAt - blockSizeChangesAfterPackets.begin()]);
            }

            // Check if we should change domain signal type after sending packet
            auto domainSignalChangeFoundAt = std::find(outputDomainChangesAfterPackets.begin(), outputDomainChangesAfterPackets.end(), static_cast<Int>(i));
            if (domainSignalChangeFoundAt != outputDomainChangesAfterPackets.end())
            {
                // Change domain signal type if appropriate
                fb.setPropertyValue("DomainSignalType",
                                    newOutputDomain[domainSignalChangeFoundAt - outputDomainChangesAfterPackets.begin()]);
            }

            // Check if we should change trigger mode after sending packet
            auto triggerModeChangeFoundAt = std::find(triggerModeChangesAfterPackets.begin(), triggerModeChangesAfterPackets.end(), i);
            if (triggerModeChangeFoundAt != triggerModeChangesAfterPackets.end())
            {
                // Change trigger mode if appropriate
                fb.setPropertyValue("TriggerMode", newTriggerMode[triggerModeChangeFoundAt - triggerModeChangesAfterPackets.begin()]);
            }
        }
    }

    void receivePacketsAndCheck(vecvec<Float> expectedData, PacketReaderPtr reader)
    {
        for (size_t i = 0; i < mockPackets.size(); i++)
        {
            // Check if you expect a packet
            if (expectedData[i].size() != 0)
            {
                // Receive data packet
                PacketPtr receivedPacket;

                // Receive until you get one expected packet
                while (true)
                {
                    receivedPacket = reader.read();
                    // Ignore nullptr and PacketType::Event
                    if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
                    {
                        break;
                    }
                }

                // Check packet contents
                auto dataPacket = static_cast<DataPacketPtr>(receivedPacket);
                auto data = static_cast<Float*>(dataPacket.getData());
                const size_t sampleCount = dataPacket.getSampleCount();

                for (size_t ii = 0; ii < expectedData[i].size(); ii++)
                {
                    auto dataSample = data[ii];

                    // Assert that packet has expected number of samples
                    ASSERT_EQ(sampleCount, expectedData[i].size());
                    // Assert that data sample equals expected value
                    ASSERT_DOUBLE_EQ(dataSample, expectedData[i][ii]);

                    auto domainPacket = dataPacket.getDomainPacket();
                    auto domainDataDescroptor = domainPacket.getDataDescriptor();

                    if (domainDataDescroptor.getRule().getType() == DataRuleType::Explicit &&
                        domainDataDescroptor.getSampleType() == SampleType::RangeInt64)
                    {
                        // In case of ExplicitRange
                        auto domainData = static_cast<RangeData*>(domainPacket.getData());
                        const size_t domainSampleCount = domainPacket.getSampleCount();

                        // Assert that domain packet has expected number of samples
                        ASSERT_EQ(domainSampleCount, expectedDomain[i].size());

                        // Assert that domain sample start equals expected value
                        ASSERT_EQ(domainData->low, expectedDomain[i][ii]);

                        // Assert that domain sample end equals expected value
                        ASSERT_EQ(domainData->high, expectedDomainEnd[i][ii]);
                    }
                    else
                    {
                        // In case of Implicit or Explicit
                        auto domainData = static_cast<Int*>(domainPacket.getData());
                        const size_t domainSampleCount = domainPacket.getSampleCount();
                        auto domainDataSample = domainData[ii];

                        // Assert that domain packet has expected number of samples
                        ASSERT_EQ(domainSampleCount, expectedDomain[i].size());

                        // Assert that domain sample equals expected value
                        ASSERT_EQ(domainDataSample, expectedDomain[i][ii]);
                    }
                }
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
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0}};
    vecvec<Float> expectedAvg{{0.55}, {1.55}, {2.55}};
    vecvec<Float> expectedRms{{0.62048368229954287}, {1.5763882770434448}, {2.5661254840712679}};
    vecvec<Int> expectedDomain{{3}, {23}, {43}};
    Int initialOutputDomain = 0;  // Implicit

    auto helper = StatisticsTestHelper(LinearDataRule(2, 3),
                                       initialOutputDomain,
                                       expectedAvg,
                                       expectedRms,
                                       expectedDomain,
                                       SampleTypeFromType<Float>::SampleType,
                                       mockPackets);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestSmallerPackets)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3},
                              {0.4, 0.5, 0.6},
                              {0.7, 0.8, 0.9},
                              {1.0, 1.1, 1.2},
                              {1.3, 1.4, 1.5},
                              {1.6, 1.7, 1.8},
                              {1.9, 2.0, 2.1},
                              {2.2, 2.3, 2.4},
                              {2.5, 2.6, 2.7},
                              {2.8, 2.9, 3.0}};
    vecvec<Float> expectedAvg{{}, {}, {}, {0.55}, {}, {}, {1.55}, {}, {}, {2.55}};
    vecvec<Float> expectedRms{{}, {}, {}, {0.62048368229954287}, {}, {}, {1.5763882770434448}, {}, {}, {2.5661254840712679}};
    vecvec<Int> expectedDomain{{}, {}, {}, {3}, {}, {}, {23}, {}, {}, {43}};
    Int initialOutputDomain = 0;  // Implicit

    auto helper = StatisticsTestHelper(LinearDataRule(2, 3),
                                       initialOutputDomain,
                                       expectedAvg,
                                       expectedRms,
                                       expectedDomain,
                                       SampleTypeFromType<Float>::SampleType,
                                       mockPackets);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestLargerPackets)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5},
                              {1.6, 1.7, 1.8, 1.9, 2.0, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0}};
    vecvec<Float> expectedAvg{{0.55}, {1.55, 2.55}};
    vecvec<Float> expectedRms{{0.62048368229954287}, {1.5763882770434448, 2.5661254840712679}};
    vecvec<Int> expectedDomain{{3}, {23, 43}};
    Int initialOutputDomain = 0;  // Implicit

    auto helper = StatisticsTestHelper(LinearDataRule(2, 3),
                                       initialOutputDomain,
                                       expectedAvg,
                                       expectedRms,
                                       expectedDomain,
                                       SampleTypeFromType<Float>::SampleType,
                                       mockPackets);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestBlockSizeChanged)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0}};
    vecvec<Float> expectedAvg{{0.55}, {1.3, 1.8}, {2.3, 2.8}};
    vecvec<Float> expectedRms{{0.62048368229954287}, {1.3076696830622021, 1.8055470085267789}, {2.3043437243605824, 2.803569153775237}};
    vecvec<Int> expectedDomain{{3}, {23, 33}, {43, 53}};
    std::vector<Int> blockSizeChangesAfterPackets{0};
    std::vector<size_t> newBlockSizes{5};
    Int initialOutputDomain = 0;  // Implicit

    auto helper = StatisticsTestHelper(LinearDataRule(2, 3),
                                       initialOutputDomain,
                                       expectedAvg,
                                       expectedRms,
                                       expectedDomain,
                                       SampleTypeFromType<Float>::SampleType,
                                       mockPackets,
                                       {},
                                       blockSizeChangesAfterPackets,
                                       newBlockSizes);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestExplicitOutputDomain)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0}};
    vecvec<Float> expectedAvg{{0.55}, {1.55}, {2.55}};
    vecvec<Float> expectedRms{{0.62048368229954287}, {1.5763882770434448}, {2.5661254840712679}};
    vecvec<Int> expectedDomain{{5}, {25}, {45}};
    Int initialOutputDomain = 1;  // Explicit

    auto helper = StatisticsTestHelper(LinearDataRule(2, 5),
                                       initialOutputDomain,
                                       expectedAvg,
                                       expectedRms,
                                       expectedDomain,
                                       SampleTypeFromType<Float>::SampleType,
                                       mockPackets);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestExplicitRangeOutputDomain)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0}};
    vecvec<Float> expectedAvg{{0.55}, {1.55}, {2.55}};
    vecvec<Float> expectedRms{{0.62048368229954287}, {1.5763882770434448}, {2.5661254840712679}};
    vecvec<Int> expectedDomainStart{{5}, {25}, {45}};
    vecvec<Int> expectedDomainEnd{{24}, {44}, {64}};
    Int initialOutputDomain = 2;  // ExplicitRange

    auto helper = StatisticsTestHelper(LinearDataRule(2, 5),
                                       initialOutputDomain,
                                       expectedAvg,
                                       expectedRms,
                                       expectedDomainStart,
                                       SampleTypeFromType<Float>::SampleType,
                                       mockPackets,
                                       expectedDomainEnd);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestChangingOutputDomain)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0},
                              {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0}};
    vecvec<Float> expectedAvg{{0.55}, {1.55}, {2.55}, {0.55}};
    vecvec<Float> expectedRms{{0.62048368229954287}, {1.5763882770434448}, {2.5661254840712679}, {0.62048368229954287}};
    vecvec<Int> expectedDomainStart{{5}, {25}, {45}, {65}};
    Int initialOutputDomain = 0;  // Implicit
    std::vector<Int> outputDomainChangesAfterPackets{0, 1};
    std::vector<Int> newOutputDomain{1, 0};  // Explicit, Implicit
    auto helper = StatisticsTestHelper(LinearDataRule(2, 5),
                                       initialOutputDomain,
                                       expectedAvg,
                                       expectedRms,
                                       expectedDomainStart,
                                       SampleTypeFromType<Float>::SampleType,
                                       mockPackets,
                                       {},
                                       {},
                                       {},
                                       outputDomainChangesAfterPackets,
                                       newOutputDomain);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestTriggerBasic)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0},
                              {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0}};
    vecvec<Float> expectedAvg{{0.55}, {}, {2.55}, {}, {1.55}, {2.55}};
    vecvec<Float> expectedRms{
        {0.62048368229954287}, {1.5763882770434448}, {2.5661254840712679}, {}, {1.5763882770434448}, {2.5661254840712679}};
    vecvec<Int> expectedDomain{{5}, {}, {45}, {}, {85}, {105}};
    Int initialOutputDomain = 0;  // Implicit

    std::vector<Int> triggerModeChangesAfterPackets{0, 4};
    std::vector<Bool> newTriggerMode{true, false};  // TODO test true true, false false
    vecvec<Float> mockTriggerPackets{
        {}, {0.1, 0.6}, {0.1, 0.2}, {0.6, 0.8}, {0.1, 1.0}, {}};  // TODO test sending while TriggerMode = false
    vecvec<Float> mockTriggerDomainPackets{{}, {8, 10}, {28, 30}, {48, 50}, {68, 70}, {}};

    auto helper = StatisticsTestHelper(LinearDataRule(2, 5),
                                       initialOutputDomain,
                                       expectedAvg,
                                       expectedRms,
                                       expectedDomain,
                                       SampleTypeFromType<Float>::SampleType,
                                       mockPackets,
                                       {},
                                       {},
                                       {},
                                       {},
                                       {},
                                       triggerModeChangesAfterPackets,
                                       newTriggerMode,
                                       mockTriggerPackets,
                                       mockTriggerDomainPackets);
    helper.run();
}
