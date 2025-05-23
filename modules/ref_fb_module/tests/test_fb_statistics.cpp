#include <opendaq/context_internal_ptr.h>
#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>
#include <ref_fb_module/module_dll.h>
#include <testutils/memcheck_listener.h>

using namespace daq;

template <typename T>
using vecvec = std::vector<std::vector<T> >;

class StatisticsTest : public testing::Test
{
};

class StatisticsTestStatus: public StatisticsTest
{
public:
    FunctionBlockPtr fb;
    ContextPtr context;
protected:
    void SetUp() override
    {
        // Create module
        ModulePtr module;
        const auto logger = Logger();
        auto moduleManager = ModuleManager("[[none]]");
        context = Context(Scheduler(logger), logger, TypeManager(), moduleManager, nullptr);
        createModule(&module, context);
        moduleManager.addModule(module);
        moduleManager = context.asPtr<IContextInternal>().moveModuleManager();

        // Crate config
        PropertyObjectPtr config = module.getAvailableFunctionBlockTypes().get("RefFBModuleStatistics").createDefaultConfig();
        config.setPropertyValue("UseMultiThreadedScheduler", false);

        // Create function block
        fb = module.createFunctionBlock("RefFBModuleStatistics", nullptr, "fb", config);
    }
};

class StatisticsTestStatusSignal : public StatisticsTestStatus
{
public:
    SignalConfigPtr signal;
protected:
    void SetUp() override
    {
        StatisticsTestStatus::SetUp();

        // Create signal with descriptor
        auto signalDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).setRule(ExplicitDataRule()).build();
        signal = SignalWithDescriptor(context, signalDescriptor, nullptr, "signal");
    }
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
                         const std::vector<Int>& overlapChangesBeforePackets = {},
                         const std::vector<Int>& newOverlaps = {},
                         const vecvec<TT>& mockTriggerPackets = {},
                         const vecvec<Int>& mockTriggerDomainPackets = {},
                         const SampleType& sampleTypeTrigger = SampleTypeFromType<Float>::SampleType)
    {
        // Create logger, module manager, context and module
        const auto logger = Logger();
        moduleManager = ModuleManager("[[none]]");
        context = Context(Scheduler(logger), logger, TypeManager(), moduleManager, nullptr);
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
        this->overlapChangesBeforePackets = overlapChangesBeforePackets;
        this->newOverlaps = newOverlaps;
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
    std::vector<Int> overlapChangesBeforePackets;
    std::vector<Int> newOverlaps;
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
            auto domainPacketData = static_cast<Int*>(domainPacket.getRawData());
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
        PropertyObjectPtr config = module.getAvailableFunctionBlockTypes().get("RefFBModuleStatistics").createDefaultConfig();
        config.setPropertyValue("UseMultiThreadedScheduler", false);

        // Create function block
        fb = module.createFunctionBlock("RefFBModuleStatistics", nullptr, "fb", config);

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
            // Check if we should change packet overlap after sending packet
            auto overlapChangeFoundAt =
                std::find(overlapChangesBeforePackets.begin(), overlapChangesBeforePackets.end(), static_cast<Int>(i));
            if (overlapChangeFoundAt != overlapChangesBeforePackets.end())
            {
                fb.setPropertyValue("Overlap", newOverlaps[overlapChangeFoundAt - overlapChangesBeforePackets.begin()]);
            }

            // Check if we should change trigger mode before sending packet
            auto triggerModeChangeFoundAt =
                std::find(triggerModeChangesBeforePackets.begin(), triggerModeChangesBeforePackets.end(), static_cast<Int>(i));
            if (triggerModeChangeFoundAt != triggerModeChangesBeforePackets.end())
            {
                // Change trigger mode if appropriate
                bool enableTrigger = newTriggerMode[triggerModeChangeFoundAt - triggerModeChangesBeforePackets.begin()];
                if (enableTrigger && fb.getFunctionBlocks().getCount() == 0)
                {
                    fb.addFunctionBlock("RefFBModuleTrigger");
                }
                else if (!enableTrigger && fb.getFunctionBlocks().getCount() > 0)
                {
                    fb.removeFunctionBlock(fb.getFunctionBlocks()[0]);
                }
            }

            // Create and send data packet for trigger
            if (mockTriggerPackets.size() > 0 && mockTriggerPackets[i].size() > 0)
            {
                // Prepare data packet for trigger
                auto triggerDataPacket =
                    DataPacketWithDomain(triggerDomainPackets[i], triggerSignalDescriptor, mockTriggerPackets[i].size());
                auto triggerPacketData = static_cast<TT*>(triggerDataPacket.getRawData());
                for (size_t ii = 0; ii < mockTriggerPackets[i].size(); ii++)
                    *triggerPacketData++ = static_cast<TT>(mockTriggerPackets[i][ii]);

                ASSERT_EQ(fb.getFunctionBlocks().getCount(), 1);
                FunctionBlockPtr triggerFb = fb.getFunctionBlocks()[0];

                // Nested trigger connect signal
                triggerFb.getInputPorts()[0].connect(triggerSignal);

                // Send packet for trigger
                triggerDomainSignal.sendPacket(triggerDomainPackets[i]);
                triggerSignal.sendPacket(triggerDataPacket);
            }

            // Create data packet for statistics
            auto dataPacket = DataPacketWithDomain(domainPackets[i], signalDescriptor, mockPackets[i].size());
            auto packetData = static_cast<T*>(dataPacket.getRawData());
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

                auto domainPacket = dataPacket.getDomainPacket();
                auto domainDataDescriptor = domainPacket.getDataDescriptor();

                const size_t sampleCount = dataPacket.getSampleCount();

                for (size_t ii = 0; ii < expectedData[i].size(); ii++)
                {
                    auto dataSample = data[ii];

                    // Assert that packet has expected number of samples
                    ASSERT_EQ(sampleCount, expectedData[i].size());
                    // Assert that data sample equals expected value
                    ASSERT_DOUBLE_EQ(dataSample, expectedData[i][ii]);

                    if (domainDataDescriptor.getRule().getType() == DataRuleType::Explicit &&
                        domainDataDescriptor.getSampleType() == SampleType::RangeInt64)
                    {
                        // In case of ExplicitRange
                        auto domainData = static_cast<RangeData*>(domainPacket.getData());
                        const size_t domainSampleCount = domainPacket.getSampleCount();

                        // Assert that domain packet has expected number of samples
                        ASSERT_EQ(domainSampleCount, expectedDomain[i].size());

                        // Assert that domain sample start equals expected value
                        ASSERT_EQ(domainData[ii].low, expectedDomain[i][ii]);

                        // Assert that domain sample end equals expected value
                        ASSERT_EQ(domainData[ii].high, expectedDomainEnd[i][ii]);
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
                                       {},
                                       {},
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
                                       {},
                                       {},
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
                                       {},
                                       {},
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
                                       {},
                                       {},
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
                                       {},
                                       {},
                                       mockTriggerPackets,
                                       mockTriggerDomainPackets);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestOverlapImplicitRule)
{
    // default block size = 10

    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0},
                              {3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9, 4.0},
                              {4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8, 4.9, 5.0},
                              {5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7, 5.8, 5.9, 6.0}};
    vecvec<Float> expectedAvg{{0.55},
                              {1.05, 1.55},
                              {2.05, 2.55},
                              {3.05, 3.55},
                              {4.05, 4.55},
                              {5.05, 5.55}};
    vecvec<Float> expectedRms{{0.6204836822995429},
                              {1.088577052853862, 1.5763882770434448},
                              {2.0700241544484452, 2.566125484071268},
                              {3.063494736408078, 3.561600763701625},
                              {4.060172410132358, 4.5590569200219475},
                              {5.058161721416191, 5.557427462414602}};
    vecvec<Int> expectedDomain{{0},
                               {5, 10},
                               {15, 20},
                               {25, 30},
                               {35, 40},
                               {45, 50}};
    Int initialOutputDomain = 0;  // Implicit
    std::vector<Int> overlapChangesBeforePackets{0};
    std::vector<Int> newOverlaps{50}; // overlaps measured in percents (%)

    auto helper = StatisticsTestHelper(LinearDataRule(1, 0),
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
                                       {},
                                       {},
                                       overlapChangesBeforePackets,
                                       newOverlaps);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestOverlapExplicitRule)
{
    // default block size = 10

    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0},
                              {3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9, 4.0},
                              {4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8, 4.9, 5.0},
                              {5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7, 5.8, 5.9, 6.0}};
    vecvec<Float> expectedAvg{{0.55},
                              {1.2499999999999998},
                              {1.95},
                              {2.65, 3.35},
                              {4.05},
                              {4.75, 5.45}};
    vecvec<Float> expectedRms{{0.6204836822995429},
                              {1.282575533838066},
                              {1.9710403344427023},
                              {2.665520587052368, 3.3622908856908857},
                              {4.060172410132358},
                              {4.758676286531791, 5.457563558951925}};
    vecvec<Int> expectedDomain{{0},
                               {7},
                               {14},
                               {21, 28},
                               {35},
                               {42, 49}};
    Int initialOutputDomain = 1;  // Explicit
    std::vector<Int> overlapChangesBeforePackets{0};
    std::vector<Int> newOverlaps{30}; // overlaps measured in percents (%)

    auto helper = StatisticsTestHelper(LinearDataRule(1, 0),
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
                                       {},
                                       {},
                                       overlapChangesBeforePackets,
                                       newOverlaps);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestOverlapExplicitRangeRule)
{
    // default block size = 10

    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0},
                              {3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9, 4.0},
                              {4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8, 4.9, 5.0},
                              {5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7, 5.8, 5.9, 6.0}};
    vecvec<Float> expectedAvg{{0.55},
                              {1.2499999999999998},
                              {1.95},
                              {2.65, 3.35},
                              {4.05},
                              {4.75, 5.45}};
    vecvec<Float> expectedRms{{0.6204836822995429},
                              {1.282575533838066},
                              {1.9710403344427023},
                              {2.665520587052368, 3.3622908856908857},
                              {4.060172410132358},
                              {4.758676286531791, 5.457563558951925}};
    vecvec<Int> expectedDomain{{0},
                               {7},
                               {14},
                               {21, 28},
                               {35},
                               {42, 49}};
    vecvec<Int> expectedDomainEnd{{9},
                                  {16},
                                  {23},
                                  {30, 37},
                                  {44},
                                  {51, 58}};
    Int initialOutputDomain = 2;  // Explicit range
    std::vector<Int> overlapChangesBeforePackets{0};
    std::vector<Int> newOverlaps{30}; // overlaps measured in percents (%)

    auto helper = StatisticsTestHelper(LinearDataRule(1, 0),
                                       initialOutputDomain,
                                       expectedAvg,
                                       expectedRms,
                                       expectedDomain,
                                       SampleTypeFromType<Float>::SampleType,
                                       mockPackets,
                                       expectedDomainEnd,
                                       {},
                                       {},
                                       {},
                                       {},
                                       {},
                                       {},
                                       overlapChangesBeforePackets,
                                       newOverlaps);
    helper.run();
}

TEST_F(StatisticsTest, StatisticsTestOverlapWithTrigger)
{
    // default block size = 10
    // trigger default threshold = 0.5

    vecvec<Float> mockPackets{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0},
                              {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0},
                              {2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3.0},
                              {3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9, 4.0},
                              {4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8, 4.9, 5.0},
                              {5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7, 5.8, 5.9, 6.0}};
    vecvec<Float> expectedAvg{{0.55},
                              {1.2499999999999998},
                              {},
                              {3.05},
                              {3.75},
                              {5.35}};
    vecvec<Float> expectedRms{{0.6204836822995429},
                              {1.282575533838066},
                              {},
                              {3.063494736408078},
                              {3.7609839138182974},
                              {5.357704732439069}};
    vecvec<Int> expectedDomain{{0},
                               {7},
                               {},
                               {25},
                               {32},
                               {48}};
    vecvec<Int> expectedDomainEnd{{9},
                                  {16},
                                  {},
                                  {34},
                                  {41},
                                  {57}};
    Int initialOutputDomain = 2;  // Explicit
    std::vector<Int> overlapChangesBeforePackets{0};
    std::vector<Int> newOverlaps{30}; // overlaps measured in percents (%)

    std::vector<Int> triggerModeChangesBeforePackets{0};
    std::vector<Bool> newTriggerMode{true};

    vecvec<Float> mockTriggerPackets{{1.0}, {0.1}, {1.0}, {0.1}, {1.0}, {}};
    vecvec<Int> mockTriggerDomainPackets{{0}, {22}, {25}, {46}, {48}, {}};

    auto helper = StatisticsTestHelper(LinearDataRule(1, 0),
                                       initialOutputDomain,
                                       expectedAvg,
                                       expectedRms,
                                       expectedDomain,
                                       SampleTypeFromType<Float>::SampleType,
                                       mockPackets,
                                       expectedDomainEnd,
                                       {},
                                       {},
                                       {},
                                       {},
                                       triggerModeChangesBeforePackets,
                                       newTriggerMode,
                                       overlapChangesBeforePackets,
                                       newOverlaps,
                                       mockTriggerPackets,
                                       mockTriggerDomainPackets);
    helper.run();
}

TEST_F(StatisticsTestStatus, StatisticsOk1)
{
    // ComponentStatus is Ok
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Ok", context.getTypeManager()));
    ASSERT_EQ(fb.getStatusContainer().getStatusMessage("ComponentStatus"), "");
}

TEST_F(StatisticsTestStatus, StatisticsException1)
{
    // Trigger configure
    fb.setPropertyValue("BlockSize", 20);

    // Incomplete input signal descriptors
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"),
              Enumeration("ComponentStatusType", "Warning", context.getTypeManager()));
    ASSERT_EQ(fb.getStatusContainer().getStatusMessage("ComponentStatus"), "Incomplete input signal descriptors");
}

TEST_F(StatisticsTestStatusSignal, StatisticsException2)
{
    // Trigger configure
    fb.getInputPorts()[0].connect(signal);

    // Incomplete input signal descriptors
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"),
              Enumeration("ComponentStatusType", "Warning", context.getTypeManager()));
    ASSERT_EQ(fb.getStatusContainer().getStatusMessage("ComponentStatus"), "Incompatible domain data sample type Null");
}

TEST_F(StatisticsTestStatusSignal, StatisticsException3)
{
    // Create domain signal with descriptor
    auto domainSignalNonLinearDescriptor = DataDescriptorBuilder().setSampleType(SampleType::UInt64).setRule(ConstantDataRule()).build();
    auto domainSignalNonLinear = SignalWithDescriptor(context, domainSignalNonLinearDescriptor, nullptr, "signal");

    // Set domain signal
    signal.setDomainSignal(domainSignalNonLinear);

    // Trigger configure
    fb.getInputPorts()[0].connect(signal);

    // Incomplete input signal descriptors
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"),
              Enumeration("ComponentStatusType", "Warning", context.getTypeManager()));
    ASSERT_EQ(fb.getStatusContainer().getStatusMessage("ComponentStatus"), "Domain rule type is not Linear");
}

TEST_F(StatisticsTestStatusSignal, StatisticsOk2)
{
    // Create domain signal with descriptor
    auto domainSignalLinearDescriptor = DataDescriptorBuilder().setSampleType(SampleType::UInt64).setRule(LinearDataRule(1, 3)).build();
    auto domainSignal = SignalWithDescriptor(context, domainSignalLinearDescriptor, nullptr, "signal");

    // Set domain signal
    signal.setDomainSignal(domainSignal);

    // Trigger configure
    fb.getInputPorts()[0].connect(signal);

    // Successful configuration
    ASSERT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Ok", context.getTypeManager()));
    ASSERT_EQ(fb.getStatusContainer().getStatusMessage("ComponentStatus"), "");
}
