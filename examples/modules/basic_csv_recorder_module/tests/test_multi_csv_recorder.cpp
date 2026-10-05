#include <basic_csv_recorder_module/module_dll.h>
#include <coretypes/filesystem.h>
#include <opendaq/context_internal_ptr.h>
#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>
#include <testutils/memcheck_listener.h>

#include <chrono>
#include <fstream>
#include <thread>

using namespace daq;

static ModulePtr createModule(const ContextPtr& context)
{
    ModulePtr module;
    auto logger = Logger();
    createModule(&module, context);
    return module;
}

static ContextPtr createContext()
{
    const auto logger = Logger();
    return Context(Scheduler(logger), logger, TypeManager(), nullptr, nullptr);
}

// The recorder's reader evaluates on the scheduler, so statuses, port counts and written samples follow asynchronously
template <typename Condition>
static bool waitFor(Condition condition, int timeoutMs = 3000)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (!condition())
    {
        if (std::chrono::steady_clock::now() > deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return true;
}

static bool waitForStatus(const FunctionBlockPtr& fb, ComponentStatus status)
{
    return waitFor([&] { return fb.getStatusContainer().getStatus("ComponentStatus") == status; });
}

static bool waitForPortCount(const FunctionBlockPtr& fb, SizeT count)
{
    return waitFor([&] { return fb.getInputPorts().getCount() == count; });
}

// After the last connect the block still reconfigures once; data sent into that window is discarded by design
static bool settle(const FunctionBlockPtr& fb, SizeT portCount)
{
    const bool ok = waitForPortCount(fb, portCount) && waitForStatus(fb, ComponentStatus::Ok);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return ok;
}

class MultiCsvTest : public testing::Test
{
public:
    ModulePtr module;
    FunctionBlockPtr fb;
    ContextPtr context;

    DataDescriptorPtr validDescriptor;
    DataDescriptorPtr invalidDescriptor;
    DataDescriptorPtr timeDescriptor;

    ListPtr<ISignalConfig> validSignals;
    ListPtr<ISignalConfig> invalidSignals;
    SignalConfigPtr timeSignal;

    fs::path outputFolder;

protected:
    void SetUp() override
    {
        context = createContext();
        module = createModule(context);
        fb = module.createFunctionBlock("MultiCsvRecorder", nullptr, "fb");

        outputFolder = fs::path(testing::TempDir()) / "test_output";

        fb.setPropertyValue("Directory", outputFolder.string());
        fb.setPropertyValue("FileTimestampEnabled", false);

        invalidDescriptor = DataDescriptorBuilder().setSampleType(SampleType::ComplexFloat32).build();
        ReferenceDomainInfoPtr refDomainInfo =
            ReferenceDomainInfoBuilder().setReferenceDomainOffset(0).setReferenceTimeProtocol(TimeProtocol::Utc).build();
        timeDescriptor = DataDescriptorBuilder()
                             .setName("Time")
                             .setSampleType(SampleType::Int64)
                             .setTickResolution(Ratio(1, 1000))
                             .setOrigin("1970-01-01T00:00:00")
                             .setRule(LinearDataRule(1, 0))
                             .setUnit(Unit("s", -1, "seconds", "time"))
                             .setReferenceDomainInfo(refDomainInfo)
                             .build();

        validDescriptor =
            DataDescriptorBuilder().setName("value").setSampleType(SampleType::Float64).setUnit(Unit("V", -1, "volts", "voltage")).build();
        validSignals = List<ISignal>();
        invalidSignals = List<ISignal>();
        timeSignal = SignalWithDescriptor(context, timeDescriptor, nullptr, "time_sig");

        for (size_t i = 0; i < 10; ++i)
        {
            validSignals.pushBack(SignalWithDescriptor(context, validDescriptor, nullptr, fmt::format("sig{}", i)));
            invalidSignals.pushBack(SignalWithDescriptor(context, invalidDescriptor, nullptr, fmt::format("sig{}", i)));

            validSignals[i].setDomainSignal(timeSignal);
            invalidSignals[i].setDomainSignal(timeSignal);
        }
    }

    void TearDown() override
    {
        context.getScheduler().stop();
    }

    // Connects signal i to port i; every connect adds the next free port, so the ports are waited for
    void connect(SizeT port, const SignalConfigPtr& signal)
    {
        ASSERT_TRUE(waitForPortCount(fb, port + 1));
        fb.getInputPorts()[port].connect(signal);
    }

    void connectAll()
    {
        for (size_t i = 0; i < validSignals.getCount(); ++i)
            connect(i, validSignals[i]);
        ASSERT_TRUE(settle(fb, 11));
    }

    void sendData(SizeT sampleCount, SizeT offset, bool sendInvalid, std::pair<size_t, size_t> signalRange)
    {
        DataPacketPtr domainPacket = DataPacket(timeDescriptor, sampleCount, offset);

        timeSignal.sendPacket(domainPacket);
        for (size_t i = signalRange.first; i < signalRange.second; ++i)
        {
            DataPacketPtr packet = DataPacketWithDomain(domainPacket, validDescriptor, sampleCount);
            double* sumValueData = static_cast<double*>(packet.getRawData());
            for (size_t j = 0; j < sampleCount; ++j)
                sumValueData[j] = i + j - 0.13;
            validSignals[i].sendPacket(packet);
        }

        if (sendInvalid)
        {
            DataPacketPtr invalidValuePacket = DataPacketWithDomain(domainPacket, invalidDescriptor, sampleCount);
            for (size_t i = signalRange.first; i < signalRange.second; ++i)
                invalidSignals[i].sendPacket(invalidValuePacket);
        }
    }

    // The block reads on the scheduler and the writer flushes on its own thread
    static void letDataLand()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
};

TEST_F(MultiCsvTest, Create)
{
    ASSERT_TRUE(fb.assigned());
    EXPECT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);
}

TEST_F(MultiCsvTest, ConnectSignal)
{
    ASSERT_NO_THROW(fb.getInputPorts()[0].connect(validSignals[0]));
    EXPECT_TRUE(waitForStatus(fb, ComponentStatus::Ok));
    EXPECT_TRUE(waitForPortCount(fb, 2));
}

TEST_F(MultiCsvTest, ConnectSignals)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
    {
        ASSERT_NO_THROW(fb.getInputPorts()[0].connect(validSignals[i]));
    }

    EXPECT_TRUE(waitForStatus(fb, ComponentStatus::Ok));
}

TEST_F(MultiCsvTest, DisconnectSignals)
{
    connectAll();
    EXPECT_EQ(fb.getInputPorts().getCount(), 11u);

    for (const auto& ip : fb.getInputPorts())
        ip.disconnect();

    EXPECT_TRUE(waitForPortCount(fb, 1));
    EXPECT_TRUE(waitForStatus(fb, ComponentStatus::Warning));
}

TEST_F(MultiCsvTest, WriteSamples)
{
    // Remove the folder to:
    // a) test creation of missing folders
    // b) make sure the file without any serial number suffixes is the latest one.
    EXPECT_NO_THROW(fs::remove_all(outputFolder));

    connectAll();
    fb.asPtr<IRecorder>(true).startRecording();

    sendData(100, 817, false, std::make_pair(0, 10));
    letDataLand();

    fb.asPtr<IRecorder>(true).stopRecording();

    // Check the file contents
    std::ifstream readIn((this->outputFolder / "output.csv").string());

    ASSERT_TRUE(readIn.is_open());

    std::string line;
    std::getline(readIn, line);
    std::string headerLine("# domain;unit=seconds;resolution=1/1000;delta=1;origin=1970-01-01T00:00:00;starting_tick=817;");
    EXPECT_EQ(line, headerLine);

    std::getline(readIn, line);
    std::string columns("\"sig0 (V)\",\"sig1 (V)\",\"sig2 (V)\",\"sig3 (V)\",\"sig4 (V)\",\"sig5 (V)\",\"sig6 (V)\",\"sig7 (V)\",\"sig8 "
                        "(V)\",\"sig9 (V)\"");
    EXPECT_EQ(line, columns);

    std::getline(readIn, line);
    std::string firstSamples("-0.13,0.87,1.87,2.87,3.87,4.87,5.87,6.87,7.87,8.87");
    EXPECT_EQ(line, firstSamples);
}

TEST_F(MultiCsvTest, WriteSamplesWithDomain)
{
    EXPECT_NO_THROW(fs::remove_all(outputFolder));
    fb.setPropertyValue("WriteDomain", true);

    connectAll();
    fb.asPtr<IRecorder>(true).startRecording();

    sendData(100, 817, false, std::make_pair(0, 10));
    letDataLand();

    fb.asPtr<IRecorder>(true).stopRecording();

    // Check the file contents
    std::ifstream readIn((this->outputFolder / "output.csv").string());

    ASSERT_TRUE(readIn.is_open());

    std::string line;
    std::getline(readIn, line);
    std::string headerLine("# domain;unit=seconds;resolution=1/1000;delta=1;origin=1970-01-01T00:00:00;starting_tick=817;");
    EXPECT_EQ(line, headerLine);

    std::getline(readIn, line);
    std::string columns(
        "Domain,\"sig0 (V)\",\"sig1 (V)\",\"sig2 (V)\",\"sig3 (V)\",\"sig4 (V)\",\"sig5 (V)\",\"sig6 (V)\",\"sig7 (V)\",\"sig8 "
        "(V)\",\"sig9 (V)\"");
    EXPECT_EQ(line, columns);

    std::getline(readIn, line);
    std::string firstSamples("817,-0.13,0.87,1.87,2.87,3.87,4.87,5.87,6.87,7.87,8.87");
    EXPECT_EQ(line, firstSamples);
}

TEST_F(MultiCsvTest, DetectSampleRateDiff)
{
    fb.getInputPorts()[0].connect(validSignals[0]);
    ASSERT_TRUE(settle(fb, 2));
    fb.asPtr<IRecorder>(true).startRecording();
    EXPECT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Ok);

    sendData(100, 0, false, std::make_pair(0, 10));
    letDataLand();
    fb.asPtr<IRecorder>(true).stopRecording();

    DataDescriptorPtr halfRateTimeDescriptor = DataDescriptorBuilder()
                                                   .setName("Time")
                                                   .setSampleType(SampleType::Int64)
                                                   .setTickResolution(Ratio(1, 1000))
                                                   .setOrigin("1970-01-01T00:00:00")
                                                   .setRule(LinearDataRule(2, 0))
                                                   .setUnit(Unit("s", -1, "seconds", "time"))
                                                   .build();

    SignalConfigPtr halfRateTimeSignal = SignalWithDescriptor(context, halfRateTimeDescriptor, nullptr, fmt::format("halftime_sig"));
    SignalConfigPtr halfRateSignal = SignalWithDescriptor(context, validDescriptor, nullptr, fmt::format("halfrate_sig"));
    halfRateSignal.setDomainSignal(halfRateTimeSignal);

    // An input at another rate is set aside and reported; the recorder goes on with the compatible ones
    fb.getInputPorts()[1].connect(halfRateSignal);
    EXPECT_TRUE(waitForStatus(fb, ComponentStatus::Warning));

    SizeT sampleCount = 100;
    SizeT offset = 0;
    DataPacketPtr dPacket = DataPacket(halfRateTimeDescriptor, sampleCount / 4, offset / 4);
    DataPacketPtr vPacket = DataPacketWithDomain(dPacket, validDescriptor, sampleCount / 4);
    halfRateSignal.sendPacket(vPacket);
    letDataLand();
}

TEST_F(MultiCsvTest, DetectDescriptorChange)
{
    EXPECT_NO_THROW(fs::remove_all(outputFolder));
    fb.setPropertyValue("WriteDomain", true);

    connectAll();
    fb.asPtr<IRecorder>(true).startRecording();

    sendData(10, 817, false, std::make_pair(0, 10));
    letDataLand();

    ReferenceDomainInfoPtr rdInfo =
        ReferenceDomainInfoBuilder().setReferenceDomainOffset(0).setReferenceTimeProtocol(TimeProtocol::Utc).build();
    DataDescriptorPtr changedDescriptor = DataDescriptorBuilder()
                                              .setName("Time")
                                              .setSampleType(SampleType::Int64)
                                              .setTickResolution(Ratio(1, 1000))
                                              .setOrigin("1970-01-01T00:00:00")
                                              .setRule(LinearDataRule(4, 0))  // Delta changes to 4
                                              .setUnit(Unit("s", -1, "seconds", "time"))
                                              .setReferenceDomainInfo(rdInfo)
                                              .build();

    // The shared domain changes for every input at once: the main domain change opens a second file
    timeSignal.setDescriptor(changedDescriptor);
    letDataLand();

    sendData(10, 828, false, std::make_pair(0, 10));
    letDataLand();

    fb.asPtr<IRecorder>(true).stopRecording();

    // Check the file contents
    std::ifstream readIn((this->outputFolder / "output.csv").string());

    ASSERT_TRUE(readIn.is_open());

    std::string line;
    std::string reference;

    std::getline(readIn, line);
    reference = "# "
                "domain;unit=seconds;resolution=1/"
                "1000;delta=1;origin=1970-01-01T00:00:00;starting_tick=817;";
    EXPECT_EQ(line, reference);

    std::getline(readIn, line);
    const std::string columns = "Domain,\"sig0 (V)\",\"sig1 (V)\",\"sig2 (V)\",\"sig3 (V)\",\"sig4 (V)\",\"sig5 "
                                "(V)\",\"sig6 (V)\",\"sig7 (V)\",\"sig8 "
                                "(V)\",\"sig9 (V)\"";
    EXPECT_EQ(line, columns);

    std::getline(readIn, line);
    reference = "817,-0.13,0.87,1.87,2.87,3.87,4.87,5.87,6.87,7.87,8.87";
    EXPECT_EQ(line, reference);

    // After descriptor change a second file is created with different contents
    std::ifstream readIn2((this->outputFolder / "output_001.csv").string());
    ASSERT_TRUE(readIn2.is_open());

    std::getline(readIn2, line);
    reference = "# "
                "domain;unit=seconds;resolution=1/"
                "1000;delta=4;origin=1970-01-01T00:00:00;starting_tick=828;";
    EXPECT_EQ(line, reference);

    std::getline(readIn2, line);
    EXPECT_EQ(line, columns);

    std::getline(readIn2, line);
    reference = "828,-0.13,0.87,1.87,2.87,3.87,4.87,5.87,6.87,7.87,8.87";
    EXPECT_EQ(line, reference);

    std::getline(readIn2, line);
    reference = "832,0.87,1.87,2.87,3.87,4.87,5.87,6.87,7.87,8.87,9.87";
    EXPECT_EQ(line, reference);
}
