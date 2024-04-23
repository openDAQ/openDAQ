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

template <typename T, typename TT = Float>
class StatisticsTestHelper
{
public:
    StatisticsTestHelper(const DataRulePtr& rule,
                         const Int& initialOutputDomain,
                         const vecvec<Float>& expectedAvg,
                         const vecvec<Float>& expectedRms,
                         const vecvec<Int>& expectedDomain,
                         const SampleType& sampleType,
                         const vecvec<T>& mockPackets,
                         const vecvec<Int>& expectedDomainEnd = {},
                         const std::vector<Int>& blockSizeChangesAfterPackets = {},
                         const std::vector<size_t>& newBlockSizes = {},
                         const std::vector<Int>& outputDomainChangesAfterPackets = {},
                         const std::vector<Int>& newOutputDomain = {},
                         const std::vector<Int>& triggerModeChangesBeforePackets = {},
                         const std::vector<Bool>& newTriggerMode = {},
                         const vecvec<TT>& mockTriggerPackets = {},
                         const vecvec<Int>& mockTriggerDomainPackets = {},
                         const SampleType& sampleTypeTrigger = SampleTypeFromType<Float>::SampleType)
    {
        // Create logger, module manager, context and module
        const auto logger = Logger();
        moduleManager = ModuleManager("[[none]]");
        context = Context(Scheduler(logger), logger, nullptr, moduleManager, nullptr);
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
        this->triggerModeChangesBeforePackets = triggerModeChangesBeforePackets;
        this->newTriggerMode = newTriggerMode;
        this->mockTriggerPackets = mockTriggerPackets;
        this->mockTriggerDomainPackets = mockTriggerDomainPackets;
        this->sampleTypeTrigger = sampleTypeTrigger;
    }

    void run()
    {
        createDomainSignals();
        createDomainPackets();
        createSignals();
        createFunctionBlock();
        sendPacketsAndChangeProperties();
        receivePacketsAndCheckBoth();

        // Fix so PacketReadyNotification::Scheduler works
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
    std::vector<Int> triggerModeChangesBeforePackets;
    std::vector<Bool> newTriggerMode;
    vecvec<Float> mockTriggerPackets;
    vecvec<Int> mockTriggerDomainPackets;
    SampleType sampleTypeTrigger;
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
        triggerSignalDescriptorBuilder.setSampleType(sampleTypeTrigger);
        triggerSignalDescriptorBuilder.setValueRange(Range(0, 300));
        triggerSignalDescriptorBuilder.setRule(ExplicitDataRule());
        triggerSignalDescriptor = triggerSignalDescriptorBuilder.build();
        triggerSignal = SignalWithDescriptor(context, triggerSignalDescriptor, nullptr, "trigger_signal");

        // Set domain signal of signal for trigger
        triggerSignal.setDomainSignal(triggerDomainSignal);
    }

    void createFunctionBlock()
    {
        // Fix for race condition
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
            // Check if we should change trigger mode before sending packet
            auto triggerModeChangeFoundAt =
                std::find(triggerModeChangesBeforePackets.begin(), triggerModeChangesBeforePackets.end(), static_cast<Int>(i));
            if (triggerModeChangeFoundAt != triggerModeChangesBeforePackets.end())
            {
                // Change trigger mode if appropriate
                fb.setPropertyValue("TriggerMode", newTriggerMode[triggerModeChangeFoundAt - triggerModeChangesBeforePackets.begin()]);
            }

            // Create and send data packet for trigger
            if (mockTriggerPackets.size() > 0 && mockTriggerPackets[i].size() > 0)
            {
                // Prepare data packet for trigger
                auto triggerDataPacket =
                    DataPacketWithDomain(triggerDomainPackets[i], triggerSignalDescriptor, mockTriggerPackets[i].size());
                auto triggerPacketData = static_cast<TT*>(triggerDataPacket.getData());
                for (size_t ii = 0; ii < mockTriggerPackets[i].size(); ii++)
                    *triggerPacketData++ = static_cast<TT>(mockTriggerPackets[i][ii]);

                // Find nested trigger function block
                const auto& fbs = fb.getFunctionBlocks();
                FunctionBlockPtr triggerFb = nullptr;
                for (const auto& nfb : fbs)
                {
                    if (nfb.getLocalId() == "nfbt")
                    {
                        triggerFb = nfb;
                    }
                }

                // Nested trigger connect signal
                triggerFb.getInputPorts()[0].connect(triggerSignal);

                // Send packet for trigger
                triggerDomainSignal.sendPacket(triggerDomainPackets[i]);
                triggerSignal.sendPacket(triggerDataPacket);
            }

            // Create data packet for statistics
            auto dataPacket = DataPacketWithDomain(domainPackets[i], signalDescriptor, mockPackets[i].size());
            auto packetData = static_cast<T*>(dataPacket.getData());
            for (size_t ii = 0; ii < mockPackets[i].size(); ii++)
                *packetData++ = static_cast<T>(mockPackets[i][ii]);

            // Send packet for statistics
            domainSignal.sendPacket(domainPackets[i]);
            signal.sendPacket(dataPacket);

            // Check if we should change block size after sending packet
            auto blockSizeChangeFoundAt =
                std::find(blockSizeChangesAfterPackets.begin(), blockSizeChangesAfterPackets.end(), static_cast<Int>(i));
            if (blockSizeChangeFoundAt != blockSizeChangesAfterPackets.end())
            {
                // Change block size if appropriate
                fb.setPropertyValue("BlockSize", newBlockSizes[blockSizeChangeFoundAt - blockSizeChangesAfterPackets.begin()]);
            }

            // Check if we should change domain signal type after sending packet
            auto domainSignalChangeFoundAt =
                std::find(outputDomainChangesAfterPackets.begin(), outputDomainChangesAfterPackets.end(), static_cast<Int>(i));
            if (domainSignalChangeFoundAt != outputDomainChangesAfterPackets.end())
            {
                // Change domain signal type if appropriate
                fb.setPropertyValue("DomainSignalType",
                                    newOutputDomain[domainSignalChangeFoundAt - outputDomainChangesAfterPackets.begin()]);
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
    vecvec<Float> expectedAvg{{}, {}, {2.35}, {0.95}, {}, {}};
    vecvec<Float> expectedRms{{}, {}, {2.3674881203503428}, {1.3946325680981353}, {}, {}};
    vecvec<Int> expectedDomain{{}, {}, {41}, {61}, {}, {}};
    Int initialOutputDomain = 0;  // Implicit

    std::vector<Int> triggerModeChangesBeforePackets{0};
    std::vector<Bool> newTriggerMode{true};
    vecvec<Float> mockTriggerPackets{{0.1, 0.6, 0.1}, {}, {}, {}, {}, {}};
    vecvec<Int> mockTriggerDomainPackets{{26, 40, 90}, {}, {}, {}, {}, {}};

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
                                       triggerModeChangesBeforePackets,
                                       newTriggerMode,
                                       mockTriggerPackets,
                                       mockTriggerDomainPackets);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestTriggerDropHistory)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0},
                              {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0}};
    vecvec<Float> expectedAvg{{}, {}, {2.35}, {0.95}, {1.35}, {2.35}};
    vecvec<Float> expectedRms{{}, {}, {2.3674881203503428}, {1.3946325680981353}, {1.3802173741842263}, {2.3674881203503428}};
    vecvec<Int> expectedDomain{{}, {}, {41}, {61}, {81}, {101}};
    Int initialOutputDomain = 0;  // Implicit

    std::vector<Int> triggerModeChangesBeforePackets{0};
    std::vector<Bool> newTriggerMode{true};
    vecvec<Float> mockTriggerPackets{{0.6}, {}, {}, {}, {}, {}};
    vecvec<Int> mockTriggerDomainPackets{{40}, {}, {}, {}, {}, {}};

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
                                       triggerModeChangesBeforePackets,
                                       newTriggerMode,
                                       mockTriggerPackets,
                                       mockTriggerDomainPackets);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestTriggerTimely)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0},
                              {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0}};
    vecvec<Float> expectedAvg{{}, {}, {2.35}, {0.95}, {}, {}};
    vecvec<Float> expectedRms{{}, {}, {2.3674881203503428}, {1.3946325680981353}, {}, {}};
    vecvec<Int> expectedDomain{{}, {}, {41}, {61}, {}, {}};
    Int initialOutputDomain = 0;  // Implicit

    std::vector<Int> triggerModeChangesBeforePackets{0};
    std::vector<Bool> newTriggerMode{true};
    vecvec<Float> mockTriggerPackets{{}, {0.1, 0.6}, {}, {}, {0.1}, {}};
    vecvec<Int> mockTriggerDomainPackets{{}, {26, 40}, {}, {}, {90}, {}};

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
                                       triggerModeChangesBeforePackets,
                                       newTriggerMode,
                                       mockTriggerPackets,
                                       mockTriggerDomainPackets);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestTriggerMultipleChanges)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0},
                              {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0},
                              {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0}};
    vecvec<Float> expectedAvg{{}, {}, {2.35}, {}, {0.85}, {1.85}, {}, {}, {}};
    vecvec<Float> expectedRms{{}, {}, {2.3674881203503428}, {}, {0.89721792224631802}, {1.8721645226849055}, {}, {}, {}};
    vecvec<Int> expectedDomain{{}, {}, {41}, {}, {71}, {91}, {}, {}, {}};
    Int initialOutputDomain = 0;  // Implicit

    std::vector<Int> triggerModeChangesBeforePackets{0};
    std::vector<Bool> newTriggerMode{true};
    vecvec<Float> mockTriggerPackets{{0.6, 0.1, 0.6, 0.1}, {}, {}, {}, {}, {}, {}, {}, {}};
    vecvec<Int> mockTriggerDomainPackets{{40, 62, 70, 126}, {}, {}, {}, {}, {}, {}, {}, {}};

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
                                       triggerModeChangesBeforePackets,
                                       newTriggerMode,
                                       mockTriggerPackets,
                                       mockTriggerDomainPackets);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestTriggerMultipleChangesAndModeChanges)
{
    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0},
                              {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0},
                              {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0}};
    vecvec<Float> expectedAvg{{}, {}, {2.35}, {}, {0.85}, {1.85}, {}, {1.55}, {2.55}};
    vecvec<Float> expectedRms{
        {}, {}, {2.3674881203503428}, {}, {0.89721792224631802}, {1.8721645226849055}, {}, {1.5763882770434448}, {2.5661254840712679}};
    vecvec<Int> expectedDomain{{}, {}, {41}, {}, {71}, {91}, {}, {145}, {165}};
    Int initialOutputDomain = 0;  // Implicit

    std::vector<Int> triggerModeChangesBeforePackets{0, 1, 7, 8};
    std::vector<Bool> newTriggerMode{true, true, false, false};
    vecvec<Float> mockTriggerPackets{{0.6, 0.1, 0.6, 0.1}, {}, {}, {}, {}, {}, {}, {}, {}};
    vecvec<Int> mockTriggerDomainPackets{{40, 62, 70, 126}, {}, {}, {}, {}, {}, {}, {}, {}};

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
                                       triggerModeChangesBeforePackets,
                                       newTriggerMode,
                                       mockTriggerPackets,
                                       mockTriggerDomainPackets);
    helper.run();
}
