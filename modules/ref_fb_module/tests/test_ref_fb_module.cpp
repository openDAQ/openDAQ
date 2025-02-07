#include <coretypes/common.h>
#include <opendaq/context_factory.h>
#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/signal_factory.h>
#include <ref_fb_module/module_dll.h>
#include <ref_fb_module/version.h>
#include <testutils/testutils.h>
#include <thread>
#include "testutils/memcheck_listener.h"

using RefFbModuleTest = testing::Test;
using namespace daq;

class ReferenceDomainOffsetHelper
{
public:
    ModulePtr module;
    DataDescriptorPtr signalDescriptor;
    SignalConfigPtr signal;
    uint64_t sampleCount;
    DataPacketPtr domainPacket;
    SignalConfigPtr domainSignal;
    ContextPtr context;

    ReferenceDomainOffsetHelper(uint64_t count = 5)
    {
        // Save desired sample count for later
        sampleCount = count;
        // Create domain signal
        auto logger = Logger();
        context = Context(Scheduler(logger), logger, TypeManager(), nullptr, nullptr);
        auto domainSignalDescriptor = DataDescriptorBuilder()
                                          .setUnit(Unit("s", -1, "seconds", "time"))
                                          .setSampleType(SampleType::Int64)
                                          .setRule(LinearDataRule(5, 3))
                                          .setOrigin("1970")
                                          .setTickResolution(Ratio(1, 1000))
                                          .setReferenceDomainInfo(ReferenceDomainInfoBuilder().setReferenceDomainOffset(100).build())
                                          .build();
        domainSignal = SignalWithDescriptor(context, domainSignalDescriptor, nullptr, "DomainSignal");
        domainPacket = DataPacket(domainSignalDescriptor, count, 1);
        // Create signal with descriptor
        signalDescriptor =
            DataDescriptorBuilder().setSampleType(SampleType::Float64).setValueRange(Range(0, 300)).setRule(ExplicitDataRule()).build();
        signal = SignalWithDescriptor(context, signalDescriptor, nullptr, "Signal");
        // Set domain signal of signal
        signal.setDomainSignal(domainSignal);
        // Create module
        createModule(&module, context);
    }

    std::vector<int64_t> sendAndReceive(SignalPtr fbSignal)
    {
        // Create reader
        auto reader = PacketReader(fbSignal);

        // Create data packet
        auto dataPacket = DataPacketWithDomain(domainPacket, signalDescriptor, sampleCount);
        auto packetData = static_cast<double*>(dataPacket.getRawData());
        for (size_t i = 0; i < sampleCount; i++)
            *packetData++ = static_cast<double>(i);

        // Send packet
        domainSignal.sendPacket(domainPacket);
        signal.sendPacket(dataPacket);

        PacketPtr receivedPacket;
        while (true)
        {
            receivedPacket = reader.read();
            if (receivedPacket.assigned() && receivedPacket.getType() == PacketType::Data)
                break;
        }

        context.getScheduler().stop();

        auto receivedDomainPacket = receivedPacket.asPtr<IDataPacket>().getDomainPacket();
        auto* data = receivedDomainPacket.getData();
        auto dataSize = receivedDomainPacket.getDataSize();
        auto vectorSize = dataSize / sizeof(int64_t);
        auto domainPacketData = std::vector<int64_t>(vectorSize);
        std::memcpy(domainPacketData.data(), data, dataSize);
        return domainPacketData;
    }
};

static ModulePtr CreateModule()
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, Context(Scheduler(logger), logger, TypeManager(), nullptr, nullptr));
    return module;
}

TEST_F(RefFbModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(RefFbModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getModuleInfo().getName(), "ReferenceFunctionBlockModule");
}

TEST_F(RefFbModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getModuleInfo().getVersionInfo().assigned());
}

TEST_F(RefFbModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getModuleInfo().getVersionInfo();

    ASSERT_EQ(version.getMajor(), REF_FB_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), REF_FB_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), REF_FB_MODULE_PATCH_VERSION);
}

TEST_F(RefFbModuleTest, EnumerateDevices)
{
    auto module = CreateModule();

    ListPtr<IDeviceInfo> deviceInfoDict;
    ASSERT_NO_THROW(deviceInfoDict = module.getAvailableDevices());
    ASSERT_EQ(deviceInfoDict.getCount(), 0u);
}

TEST_F(RefFbModuleTest, GetAvailableComponentTypes)
{
    const auto module = CreateModule();

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 0u);

    DictPtr<IString, IServerType> serverTypes;
    ASSERT_NO_THROW(serverTypes = module.getAvailableServerTypes());
    ASSERT_EQ(serverTypes.getCount(), 0u);

    DictPtr<IString, IFunctionBlockType> functionBlockTypes;
    ASSERT_NO_THROW(functionBlockTypes = module.getAvailableFunctionBlockTypes());
    ASSERT_TRUE(functionBlockTypes.assigned());
#ifdef __APPLE__
    ASSERT_EQ(functionBlockTypes.getCount(), 8u);
#else
    ASSERT_EQ(functionBlockTypes.getCount(), 9u);
    ASSERT_TRUE(functionBlockTypes.hasKey("RefFBModuleRenderer"));
    ASSERT_EQ("RefFBModuleRenderer", functionBlockTypes.get("RefFBModuleRenderer").getId());
#endif
    ASSERT_TRUE(functionBlockTypes.hasKey("RefFBModuleStatistics"));
    ASSERT_EQ("RefFBModuleStatistics", functionBlockTypes.get("RefFBModuleStatistics").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("RefFBModulePower"));
    ASSERT_EQ("RefFBModulePower", functionBlockTypes.get("RefFBModulePower").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("RefFBModulePowerReader"));
    ASSERT_EQ("RefFBModulePowerReader", functionBlockTypes.get("RefFBModulePowerReader").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("RefFBModuleScaling"));
    ASSERT_EQ("RefFBModuleScaling", functionBlockTypes.get("RefFBModuleScaling").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("RefFBModuleClassifier"));
    ASSERT_EQ("RefFBModuleClassifier", functionBlockTypes.get("RefFBModuleClassifier").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("RefFBModuleTrigger"));
    ASSERT_EQ("RefFBModuleTrigger", functionBlockTypes.get("RefFBModuleTrigger").getId());

    ASSERT_TRUE(functionBlockTypes.hasKey("RefFBModuleStructDecoder"));
    ASSERT_EQ("RefFBModuleStructDecoder", functionBlockTypes.get("RefFBModuleStructDecoder").getId());

    // Check module info for module
    ModuleInfoPtr moduleInfo;
    ASSERT_NO_THROW(moduleInfo = module.getModuleInfo());
    ASSERT_NE(moduleInfo, nullptr);
    ASSERT_EQ(moduleInfo.getName(), "ReferenceFunctionBlockModule");
    ASSERT_EQ(moduleInfo.getId(), "ReferenceFunctionBlockModule");

    // Check version info for module
    VersionInfoPtr versionInfoModule;
    ASSERT_NO_THROW(versionInfoModule = moduleInfo.getVersionInfo());
    ASSERT_NE(versionInfoModule, nullptr);
    ASSERT_EQ(versionInfoModule.getMajor(), REF_FB_MODULE_MAJOR_VERSION);
    ASSERT_EQ(versionInfoModule.getMinor(), REF_FB_MODULE_MINOR_VERSION);
    ASSERT_EQ(versionInfoModule.getPatch(), REF_FB_MODULE_PATCH_VERSION);

    // Check module and version info for function block types
    for (const auto& functionBlockType : functionBlockTypes)
    {
        ModuleInfoPtr moduleInfoFunctionBlockType;
        ASSERT_NO_THROW(moduleInfoFunctionBlockType = functionBlockType.second.getModuleInfo());
        ASSERT_NE(moduleInfoFunctionBlockType, nullptr);
        ASSERT_EQ(moduleInfoFunctionBlockType.getName(), "ReferenceFunctionBlockModule");
        ASSERT_EQ(moduleInfoFunctionBlockType.getId(), "ReferenceFunctionBlockModule");

        VersionInfoPtr versionInfoFunctionBlockType;
        ASSERT_NO_THROW(versionInfoFunctionBlockType = moduleInfoFunctionBlockType.getVersionInfo());
        ASSERT_NE(versionInfoFunctionBlockType, nullptr);
        ASSERT_EQ(versionInfoFunctionBlockType.getMajor(), REF_FB_MODULE_MAJOR_VERSION);
        ASSERT_EQ(versionInfoFunctionBlockType.getMinor(), REF_FB_MODULE_MINOR_VERSION);
        ASSERT_EQ(versionInfoFunctionBlockType.getPatch(), REF_FB_MODULE_PATCH_VERSION);
    }
}

TEST_F(RefFbModuleTest, CreateFunctionBlockNotFound)
{
    const auto module = CreateModule();

    ASSERT_THROW(module.createFunctionBlock("test", nullptr, "Id"), NotFoundException);
}

TEST_F(RefFbModuleTest, DISABLED_CreateFunctionBlockRenderer)
{
    MemCheckListener::expectMemoryLeak = true;

    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("RefFBModuleRenderer", nullptr, "Id");
    ASSERT_TRUE(fb.assigned());

    // std::this_thread::sleep_for(std::chrono::milliseconds(10000));
}

TEST_F(RefFbModuleTest, CreateFunctionBlockStatistics)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("RefFBModuleStatistics", nullptr, "Id");
    ASSERT_TRUE(fb.assigned());
}

TEST_F(RefFbModuleTest, StatisticsNumOfSignals)
{
    auto module = CreateModule();

    auto fb = module.createFunctionBlock("RefFBModuleStatistics", nullptr, "Id");
    ASSERT_EQ(fb.getSignals(search::Recursive(search::Any())).getCount(), 3u);
}

TEST_F(RefFbModuleTest, CreateFunctionBlockClassifier)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("RefFBModuleClassifier", nullptr, "Id");
    ASSERT_TRUE(fb.assigned());
}

TEST_F(RefFbModuleTest, CreateFunctionBlockTrigger)
{
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("RefFBModuleTrigger", nullptr, "Id");
    ASSERT_TRUE(fb.assigned());
}

TEST_F(RefFbModuleTest, AddFunctionBlockBackwardsCompat)
{
    const auto instance = Instance();

    instance.addFunctionBlock("ref_fb_module_classifier");
    instance.addFunctionBlock("ref_fb_module_fft");
    instance.addFunctionBlock("ref_fb_module_power");
    // instance.addFunctionBlock("ref_fb_module_renderer");
    instance.addFunctionBlock("ref_fb_module_scaling");
    instance.addFunctionBlock("ref_fb_module_statistics");
    instance.addFunctionBlock("ref_fb_module_trigger");
    instance.addFunctionBlock("audio_device_module_wav_writer");
}

TEST_F(RefFbModuleTest, TriggerWithReferenceDomainOffset)
{
    // Create helper
    auto help = ReferenceDomainOffsetHelper();

    // Fix for race condition
    auto config = help.module.getAvailableFunctionBlockTypes().get("RefFBModuleTrigger").createDefaultConfig();
    config.setPropertyValue("UseMultiThreadedScheduler", false);

    // Create function block
    auto fb = help.module.createFunctionBlock("RefFBModuleTrigger", nullptr, "FB", config);

    // Set input (port) and output (signal) of the function block
    fb.getInputPorts()[0].connect(help.signal);

    // Call helper method
    auto domainData = help.sendAndReceive(fb.getSignals()[0]);

    // Check domain data

    // input data:      0, 1, 2, 3, 4
    //                     ^ trigger, becauase greater than 0.5
    // input domain:    104, 109, 114, 119, 124 (offset = 1, start = 3, reference domain offset = 100, delta = 5)
    //                        ^ expected output domain, one sample with value 109

    ASSERT_EQ(domainData[0], 109);
}

TEST_F(RefFbModuleTest, ScalingWithReferenceDomainOffset)
{
    // Create helper
    auto help = ReferenceDomainOffsetHelper();

    // Create function block
    auto fb = help.module.createFunctionBlock("RefFBModuleScaling", nullptr, "FB");

    // Set input (port) and output (signal) of the function block
    fb.getInputPorts()[0].connect(help.signal);

    // Call helper method
    auto domainData = help.sendAndReceive(fb.getSignals()[0]);

    // Check domain data

    // input data:             0, 1, 2, 3, 4
    // input/output domain:    104, 109, 114, 119, 124 (offset = 1, start = 3, reference domain offset = 100, delta = 5)

    ASSERT_EQ(domainData[0], 104);
    ASSERT_EQ(domainData[1], 109);
    ASSERT_EQ(domainData[2], 114);
    ASSERT_EQ(domainData[3], 119);
    ASSERT_EQ(domainData[4], 124);
}

TEST_F(RefFbModuleTest, PowerWithReferenceDomainOffset)
{
    // Create helper
    auto help = ReferenceDomainOffsetHelper();

    // Create function block
    auto fb = help.module.createFunctionBlock("RefFBModulePower", nullptr, "FB");

    // Set input (port) and output (signal) of the function block
    fb.getInputPorts()[0].connect(help.signal);
    fb.getInputPorts()[1].connect(help.signal);

    // Call helper method
    auto domainData = help.sendAndReceive(fb.getSignals()[0]);

    // Check domain data

    // input data:             2x 0, 1, 2, 3, 4
    // input/output domain:    104, 109, 114, 119, 124 (offset = 1, start = 3, reference domain offset = 100, delta = 5)

    ASSERT_EQ(domainData[0], 104);
    ASSERT_EQ(domainData[1], 109);
    ASSERT_EQ(domainData[2], 114);
    ASSERT_EQ(domainData[3], 119);
    ASSERT_EQ(domainData[4], 124);
}

TEST_F(RefFbModuleTest, PowerReaderWithReferenceDomainOffset)
{
    // Create helper
    auto help = ReferenceDomainOffsetHelper();

    // Create function block
    auto fb = help.module.createFunctionBlock("RefFBModulePowerReader", nullptr, "FB");

    // Set input (port) and output (signal) of the function block
    fb.getInputPorts()[0].connect(help.signal);
    fb.getInputPorts()[1].connect(help.signal);

    // Call helper method
    auto domainData = help.sendAndReceive(fb.getSignals()[0]);

    // Check domain data

    // input data:             2x 0, 1, 2, 3, 4
    // input domain:           104, 109, 114, 119, 124
    // output domain:          109, 114, 119, 124 (offset = 1, start = 3, reference domain offset = 100, delta = 5)
    // starts later probably due to a philosohpical decision, however reference domain offset is certainly applied
    // also 1 fewer sample is provided in the output domain

    ASSERT_EQ(domainData[0], 109);
    ASSERT_EQ(domainData[1], 114);
    ASSERT_EQ(domainData[2], 119);
    ASSERT_EQ(domainData[3], 124);
}

TEST_F(RefFbModuleTest, StatisticsWithReferenceDomainOffset)
{
    // Create helper
    auto help = ReferenceDomainOffsetHelper(10);

    // Create function block
    auto fb = help.module.createFunctionBlock("RefFBModuleStatistics", nullptr, "FB");

    // Set input (port) and output (signal) of the function block
    fb.getInputPorts()[0].connect(help.signal);

    // Call helper method
    auto domainData = help.sendAndReceive(fb.getSignals()[0]);

    // Check domain data

    // input data:             0, 1, 2, 3, 4
    // input domain:           104, 109, 114, 119, 124 (offset = 1, start = 3, reference domain offset = 100, delta = 5)
    //                          ^

    ASSERT_EQ(domainData[0], 104);
}

TEST_F(RefFbModuleTest, FFTWithReferenceDomainOffset)
{
    // Create helper
    auto help = ReferenceDomainOffsetHelper(2048);

    // Create function block
    auto fb = help.module.createFunctionBlock("RefFBModuleFFT", nullptr, "FB");

    // Set input (port) and output (signal) of the function block
    fb.getInputPorts()[0].connect(help.signal);
    auto reader = PacketReader(fb.getSignals()[0]);

    // Call helper method
    auto domainData = help.sendAndReceive(fb.getSignals()[0]);

    // Check domain data

    // input data:             0, 1, 2, 3, 4
    // input domain:           104, 109, 114, 119, 124 (offset = 1, start = 3, reference domain offset = 100, delta = 5)
    //                          ^

    ASSERT_EQ(domainData[0], 104);
}

TEST_F(RefFbModuleTest, ClassifierWithReferenceDomainOffset)
{
    // Create helper
    auto help = ReferenceDomainOffsetHelper(100);

    // Create function block
    auto fb = help.module.createFunctionBlock("RefFBModuleClassifier", nullptr, "FB");
    fb.setPropertyValue("BlockSize", 5);

    // Set input (port) and output (signal) of the function block
    fb.getInputPorts()[0].connect(help.signal);
    auto reader = PacketReader(fb.getSignals()[0]);

    // Call helper method
    auto domainData = help.sendAndReceive(fb.getSignals()[0]);

    // Check domain data

    // input data:             0, 1, 2, 3, 4
    // input domain:           104, 109, 114, 119, 124 (offset = 1, start = 3, reference domain offset = 100, delta = 5)
    //                          ^

    ASSERT_EQ(domainData[0], 104);
}
