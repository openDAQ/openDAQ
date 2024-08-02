#include <thread>
#include <chrono>

#include <testutils/testutils.h>
#include <file_writer_module/module_dll.h>
#include <file_writer_module/version.h>
#include <gmock/gmock.h>

#include <coretypes/common.h>
#include <opendaq/opendaq.h>
#include <opendaq/module_ptr.h>
#include <opendaq/device_ptr.h>
#include <opendaq/context_factory.h>
#include <opendaq/context_internal_ptr.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/instance_factory.h>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/io/file.h>
#include <parquet/stream_reader.h>
#include <parquet/exception.h>

using namespace daq;

static ModulePtr CreateModule()
{
    ModulePtr module;
    createModule(&module, NullContext());
    return module;
}

static ModulePtr CreateModule(const ContextPtr& context)
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, context);
    return module;
}

static ContextPtr CreateContext()
{
    const auto logger = Logger();
    return Context(Scheduler(logger), logger, TypeManager(), nullptr, nullptr);
}

class FileWriterModuleTest : public testing::Test
{
protected:
    SignalConfigPtr timeSignal1;
    SignalConfigPtr timeSignal2;
    SignalConfigPtr voltageSignal;
    SignalConfigPtr currentSignal;
    SignalConfigPtr vibrationSignal;

    void createSignals(const ContextPtr& ctx)
    {
        timeSignal1 = Signal(ctx, nullptr, "time_1");
        timeSignal2 = Signal(ctx, nullptr, "time_2");

        voltageSignal = Signal(ctx, nullptr, "voltage");
        voltageSignal.setDomainSignal(timeSignal1);
        currentSignal = Signal(ctx, nullptr, "current");
        currentSignal.setDomainSignal(timeSignal1);

        vibrationSignal = Signal(ctx, nullptr, "vibration");
        vibrationSignal.setDomainSignal(timeSignal2);

        const auto timeDescriptor1 = DataDescriptorBuilder()
            .setSampleType(SampleType::Int64)
            .setTickResolution(Ratio(1, 1000))
            .setOrigin("1970-01-01T00:00:00")
            .setRule(LinearDataRule(1, 0))
            .setUnit(Unit("s", -1, "second", "time"))
            .build();
        timeSignal1.setDescriptor(timeDescriptor1);
        
        const auto timeDescriptor2 = DataDescriptorBuilder()
            .setSampleType(SampleType::Int64)
            .setTickResolution(Ratio(1, 1024))
            .setOrigin("1970-01-01T00:00:00")
            .setRule(LinearDataRule(1, 0))
            .setUnit(Unit("s", -1, "second", "time"))
            .build();
        timeSignal1.setDescriptor(timeDescriptor2);
        
        const auto voltageDescriptor = DataDescriptorBuilder()
            .setSampleType(SampleType::Float64)
            .build();
        voltageSignal.setDescriptor(voltageDescriptor);

        const auto currentDescriptor = DataDescriptorBuilder()
            .setSampleType(SampleType::Float64)
            .build();
        currentSignal.setDescriptor(currentDescriptor);

        const auto vibrationDescriptor = DataDescriptorBuilder()
            .setSampleType(SampleType::Float64)
            .build();
        vibrationSignal.setDescriptor(vibrationDescriptor);
    }
};

TEST_F(FileWriterModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(FileWriterModuleTest, ModuleName)
{
    auto module = CreateModule();
    ASSERT_EQ(module.getName(), "File writer module");
}

TEST_F(FileWriterModuleTest, VersionAvailable)
{
    auto module = CreateModule();
    ASSERT_TRUE(module.getVersionInfo().assigned());
}

TEST_F(FileWriterModuleTest, VersionCorrect)
{
    auto module = CreateModule();
    auto version = module.getVersionInfo();

    ASSERT_EQ(version.getMajor(), FILE_WRITER_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), FILE_WRITER_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), FILE_WRITER_MODULE_PATCH_VERSION);
}

TEST_F(FileWriterModuleTest, Create)
{
    const auto module = CreateModule(CreateContext());
    auto fb = module.createFunctionBlock("FileWriterModuleParquet", nullptr, "id");
    ASSERT_TRUE(fb.assigned());
}

TEST_F(FileWriterModuleTest, Connect)
{
    const auto ctx = CreateContext();
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("FileWriterModuleParquet", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals(ctx);

    // Test if after each connect a new input ports gets added.
    for (size_t i = 0; i < 10; ++i)
    {
        ASSERT_EQ(fb.getInputPorts().toVector().size(), i + 1);
        fb.getInputPorts()[i].connect(voltageSignal);
    }
}

TEST_F(FileWriterModuleTest, CheckSetProperties)
{
    auto ctx = CreateContext();
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("FileWriterModuleParquet", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    // Check Default Values
    ASSERT_EQ(fb.getPropertyValue("FileName"), "ExampleFile");
    ASSERT_EQ(fb.getPropertyValue("BatchCylce"), 20);
    ASSERT_EQ(fb.getPropertyValue("RecordingActive"), false);
    ASSERT_EQ(fb.getPropertyValue("Path"), "");

    // Manipulate
    fb.setPropertyValue("FileName", "openDAQData");
    fb.setPropertyValue("BatchCylce", 99);
    fb.setPropertyValue("RecordingActive", true);
    fb.setPropertyValue("Path", "myPath");
    ASSERT_EQ(fb.getPropertyValue("FileName"), "openDAQData");
    ASSERT_EQ(fb.getPropertyValue("BatchCylce"), 99);
    ASSERT_EQ(fb.getPropertyValue("RecordingActive"), true);
    ASSERT_EQ(fb.getPropertyValue("Path"), "myPath");

    // Try to set negative batch cycle
    fb.setPropertyValue("BatchCylce", -1);
    ASSERT_EQ(fb.getPropertyValue("BatchCylce"), 1);

    // Try to set min value
    fb.setPropertyValue("BatchCylce", 0);
    ASSERT_EQ(fb.getPropertyValue("BatchCylce"), 1);

    // Try to set max value
    fb.setPropertyValue("BatchCylce", 3601);
    ASSERT_EQ(fb.getPropertyValue("BatchCylce"), 3600);
}


TEST_F(FileWriterModuleTest, StoreOneSignal)
{
    auto ctx = CreateContext();
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("FileWriterModuleParquet", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals(ctx);
    fb.getInputPorts()[0].connect(voltageSignal);
    
    // Execute recording
    fb.setPropertyValue("RecordingActive", true);
    size_t packagesToSend = 10;
    for (size_t i = 0; i < packagesToSend; i++)
    {
        const auto timePacket = DataPacket(timeSignal1.getDescriptor(), 100, 1000 + 100 * i);
        timeSignal1.sendPacket(timePacket);
        const auto voltagePacket = DataPacketWithDomain(timePacket, voltageSignal.getDescriptor(), 100);
        double* inputData = static_cast<double*>(voltagePacket.getData());
        for (size_t dataPoint = 0; dataPoint < 100; ++dataPoint)
        {
            *inputData = (i * 100) + dataPoint;
            inputData++;
        }
        voltageSignal.sendPacket(voltagePacket);
    }
    // Recording to false triggers file write
    fb.setPropertyValue("RecordingActive", false);

    std::shared_ptr<arrow::io::ReadableFile> infile;

    // Created Recorded files have a file format
    // <fileName>_x_y.parquet
    // where x is table where signals are sharing a domain signal
    // where y is subcount if file batching is done.
    PARQUET_ASSIGN_OR_THROW(infile, arrow::io::ReadableFile::Open("ExampleFile_1_0.parquet"));

    parquet::StreamReader stream{parquet::ParquetFileReader::Open(infile)};
    
    size_t rowCount = 0;
    std::int64_t time;
    std::int64_t checkTime = 1000;
    double value;
    double checkValue = 0;
    while ( !stream.eof() )
    {
        stream >> time >> value >> parquet::EndRow;
        ASSERT_EQ(time, checkTime++);
        ASSERT_DOUBLE_EQ(value, checkValue++);
        rowCount++;
    }
    ASSERT_EQ(rowCount,1000);
}

TEST_F(FileWriterModuleTest, TestBatching)
{
    auto ctx = CreateContext();
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("FileWriterModuleParquet", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals(ctx);
    fb.getInputPorts()[0].connect(voltageSignal);
    
    // Execute recording and use other name for file
    fb.setPropertyValue("FileName", "openDAQData");
    fb.setPropertyValue("BatchCylce", 4);
    fb.setPropertyValue("RecordingActive", true);
    size_t packagesToSend = 10;
    for (size_t i = 0; i < packagesToSend; i++)
    {
        // Start later in time
        const auto timePacket = DataPacket(timeSignal1.getDescriptor(), 100, 2000 + 100 * i);
        timeSignal1.sendPacket(timePacket);
        const auto voltagePacket = DataPacketWithDomain(timePacket, voltageSignal.getDescriptor(), 100);
        double* inputData = static_cast<double*>(voltagePacket.getData());
        for (size_t dataPoint = 0; dataPoint < 100; ++dataPoint)
        {
            *inputData = (i * 100) + dataPoint;
            inputData++;
        }
        voltageSignal.sendPacket(voltagePacket);
        // At i == 3 it is the half of the 10 packages to be send.
        // The reason for is that package 0,1,2,3,4 needs to be send, but at package 4 
        // the evaluation is done if the time is over the BatchCylce
        if (i == 3)
        {
            // 500 to have some buffer even if the number is set to 4 sec.
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        }
    }
    // Recording to false triggers file write
    fb.setPropertyValue("RecordingActive", false);

    size_t rowCount = 0;
    std::int64_t time;
    std::int64_t checkTime = 2000;
    double value;
    double checkValue = 0;
    for (size_t bachNr = 0; bachNr < 2; ++bachNr)
    {
        std::shared_ptr<arrow::io::ReadableFile> infile;
        // Created Recorded files have a file format
        // <fileName>_x_y.parquet
        // where x is table where signals are sharing a domain signal
        // where y is subcount if file batching is done.
        // to batches are created booth with 500 lines of data
        PARQUET_ASSIGN_OR_THROW(infile, arrow::io::ReadableFile::Open("openDAQData_1_"+std::to_string(bachNr)+".parquet"));
        parquet::StreamReader stream{parquet::ParquetFileReader::Open(infile)};

        while ( !stream.eof() )
        {
            stream >> time >> value >> parquet::EndRow;
            ASSERT_EQ(time, checkTime++);
            ASSERT_DOUBLE_EQ(value, checkValue++);
            rowCount++;
        }
        ASSERT_EQ(rowCount,500);
        rowCount = 0;
        infile.reset();
    }
}