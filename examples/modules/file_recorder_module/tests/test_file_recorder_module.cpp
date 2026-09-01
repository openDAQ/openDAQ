#include <algorithm>
#include <chrono>
#include <numeric>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <testutils/testutils.h>

#include <coretypes/common.h>
#include <coretypes/errorinfo.h>
#include <coretypes/filesystem.h>
#include <opendaq/context_factory.h>
#include <opendaq/module_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/recorder.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/signal_factory.h>

#include <file_recorder_module/file_format.h>
#include <file_recorder_module/module_dll.h>
#include <file_recorder_module/version.h>

using namespace daq;
using namespace daq::modules::file_recorder_module;

namespace
{

constexpr auto RECORDER_ID = "FileRecorder";
constexpr auto PLAYER_ID = "FilePlayer";

ModulePtr createTestModule()
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto context = Context(scheduler, logger, TypeManager(), nullptr);

    ModulePtr module;
    createModule(&module, context);
    return module;
}

DataDescriptorPtr valueDescriptor()
{
    return DataDescriptorBuilder()
        .setName("TestSignal")
        .setSampleType(SampleType::Float64)
        .setRule(ExplicitDataRule())
        .setUnit(Unit("V", -1, "volts", "voltage"))
        .build();
}

DataDescriptorPtr linearDomainDescriptor(Int delta)
{
    return DataDescriptorBuilder()
        .setName("Time")
        .setSampleType(SampleType::Int64)
        .setRule(LinearDataRule(delta, 0))
        .setTickResolution(Ratio(1, 1000000))
        .setUnit(Unit("s", -1, "seconds", "time"))
        .setOrigin("1970-01-01T00:00:00Z")
        .build();
}

DataDescriptorPtr explicitDomainDescriptor()
{
    return DataDescriptorBuilder()
        .setName("Time")
        .setSampleType(SampleType::Int64)
        .setRule(ExplicitDataRule())
        .setTickResolution(Ratio(1, 1000000))
        .setUnit(Unit("s", -1, "seconds", "time"))
        .setOrigin("1970-01-01T00:00:00Z")
        .build();
}

/*!
 * Creates a standalone signal, with a domain signal described by @p domainDescriptor unless it is
 * unassigned, in which case the signal has no domain at all.
 */
SignalConfigPtr createSignal(const ContextPtr& context, const DataDescriptorPtr& domainDescriptor)
{
    auto signal = Signal(context, nullptr, "test_signal");
    signal.setDescriptor(valueDescriptor());

    if (domainDescriptor.assigned())
    {
        auto domainSignal = Signal(context, nullptr, "test_signal_domain");
        domainSignal.setDescriptor(domainDescriptor);
        signal.setDomainSignal(domainSignal);
    }

    return signal;
}

/*!
 * Sends @p count samples counting up from @p firstValue, with an implicit domain starting at
 * @p domainStart.
 */
void sendImplicitPacket(const SignalConfigPtr& signal, std::size_t count, double firstValue, Int domainStart)
{
    const auto domainPacket = DataPacket(signal.getDomainSignal().getDescriptor(), count, domainStart);
    auto packet = DataPacketWithDomain(domainPacket, signal.getDescriptor(), count);

    auto* data = static_cast<double*>(packet.getRawData());
    std::iota(data, data + count, firstValue);

    signal.getDomainSignal().asPtr<ISignalConfig>().sendPacket(domainPacket);
    signal.sendPacket(packet);
}

/*!
 * Sends one sample per entry of @p ticks, with the domain values given explicitly.
 */
void sendExplicitPacket(const SignalConfigPtr& signal, const std::vector<Int>& ticks, double firstValue)
{
    const auto count = ticks.size();

    auto domainPacket = DataPacket(signal.getDomainSignal().getDescriptor(), count);
    std::copy(ticks.begin(), ticks.end(), static_cast<Int*>(domainPacket.getRawData()));

    auto packet = DataPacketWithDomain(domainPacket, signal.getDescriptor(), count);
    auto* data = static_cast<double*>(packet.getRawData());
    std::iota(data, data + count, firstValue);

    signal.getDomainSignal().asPtr<ISignalConfig>().sendPacket(domainPacket);
    signal.sendPacket(packet);
}

/*!
 * Sends @p count samples with no domain packet at all.
 */
void sendPacketWithoutDomain(const SignalConfigPtr& signal, std::size_t count, double firstValue)
{
    auto packet = DataPacket(signal.getDescriptor(), count);

    auto* data = static_cast<double*>(packet.getRawData());
    std::iota(data, data + count, firstValue);

    signal.sendPacket(packet);
}

std::vector<fs::path> recordingsIn(const fs::path& directory)
{
    std::vector<fs::path> paths;

    if (fs::exists(directory))
        for (const auto& entry : fs::directory_iterator(directory))
            if (entry.is_regular_file() && entry.path().extension() == file_format::FILE_EXTENSION)
                paths.push_back(entry.path());

    std::sort(paths.begin(), paths.end());
    return paths;
}

/*!
 * A directory under the working directory which is emptied on construction and destruction, so
 * that a failing test does not leave recordings behind for the next one to find.
 */
class ScratchDirectory
{
public:
    explicit ScratchDirectory(const std::string& name)
        : path(fs::current_path() / name)
    {
        fs::remove_all(path);
        fs::create_directories(path);
    }

    ~ScratchDirectory()
    {
        std::error_code ignored;
        fs::remove_all(path, ignored);
    }

    const fs::path& get() const
    {
        return path;
    }

private:
    fs::path path;
};

/*!
 * Reads from @p signal until @p count samples have arrived or @p timeout elapses.
 */
std::size_t readSamples(const StreamReaderPtr& reader,
                        double* values,
                        Int* domain,
                        std::size_t count,
                        std::chrono::milliseconds timeout = std::chrono::seconds(5))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::size_t total = 0;

    while (total < count && std::chrono::steady_clock::now() < deadline)
    {
        SizeT toRead = count - total;
        reader.readWithDomain(values + total, domain + total, &toRead, 100);
        total += toRead;
    }

    return total;
}

}

using FileRecorderModuleTest = testing::Test;

TEST_F(FileRecorderModuleTest, CreateModule)
{
    IModule* module = nullptr;
    const ErrCode errCode = createModule(&module, NullContext());

    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));
    ASSERT_NE(module, nullptr);

    module->releaseRef();
}

TEST_F(FileRecorderModuleTest, ModuleName)
{
    ModulePtr module;
    createModule(&module, NullContext());

    ASSERT_EQ(module.getModuleInfo().getName(), "FileRecorderModule");
}

TEST_F(FileRecorderModuleTest, VersionCorrect)
{
    ModulePtr module;
    createModule(&module, NullContext());

    const auto version = module.getModuleInfo().getVersionInfo();

    ASSERT_EQ(version.getMajor(), FILE_RECORDER_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), FILE_RECORDER_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), FILE_RECORDER_MODULE_PATCH_VERSION);
}

TEST_F(FileRecorderModuleTest, AvailableFunctionBlockTypes)
{
    const auto types = createTestModule().getAvailableFunctionBlockTypes();

    ASSERT_TRUE(types.hasKey(RECORDER_ID));
    ASSERT_TRUE(types.hasKey(PLAYER_ID));
}

TEST_F(FileRecorderModuleTest, RecorderIsTaggedAsRecorder)
{
    const auto fb = createTestModule().createFunctionBlock(RECORDER_ID, nullptr, "fb");

    ASSERT_TRUE(fb.getTags().contains("Recorder"));
    ASSERT_TRUE(fb.asPtrOrNull<IRecorder>().assigned());
}

TEST_F(FileRecorderModuleTest, KeepsExactlyOneFreeInputPort)
{
    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");
    const auto signal = createSignal(fb.getContext(), linearDomainDescriptor(1000));

    ASSERT_EQ(fb.getInputPorts().getCount(), 1u);

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    ASSERT_EQ(fb.getInputPorts().getCount(), 2u);

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().disconnect();
    ASSERT_EQ(fb.getInputPorts().getCount(), 1u);
}

TEST_F(FileRecorderModuleTest, RecordsIntoConfiguredDirectory)
{
    const ScratchDirectory scratch("file_recorder_records");

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");
    const auto signal = createSignal(fb.getContext(), linearDomainDescriptor(1000));

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    fb.setPropertyValue("Path", scratch.get().string());

    const auto recorder = fb.asPtr<IRecorder>(true);
    ASSERT_EQ(recorder->startRecording(), OPENDAQ_SUCCESS);

    Bool isRecording = False;
    recorder->getIsRecording(&isRecording);
    ASSERT_TRUE(isRecording);

    for (int i = 0; i < 10; ++i)
        sendImplicitPacket(signal, 100, i * 100.0, i * 100 * 1000);

    ASSERT_EQ(recorder->stopRecording(), OPENDAQ_SUCCESS);

    recorder->getIsRecording(&isRecording);
    ASSERT_FALSE(isRecording);

    ASSERT_EQ(recordingsIn(scratch.get()).size(), 1u);
}

TEST_F(FileRecorderModuleTest, RejectsRedundantStartAndStop)
{
    const ScratchDirectory scratch("file_recorder_redundant");

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");
    const auto signal = createSignal(fb.getContext(), linearDomainDescriptor(1000));

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    fb.setPropertyValue("Path", scratch.get().string());

    const auto recorder = fb.asPtr<IRecorder>(true);

    // The rejected calls attach error info, which the test listener reports as a leak unless it
    // is consumed. Nothing here is going to read it, so clear it after each expected failure.
    ASSERT_EQ(recorder->stopRecording(), OPENDAQ_ERR_INVALIDSTATE);
    daqClearErrorInfo();

    ASSERT_EQ(recorder->startRecording(), OPENDAQ_SUCCESS);
    ASSERT_EQ(recorder->startRecording(), OPENDAQ_ERR_INVALIDSTATE);
    daqClearErrorInfo();

    sendImplicitPacket(signal, 100, 0.0, 0);

    ASSERT_EQ(recorder->stopRecording(), OPENDAQ_SUCCESS);
    ASSERT_EQ(recorder->stopRecording(), OPENDAQ_ERR_INVALIDSTATE);
    daqClearErrorInfo();

    ASSERT_EQ(recordingsIn(scratch.get()).size(), 1u);
}

TEST_F(FileRecorderModuleTest, RollsOverWhenMaxFileSizeReached)
{
    const ScratchDirectory scratch("file_recorder_rollover");

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");
    const auto signal = createSignal(fb.getContext(), linearDomainDescriptor(1000));

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    fb.setPropertyValue("Path", scratch.get().string());
    fb.setPropertyValue("MaxFileSizeMB", 1);

    const auto recorder = fb.asPtr<IRecorder>(true);
    recorder->startRecording();

    // 400k Float64 samples is a little over 3 MiB of value data, so the 1 MiB limit is crossed
    // several times.
    constexpr std::size_t packetSamples = 10000;
    for (std::size_t i = 0; i < 40; ++i)
        sendImplicitPacket(signal, packetSamples, static_cast<double>(i * packetSamples), static_cast<Int>(i * packetSamples * 1000));

    recorder->stopRecording();

    const auto files = recordingsIn(scratch.get());
    ASSERT_GE(files.size(), 3u);

    for (const auto& file : files)
        ASSERT_GT(fs::file_size(file), 0u);
}

TEST_F(FileRecorderModuleTest, RecordsSignalWithoutDomainSignal)
{
    const ScratchDirectory scratch("file_recorder_no_domain");

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");
    const auto signal = createSignal(fb.getContext(), nullptr);

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    fb.setPropertyValue("Path", scratch.get().string());

    const auto recorder = fb.asPtr<IRecorder>(true);
    recorder->startRecording();

    for (int i = 0; i < 5; ++i)
        sendPacketWithoutDomain(signal, 100, i * 100.0);

    recorder->stopRecording();

    ASSERT_EQ(recordingsIn(scratch.get()).size(), 1u);
}

TEST_F(FileRecorderModuleTest, RoundTripAtFixedSampleRate)
{
    const ScratchDirectory scratch("file_recorder_roundtrip");
    constexpr std::size_t sampleCount = 500;

    const auto module = createTestModule();

    const auto recorderFb = module.createFunctionBlock(RECORDER_ID, nullptr, "recorder");
    const auto signal = createSignal(recorderFb.getContext(), linearDomainDescriptor(1000));

    recorderFb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    recorderFb.setPropertyValue("Path", scratch.get().string());

    const auto recorder = recorderFb.asPtr<IRecorder>(true);
    recorder->startRecording();
    sendImplicitPacket(signal, sampleCount, 0.0, 0);
    recorder->stopRecording();

    const auto files = recordingsIn(scratch.get());
    ASSERT_EQ(files.size(), 1u);

    const auto playerFb = module.createFunctionBlock(PLAYER_ID, nullptr, "player");
    playerFb.setPropertyValue("FilePath", files.front().string());
    playerFb.setPropertyValue("PlaybackMode", 0);
    playerFb.setPropertyValue("SampleRate", 100000.0);

    const auto reader = StreamReader(playerFb.getSignals().getItemAt(0), SampleType::Float64, SampleType::Int64);

    ProcedurePtr(playerFb.getPropertyValue("StartPlayback"))();

    std::vector<double> values(sampleCount);
    std::vector<Int> domain(sampleCount);
    const auto read = readSamples(reader, values.data(), domain.data(), sampleCount);

    ProcedurePtr(playerFb.getPropertyValue("StopPlayback"))();

    ASSERT_EQ(read, sampleCount);
    for (std::size_t i = 0; i < sampleCount; ++i)
        ASSERT_DOUBLE_EQ(values[i], static_cast<double>(i)) << "at sample " << i;
}

TEST_F(FileRecorderModuleTest, ReplayPreservesRecordedDomainGaps)
{
    const ScratchDirectory scratch("file_recorder_gaps");

    // Three samples 100 ms apart, then a 400 ms gap before the last two.
    const std::vector<Int> ticks{0, 100000, 200000, 600000, 700000};

    const auto module = createTestModule();

    const auto recorderFb = module.createFunctionBlock(RECORDER_ID, nullptr, "recorder");
    const auto signal = createSignal(recorderFb.getContext(), explicitDomainDescriptor());

    recorderFb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    recorderFb.setPropertyValue("Path", scratch.get().string());

    const auto recorder = recorderFb.asPtr<IRecorder>(true);
    recorder->startRecording();
    sendExplicitPacket(signal, ticks, 0.0);
    recorder->stopRecording();

    const auto files = recordingsIn(scratch.get());
    ASSERT_EQ(files.size(), 1u);

    const auto playerFb = module.createFunctionBlock(PLAYER_ID, nullptr, "player");
    playerFb.setPropertyValue("FilePath", files.front().string());
    playerFb.setPropertyValue("PlaybackMode", 1);
    playerFb.setPropertyValue("ShiftDomainToNow", True);

    const auto reader = StreamReader(playerFb.getSignals().getItemAt(0), SampleType::Float64, SampleType::Int64);

    const auto started = std::chrono::steady_clock::now();
    ProcedurePtr(playerFb.getPropertyValue("StartPlayback"))();

    std::vector<double> values(ticks.size());
    std::vector<Int> domain(ticks.size());
    const auto read = readSamples(reader, values.data(), domain.data(), ticks.size());

    const auto elapsed = std::chrono::steady_clock::now() - started;
    ProcedurePtr(playerFb.getPropertyValue("StopPlayback"))();

    ASSERT_EQ(read, ticks.size());

    // The domain values are rebased to start at zero, but their spacing, gap included, is the
    // recorded one.
    for (std::size_t i = 0; i < ticks.size(); ++i)
        ASSERT_EQ(domain[i], ticks[i] - ticks.front()) << "at sample " << i;

    for (std::size_t i = 0; i < ticks.size(); ++i)
        ASSERT_DOUBLE_EQ(values[i], static_cast<double>(i)) << "at sample " << i;

    // Replaying 700 ms of recording honors the pauses, so it cannot have finished sooner.
    ASSERT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 600);
}

TEST_F(FileRecorderModuleTest, ReplaysImplicitDomainAtRecordedRate)
{
    const ScratchDirectory scratch("file_recorder_implicit_replay");

    // 40 samples one millisecond apart: more than one output packet interval, so the run is also
    // split on its way out.
    constexpr Int delta = 1000;
    constexpr std::size_t sampleCount = 40;

    const auto module = createTestModule();

    const auto recorderFb = module.createFunctionBlock(RECORDER_ID, nullptr, "recorder");
    const auto signal = createSignal(recorderFb.getContext(), linearDomainDescriptor(delta));

    recorderFb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    recorderFb.setPropertyValue("Path", scratch.get().string());

    const auto recorder = recorderFb.asPtr<IRecorder>(true);
    recorder->startRecording();
    sendImplicitPacket(signal, sampleCount, 0.0, 5000);
    recorder->stopRecording();

    const auto files = recordingsIn(scratch.get());
    ASSERT_EQ(files.size(), 1u);

    const auto playerFb = module.createFunctionBlock(PLAYER_ID, nullptr, "player");
    playerFb.setPropertyValue("FilePath", files.front().string());
    playerFb.setPropertyValue("PlaybackMode", 1);
    playerFb.setPropertyValue("ShiftDomainToNow", True);

    const auto reader = StreamReader(playerFb.getSignals().getItemAt(0), SampleType::Float64, SampleType::Int64);

    ProcedurePtr(playerFb.getPropertyValue("StartPlayback"))();

    std::vector<double> values(sampleCount);
    std::vector<Int> domain(sampleCount);
    const auto read = readSamples(reader, values.data(), domain.data(), sampleCount);

    ProcedurePtr(playerFb.getPropertyValue("StopPlayback"))();

    ASSERT_EQ(read, sampleCount);

    // Rebased to zero, but keeping the recorded spacing rather than the recorded absolute start.
    for (std::size_t i = 0; i < sampleCount; ++i)
    {
        ASSERT_EQ(domain[i], static_cast<Int>(i) * delta) << "at sample " << i;
        ASSERT_DOUBLE_EQ(values[i], static_cast<double>(i)) << "at sample " << i;
    }
}

TEST_F(FileRecorderModuleTest, LoopContinuesDomainForward)
{
    const ScratchDirectory scratch("file_recorder_loop");

    // Three samples 100 ms apart, so one pass spans 200 ms and the next should start at 300 ms.
    const std::vector<Int> ticks{0, 100000, 200000};

    const auto module = createTestModule();

    const auto recorderFb = module.createFunctionBlock(RECORDER_ID, nullptr, "recorder");
    const auto signal = createSignal(recorderFb.getContext(), explicitDomainDescriptor());

    recorderFb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    recorderFb.setPropertyValue("Path", scratch.get().string());

    const auto recorder = recorderFb.asPtr<IRecorder>(true);
    recorder->startRecording();
    sendExplicitPacket(signal, ticks, 0.0);
    recorder->stopRecording();

    const auto files = recordingsIn(scratch.get());
    ASSERT_EQ(files.size(), 1u);

    const auto playerFb = module.createFunctionBlock(PLAYER_ID, nullptr, "player");
    playerFb.setPropertyValue("FilePath", files.front().string());
    playerFb.setPropertyValue("PlaybackMode", 1);
    playerFb.setPropertyValue("ShiftDomainToNow", True);
    playerFb.setPropertyValue("Loop", True);

    const auto reader = StreamReader(playerFb.getSignals().getItemAt(0), SampleType::Float64, SampleType::Int64);

    ProcedurePtr(playerFb.getPropertyValue("StartPlayback"))();

    constexpr std::size_t expected = 6;
    std::vector<double> values(expected);
    std::vector<Int> domain(expected);
    const auto read = readSamples(reader, values.data(), domain.data(), expected);

    ProcedurePtr(playerFb.getPropertyValue("StopPlayback"))();

    ASSERT_EQ(read, expected);

    // The second pass carries on from where the first ended rather than jumping back to zero.
    const std::vector<Int> expectedDomain{0, 100000, 200000, 300000, 400000, 500000};
    for (std::size_t i = 0; i < expected; ++i)
    {
        ASSERT_EQ(domain[i], expectedDomain[i]) << "at sample " << i;
        ASSERT_DOUBLE_EQ(values[i], static_cast<double>(i % ticks.size())) << "at sample " << i;
    }
}

TEST_F(FileRecorderModuleTest, ReplayReportsMissingFile)
{
    const auto module = createTestModule();
    const auto playerFb = module.createFunctionBlock(PLAYER_ID, nullptr, "player");

    playerFb.setPropertyValue("FilePath", (fs::current_path() / "no_such_recording.daqrec").string());
    ProcedurePtr(playerFb.getPropertyValue("StartPlayback"))();

    ASSERT_EQ(playerFb.getStatusContainer().getStatus("ComponentStatus"), "Error");
}
