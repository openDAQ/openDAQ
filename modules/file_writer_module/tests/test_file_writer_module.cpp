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
#include <parquet/arrow/reader.h>

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

    SignalConfigPtr doubleSignal;
    SignalConfigPtr floatSignal;
    SignalConfigPtr int64Signal;
    SignalConfigPtr int32Signal;
    SignalConfigPtr int16Signal;
    SignalConfigPtr int8Signal;

    SignalConfigPtr uint64Signal;
    SignalConfigPtr uint32Signal;
    SignalConfigPtr uint16Signal;
    SignalConfigPtr uint8Signal;

    void createSignals(const ContextPtr& ctx)
    {
        timeSignal1 = Signal(ctx, nullptr, "time_1");
        timeSignal2 = Signal(ctx, nullptr, "time_2");

        doubleSignal = Signal(ctx, nullptr, "doubleSignal");
        doubleSignal.setDomainSignal(timeSignal1);
        floatSignal = Signal(ctx, nullptr, "floatSignal");
        floatSignal.setDomainSignal(timeSignal1);
        int64Signal = Signal(ctx, nullptr, "int64Signal");
        int64Signal.setDomainSignal(timeSignal1);
        int32Signal = Signal(ctx, nullptr, "int32Signal");
        int32Signal.setDomainSignal(timeSignal1);
        int16Signal = Signal(ctx, nullptr, "int16Signal");
        int16Signal.setDomainSignal(timeSignal1);
        int8Signal = Signal(ctx, nullptr, "int8Signal");
        int8Signal.setDomainSignal(timeSignal1);

        uint64Signal = Signal(ctx, nullptr, "uint64Signal");
        uint64Signal.setDomainSignal(timeSignal2);
        uint32Signal = Signal(ctx, nullptr, "uint32Signal");
        uint32Signal.setDomainSignal(timeSignal2);
        uint16Signal = Signal(ctx, nullptr, "uint16Signal");
        uint16Signal.setDomainSignal(timeSignal2);
        uint8Signal = Signal(ctx, nullptr, "uint8Signal");
        uint8Signal.setDomainSignal(timeSignal2);

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
        timeSignal2.setDescriptor(timeDescriptor2);
        
        const auto doubleDesciptor = DataDescriptorBuilder()
            .setSampleType(SampleType::Float64)
            .build();
        doubleSignal.setDescriptor(doubleDesciptor);
        const auto floatDesciptor = DataDescriptorBuilder()
            .setSampleType(SampleType::Float32)
            .build();
        floatSignal.setDescriptor(floatDesciptor);
        const auto int64Descriptor = DataDescriptorBuilder()
            .setSampleType(SampleType::Int64)
            .build();
        int64Signal.setDescriptor(int64Descriptor);
        const auto int32Desciptor = DataDescriptorBuilder()
            .setSampleType(SampleType::Int32)
            .build();
        int32Signal.setDescriptor(int32Desciptor);
        const auto int16Descriptor = DataDescriptorBuilder()
            .setSampleType(SampleType::Int16)
            .build();
        int16Signal.setDescriptor(int16Descriptor);
        const auto int8Descriptor = DataDescriptorBuilder()
            .setSampleType(SampleType::Int8)
            .build();
        int8Signal.setDescriptor(int8Descriptor);

        const auto uint64Desciptor = DataDescriptorBuilder()
            .setSampleType(SampleType::UInt64)
            .build();
        uint64Signal.setDescriptor(uint64Desciptor);
        const auto uint32Desciptor = DataDescriptorBuilder()
            .setSampleType(SampleType::UInt32)
            .build();
        uint32Signal.setDescriptor(uint32Desciptor);
        const auto uint16Descriptor = DataDescriptorBuilder()
            .setSampleType(SampleType::UInt16)
            .build();
        uint16Signal.setDescriptor(uint16Descriptor);
        const auto uint8Descriptor = DataDescriptorBuilder()
            .setSampleType(SampleType::UInt8)
            .build();
        uint8Signal.setDescriptor(uint8Descriptor);
    }

    template <SampleType InputSampleType>
    void sendSignalWithTime1(size_t packagesToSend, size_t sampleCount, int64_t startTimeOffset, size_t startPacketCount = 0, bool sendTimePacket = true)
    {
        using InputType = typename SampleTypeToType<InputSampleType>::Type;
        SignalConfigPtr signal;
        switch (InputSampleType)
        {
            case daq::SampleType::Float64:
                signal = doubleSignal;
                break;
            case daq::SampleType::Float32:
                signal = floatSignal;
                break;
            case daq::SampleType::Int64:
                signal = int64Signal;
                break;
            case daq::SampleType::Int32:
                signal = int32Signal;
                break;
            case daq::SampleType::Int16:
                signal = int16Signal;
                break;
            case daq::SampleType::Int8:
                signal = int8Signal;
                break;
            default:
                FAIL();
        }


        for (size_t i = startPacketCount; i < (startPacketCount + packagesToSend); ++i)
        {
            const auto timePacket = DataPacket(timeSignal1.getDescriptor(), sampleCount, startTimeOffset + sampleCount * i);
            if (sendTimePacket)
                timeSignal1.sendPacket(timePacket);

            const auto packet = DataPacketWithDomain(timePacket, signal.getDescriptor(), sampleCount);
            auto sampleType = signal.getDescriptor().getSampleType();
            auto inputData = static_cast<InputType*>(packet.getData());
            for (size_t dataPoint = 0; dataPoint < sampleCount; ++dataPoint)
            {
                *inputData = (i * sampleCount) + dataPoint;
                inputData++;
            }
            signal.sendPacket(packet);
        }
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
        fb.getInputPorts()[i].connect(doubleSignal);
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
    fb.setPropertyValue("DownScalingDevider", 5);
    fb.setPropertyValue("RecordingActive", true);
    fb.setPropertyValue("Path", "myPath");
    ASSERT_EQ(fb.getPropertyValue("FileName"), "openDAQData");
    ASSERT_EQ(fb.getPropertyValue("BatchCylce"), 99);
    ASSERT_EQ(fb.getPropertyValue("DownScalingDevider"), 5);
    ASSERT_EQ(fb.getPropertyValue("RecordingActive"), true);
    ASSERT_EQ(fb.getPropertyValue("Path"), "myPath");

    // If active it should be not possible to manipulate
    //fb.setPropertyValue("DownScalingDevider", 10);
    //ASSERT_EQ(fb.getPropertyValue("DownScalingDevider"), 5);

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

 
TEST_F(FileWriterModuleTest, SchemaTest)
{
    auto ctx = CreateContext();
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("FileWriterModuleParquet", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals(ctx);
    fb.getInputPorts()[0].connect(doubleSignal);
    fb.getInputPorts()[1].connect(floatSignal);
    fb.getInputPorts()[2].connect(int32Signal);
    fb.getInputPorts()[3].connect(int64Signal);
    //fb.getInputPorts()[4].connect(int16Signal);
    //fb.getInputPorts()[5].connect(int8Signal);

    // Execute recording
    fb.setPropertyValue("FileName", "Schema");
    fb.setPropertyValue("RecordingActive", true);
    fb.setPropertyValue("BatchCylce", 3600);

    // Send Data
    sendSignalWithTime1<daq::SampleType::Float64>(1, 50, 1000);
    sendSignalWithTime1<daq::SampleType::Float32>(1, 50, 1000, 0, false);
    sendSignalWithTime1<daq::SampleType::Int32>(1, 50, 1000, 0, false);
    sendSignalWithTime1<daq::SampleType::Int64>(1, 50, 1000, 0, false);
    //sendSignalWithTime1<daq::SampleType::Int16>(1, 50, 1000, 0, false);
    //sendSignalWithTime1<daq::SampleType::Int8>(1, 50, 1000, 0, false);

    // Recording to false triggers file write
    fb.setPropertyValue("RecordingActive", false);

    // Created Recorded files have a file format
    // <fileName>_x_y.parquet
    // where x is table where signals are sharing a domain signal
    // where y is subcount if file batching is done.    
    arrow::MemoryPool* pool = arrow::default_memory_pool();
    std::shared_ptr<arrow::io::RandomAccessFile> input;
    PARQUET_ASSIGN_OR_THROW(input, arrow::io::ReadableFile::Open("Schema_1_0.parquet"));

    // Open Parquet file reader
    std::unique_ptr<parquet::arrow::FileReader> arrow_reader;
    PARQUET_THROW_NOT_OK(parquet::arrow::OpenFile(input, pool, &arrow_reader));

    // Read entire file as a single Arrow table
    std::shared_ptr<arrow::Table> table;
    PARQUET_THROW_NOT_OK(arrow_reader->ReadTable(&table));

    ASSERT_EQ(table->schema()->field(0)->type().get()->name(), "int64");
    ASSERT_EQ(table->schema()->field(1)->type().get()->name(), "double");
    ASSERT_EQ(table->schema()->field(2)->type().get()->name(), "float");
    ASSERT_EQ(table->schema()->field(3)->type().get()->name(), "int32");
    ASSERT_EQ(table->schema()->field(4)->type().get()->name(), "int64");
    //ASSERT_EQ(table->schema()->field(5)->type().get()->name(), "int16");
    //ASSERT_EQ(table->schema()->field(6)->type().get()->name(), "int8");
}

TEST_F(FileWriterModuleTest, StoreOneSignal)
{
    auto ctx = CreateContext();
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("FileWriterModuleParquet", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals(ctx);
    fb.getInputPorts()[0].connect(doubleSignal);
    
    // Execute recording
    fb.setPropertyValue("RecordingActive", true);
    // Send Data
    sendSignalWithTime1<daq::SampleType::Float64>(10, 100, 1000);
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

TEST_F(FileWriterModuleTest, DownsamplingTest)
{
    auto ctx = CreateContext();
    const auto module = CreateModule();
    int downSamplingFactor = 100;

    auto fb = module.createFunctionBlock("FileWriterModuleParquet", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals(ctx);
    //fb.getInputPorts()[0].connect(int32Signal);
    fb.getInputPorts()[0].connect(floatSignal);

    
    // Execute recording
    fb.setPropertyValue("FileName", "Downsamling");
    fb.setPropertyValue("DownScalingDevider", downSamplingFactor);
    fb.setPropertyValue("RecordingActive", true);
    // Send Data
    //sendSignalWithTime1<daq::SampleType::Int32>(10, 1000, 1000);
    sendSignalWithTime1<daq::SampleType::Float32>(10, 1000, 1000);

    // Recording to false triggers file write
    fb.setPropertyValue("RecordingActive", false);

    std::shared_ptr<arrow::io::ReadableFile> infile;

    // Created Recorded files have a file format
    // <fileName>_x_y.parquet
    // where x is table where signals are sharing a domain signal
    // where y is subcount if file batching is done.
    PARQUET_ASSIGN_OR_THROW(infile, arrow::io::ReadableFile::Open("Downsamling_1_0.parquet"));

    parquet::StreamReader stream{parquet::ParquetFileReader::Open(infile)};
    
    size_t rowCount = 0;
    std::int64_t time = 0;
    std::int64_t checkTime = 999;
    //int32_t value = 0;
    //int32_t checkValue = -1;
    float value2 = 0;
    float checkValue2 = -1.0;
    while ( !stream.eof() )
    {
        stream >> time >> value2 >> parquet::EndRow;
        checkTime += downSamplingFactor;
        ASSERT_EQ(time, checkTime);
        //checkValue += downSamplingFactor;
        checkValue2 += downSamplingFactor;
        //ASSERT_EQ(value, checkValue);
        ASSERT_FLOAT_EQ(value2, checkValue2);

        rowCount++;
    }
    ASSERT_EQ(rowCount, 100);
}


TEST_F(FileWriterModuleTest, TestBatching)
{
    auto ctx = CreateContext();
    const auto module = CreateModule();

    auto fb = module.createFunctionBlock("FileWriterModuleParquet", nullptr, "id");
    ASSERT_TRUE(fb.assigned());

    createSignals(ctx);
    fb.getInputPorts()[0].connect(doubleSignal);
    
    // Execute recording and use other name for file
    fb.setPropertyValue("FileName", "openDAQData");
    fb.setPropertyValue("BatchCylce", 4);
    fb.setPropertyValue("RecordingActive", true);
    // Send Data
    sendSignalWithTime1<daq::SampleType::Float64>(4, 100, 2000);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    // Batch is written after the time is over. 
    sendSignalWithTime1<daq::SampleType::Float64>(6, 100, 2000, 4);

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