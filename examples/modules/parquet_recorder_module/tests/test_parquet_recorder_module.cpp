#include <algorithm>
#include <cstddef>
#include <filesystem>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <coretypes/filesystem.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/recorder.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/signal_factory.h>
#include <parquet/arrow/reader.h>
#include <parquet_recorder_module/module_dll.h>
#include <parquet_recorder_module/version.h>
#include <testutils/testutils.h>

using ParquetRecorderModuleTest = testing::Test;
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

static ModulePtr CreateModule()
{
    ModulePtr module;
    createModule(&module, NullContext());
    return module;
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

TEST_F(ParquetRecorderModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(ParquetRecorderModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getModuleInfo().getName(), "ParquetRecorderModule");
}

TEST_F(ParquetRecorderModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getModuleInfo().getVersionInfo().assigned());
}

TEST_F(ParquetRecorderModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getModuleInfo().getVersionInfo();

    ASSERT_EQ(version.getMajor(), PARQUET_RECORDER_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), PARQUET_RECORDER_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), PARQUET_RECORDER_MODULE_PATCH_VERSION);
}

TEST_F(ParquetRecorderModuleTest, WriteReadCompare)
{
    // initialize the module and create a function block
    auto module = CreateModuleWithScheduler();
    auto fb = module.createFunctionBlock("ParquetRecorder", nullptr, "fb");
    ASSERT_TRUE(fb.assigned());

    auto context = fb.getContext();
    auto scheduler = context.getScheduler();
    auto recorder = fb.asPtr<daq::IRecorder>(true);
    auto inputPort = fb.getInputPorts().getItemAt(0);
    ASSERT_TRUE(inputPort.assigned());
    
    auto signal = CreateSignal(context);

    inputPort.connect(signal);

    recorder->startRecording();

    // generate packets and feed them to the signal
    for (auto i = 0; i < 100'000; i += 100)
    {
        auto domain = DataPacket(signal.getDomainSignal().getDescriptor(), 100);
        auto data = DataPacket(signal.getDescriptor(), 100);
        std::iota(static_cast<SampleTypeToType<daq::SampleType::Float64>::Type*>(data.getData()),
                  static_cast<SampleTypeToType<daq::SampleType::Float64>::Type*>(data.getData()) + 100,
                  i);
        signal.sendPacket(data);
    }

    recorder->stopRecording();
    fb = nullptr;

    // find the file
    auto cwd = fs::directory_iterator(fs::current_path());
    auto parquetItem =
        std::find_if(fs::begin(cwd),
                     fs::end(cwd),
                     [](const fs::directory_entry& item) { return item.is_regular_file() && item.path().extension() == ".parquet"; });
    ASSERT_NE(parquetItem, fs::end(cwd));

    // try to read the file
    std::shared_ptr<arrow::io::ReadableFile> input = arrow::io::ReadableFile::Open(parquetItem->path().string()).ValueOrDie();
    std::unique_ptr<parquet::arrow::FileReader> arrow_reader = parquet::arrow::OpenFile(input, arrow::default_memory_pool()).ValueOrDie();

    std::shared_ptr<arrow::Table> table;
    auto status = arrow_reader->ReadTable(&table);

    ASSERT_TRUE(status.ok());

    SampleTypeToType<daq::SampleType::Float64>::Type expectedValue = 0;
    size_t numRows = table->num_rows();
    ASSERT_EQ(numRows, 100'000);

    for (const auto& chunk : table->column(1)->chunks())
    {
        auto array = std::static_pointer_cast<arrow::DoubleArray>(chunk);
        for (const auto& value : *array)
            ASSERT_EQ(value, expectedValue++);
    }
}
