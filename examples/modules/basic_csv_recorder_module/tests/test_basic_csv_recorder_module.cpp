#include <testutils/testutils.h>
#include <basic_csv_recorder_module/module_dll.h>
#include <basic_csv_recorder_module/version.h>
#include <gmock/gmock.h>
#include <opendaq/module_ptr.h>
#include <opendaq/device_ptr.h>
#include <coretypes/common.h>
#include <coretypes/filesystem.h>
#include <opendaq/context_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/recorder.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/scheduler_factory.h>

using BasicCsvRecorderModuleTest = testing::Test;
using namespace daq;

static SignalConfigPtr CreateSignal(const ContextPtr& context)
{
    auto domainSignal = Signal(context, nullptr, "domain_signal");
    auto descriptorBuilder = DataDescriptorBuilder();
    descriptorBuilder.setName("DomainSignal")
        .setSampleType(SampleType::Int64)
        .setRule(LinearDataRule(1, 0))
        .setUnit(Unit("s", -1, "seconds", "time"));
    domainSignal.setDescriptor(descriptorBuilder.build());

    auto signal = Signal(context, nullptr, "test_signal");
    auto signalDescriptor = DataDescriptorBuilder()
                                .setName("TestSignal")
                                .setSampleType(SampleType::Float64)
                                .setRule(ExplicitDataRule())
                                .setUnit(Unit("V", -1, "volts", "voltage"))
                                .build();

    signal.setDescriptor(signalDescriptor);
    signal.setDomainSignal(domainSignal);

    return signal;
}

static ModulePtr CreateModuleWithScheduler()
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto context = Context(scheduler, logger, TypeManager(), nullptr);
    ModulePtr module;
    createModule(&module, context);
    return module;
}

static ModulePtr CreateModule()
{
    ModulePtr module;
    createModule(&module, NullContext());
    return module;
}

TEST_F(BasicCsvRecorderModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(BasicCsvRecorderModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getModuleInfo().getName(), "BasicCsvRecorderModule");
}

TEST_F(BasicCsvRecorderModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getModuleInfo().getVersionInfo().assigned());
}

TEST_F(BasicCsvRecorderModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getModuleInfo().getVersionInfo();

    ASSERT_EQ(version.getMajor(), BASIC_CSV_RECORDER_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), BASIC_CSV_RECORDER_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), BASIC_CSV_RECORDER_MODULE_PATCH_VERSION);
}

TEST_F(BasicCsvRecorderModuleTest, WritesToConfiguredSubdir)
{
    const fs::path subdir = fs::current_path() / "csv_test_subdir";

    auto cleanup = [&]()
    {
        fs::remove_all(subdir);
        for (const auto& item : fs::directory_iterator(fs::current_path()))
        {
            if (item.is_regular_file() && item.path().extension() == ".csv")
            fs::remove(item.path());
        }
    };

    cleanup();
    fs::create_directories(subdir);

    auto module = CreateModuleWithScheduler();
    auto fb = module.createFunctionBlock("BasicCsvRecorder", nullptr, "fb");
    ASSERT_TRUE(fb.assigned());

    auto context = fb.getContext();
    auto recorder = fb.asPtr<daq::IRecorder>(true);
    auto inputPort = fb.getInputPorts().getItemAt(0);
    ASSERT_TRUE(inputPort.assigned());

    auto signal = CreateSignal(context);
    inputPort.connect(signal);

    fb.setPropertyValue("Path", subdir.string());

    recorder->startRecording();

    for (auto i = 0; i < 1000; i += 100)
    {
        auto data = DataPacket(signal.getDescriptor(), 100);
        std::iota(static_cast<SampleTypeToType<daq::SampleType::Float64>::Type*>(data.getData()),
                  static_cast<SampleTypeToType<daq::SampleType::Float64>::Type*>(data.getData()) + 100,
                  i);
        signal.sendPacket(data);
    }

    recorder->stopRecording();
    fb = nullptr;

    bool foundInSubdir = false;
    for (const auto& item : fs::directory_iterator(subdir))
    {
        if (item.is_regular_file() && item.path().extension() == ".csv")
        {
            foundInSubdir = true;
            break;
        }
    }

    bool foundInCwd = false;
    for (const auto& item : fs::directory_iterator(fs::current_path()))
    {
        if (item.is_regular_file() && item.path().extension() == ".csv")
        {
            foundInCwd = true;
            break;
        }
    }

    ASSERT_TRUE(foundInSubdir);
    ASSERT_FALSE(foundInCwd);

    cleanup();
}