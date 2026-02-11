#include <basic_csv_recorder_module/module_dll.h>
#include <coretypes/filesystem.h>
#include <opendaq/context_internal_ptr.h>
#include <opendaq/instance_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/opendaq.h>
#include <testutils/memcheck_listener.h>

#include <fstream>

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
        // Create module

        context = createContext();
        module = createModule(context);

        auto config = module.getAvailableFunctionBlockTypes().get("MultiCsvRecorder").createDefaultConfig();
        config.setPropertyValue("ReaderNotificationMode", static_cast<Int>(PacketReadyNotification::SameThread));

        // Create function block
        fb = module.createFunctionBlock("MultiCsvRecorder", nullptr, "fb", config);

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
};

TEST_F(MultiCsvTest, Create)
{
    ASSERT_TRUE(fb.assigned());
    EXPECT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);
}

TEST_F(MultiCsvTest, ConnectSignal)
{
    ASSERT_NO_THROW(fb.getInputPorts()[0].connect(validSignals[0]));
    EXPECT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Ok);
}

TEST_F(MultiCsvTest, ConnectSignals)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
    {
        ASSERT_NO_THROW(fb.getInputPorts()[0].connect(validSignals[i]));
    }

    EXPECT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Ok);
}

TEST_F(MultiCsvTest, DisconnectSignals)
{
    for (size_t i = 0; i < validSignals.getCount(); ++i)
        fb.getInputPorts()[i].connect(validSignals[i]);

    EXPECT_EQ(fb.getInputPorts().getCount(), 11u);

    for (const auto& ip : fb.getInputPorts())
        ip.disconnect();

    EXPECT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);
    EXPECT_EQ(fb.getInputPorts().getCount(), 1u);
}

TEST_F(MultiCsvTest, WriteSamples)
{
    // Remove the folder to:
    // a) test creation of missing folders
    // b) make sure the file without any serial number suffixes is the latest one.
    EXPECT_NO_THROW(fs::remove_all(outputFolder));

    for (size_t i = 0; i < validSignals.getCount(); ++i)
        fb.getInputPorts()[i].connect(validSignals[i]);

    fb.asPtr<IRecorder>(true).startRecording();

    sendData(100, 817, false, std::make_pair(0, 10));

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

    for (size_t i = 0; i < validSignals.getCount(); ++i)
        fb.getInputPorts()[i].connect(validSignals[i]);

    fb.asPtr<IRecorder>(true).startRecording();

    sendData(100, 817, false, std::make_pair(0, 10));

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
    fb.asPtr<IRecorder>(true).startRecording();

    EXPECT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Ok);

    sendData(100, 0, false, std::make_pair(0, 10));
    fb.asPtr<IRecorder>(true).stopRecording();

    DataDescriptorPtr halfRateTimeDescriptor;
    SignalConfigPtr halfRateTimeSignal;
    SignalConfigPtr halfRateSignal;

    halfRateTimeDescriptor = DataDescriptorBuilder()
                                 .setName("Time")
                                 .setSampleType(SampleType::Int64)
                                 .setTickResolution(Ratio(1, 1000))
                                 .setOrigin("1970-01-01T00:00:00")
                                 .setRule(LinearDataRule(2, 0))
                                 .setUnit(Unit("s", -1, "seconds", "time"))
                                 .build();

    halfRateTimeSignal = SignalWithDescriptor(context, halfRateTimeDescriptor, nullptr, fmt::format("halftime_sig"));
    halfRateSignal = SignalWithDescriptor(context, validDescriptor, nullptr, fmt::format("halfrate_sig"));
    halfRateSignal.setDomainSignal(halfRateTimeSignal);

    fb.getInputPorts()[1].connect(halfRateSignal);
    EXPECT_EQ(fb.getStatusContainer().getStatus("ComponentStatus"), ComponentStatus::Warning);

    SizeT sampleCount = 100;
    SizeT offset = 0;
    DataPacketPtr dPacket = DataPacket(halfRateTimeDescriptor, sampleCount / 4, offset / 4);
    DataPacketPtr vPacket = DataPacketWithDomain(dPacket, validDescriptor, sampleCount / 4);
    halfRateSignal.sendPacket(vPacket);
}

TEST_F(MultiCsvTest, DetectDescriptorChange)
{
    EXPECT_NO_THROW(fs::remove_all(outputFolder));
    fb.setPropertyValue("WriteDomain", true);

    for (size_t i = 0; i < validSignals.getCount(); ++i)
        fb.getInputPorts()[i].connect(validSignals[i]);

    fb.asPtr<IRecorder>(true).startRecording();

    sendData(7, 817, false, std::make_pair(0, 10));

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

    timeSignal.setDescriptor(changedDescriptor);

    sendData(10, 828, false, std::make_pair(0, 10));

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