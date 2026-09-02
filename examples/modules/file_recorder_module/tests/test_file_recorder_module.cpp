#include <algorithm>
#include <chrono>
#include <numeric>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <testutils/testutils.h>

#include <coretypes/common.h>
#include <coretypes/errorinfo.h>
#include <coretypes/exceptions.h>
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

/*!
 * Waits until the recorder reports that it is no longer recording, and returns whether it did so
 * within @p timeout.
 */
bool waitUntilStopped(const ObjectPtr<IRecorder>& recorder, std::chrono::milliseconds timeout = std::chrono::seconds(5))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline)
    {
        Bool isRecording = False;
        recorder->getIsRecording(&isRecording);

        if (!isRecording)
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return false;
}

/*!
 * Waits until @p component reports a status message containing @p substring, and returns whether
 * it did so within @p timeout.
 */
bool waitUntilStatus(const ComponentPtr& component,
                     const std::string& substring,
                     std::chrono::milliseconds timeout = std::chrono::seconds(5))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline)
    {
        const std::string message = component.getStatusContainer().getStatusMessage("ComponentStatus").toStdString();
        if (message.find(substring) != std::string::npos)
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return false;
}

/*!
 * Asserts that a recording which ended on its own left the function block exactly as an explicit
 * stop would: the reason reported as the component's status, unlocked properties, and a new
 * recording which starts without having to be stopped first.
 */
void expectStoppedLikeAnExplicitStop(const FunctionBlockPtr& fb,
                                     const ObjectPtr<IRecorder>& recorder,
                                     const std::string& reason)
{
    Bool isRecording = True;
    recorder->getIsRecording(&isRecording);
    EXPECT_FALSE(isRecording);

    const std::string status = fb.getStatusContainer().getStatusMessage("ComponentStatus").toStdString();
    EXPECT_THAT(status, testing::HasSubstr("stopped"));
    EXPECT_THAT(status, testing::HasSubstr(reason));

    EXPECT_NO_THROW(fb.setPropertyValue("MaxFileSizeMB", 7));

    EXPECT_EQ(recorder->startRecording(), OPENDAQ_SUCCESS);
    EXPECT_EQ(recorder->stopRecording(), OPENDAQ_SUCCESS);
}

/*!
 * Replays @p file and returns the samples it produced. One more sample than @p expected is asked
 * for, so that a recording holding more than it should is detected rather than truncated.
 */
std::vector<double> replayRecording(const ModulePtr& module, const fs::path& file, std::size_t expected)
{
    const auto playerFb = module.createFunctionBlock(PLAYER_ID, nullptr, "player");
    playerFb.setPropertyValue("FilePath", file.string());
    playerFb.setPropertyValue("PlaybackMode", 0);
    playerFb.setPropertyValue("SampleRate", 100000.0);

    const auto reader = StreamReader(playerFb.getSignals().getItemAt(0), SampleType::Float64, SampleType::Int64);

    ProcedurePtr(playerFb.getPropertyValue("StartPlayback"))();

    std::vector<double> values(expected + 1);
    std::vector<Int> domain(expected + 1);
    const auto read = readSamples(reader, values.data(), domain.data(), expected + 1, std::chrono::seconds(1));

    ProcedurePtr(playerFb.getPropertyValue("StopPlayback"))();

    values.resize(read);
    return values;
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

TEST_F(FileRecorderModuleTest, PropertiesAreReadOnlyWhileRecording)
{
    const ScratchDirectory scratch("file_recorder_readonly");

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");
    const auto signal = createSignal(fb.getContext(), linearDomainDescriptor(1000));

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    fb.setPropertyValue("Path", scratch.get().string());

    const auto recorder = fb.asPtr<IRecorder>(true);
    recorder->startRecording();

    ASSERT_THROW(fb.setPropertyValue("Path", (scratch.get() / "elsewhere").string()), AccessDeniedException);
    daqClearErrorInfo();
    ASSERT_THROW(fb.setPropertyValue("MaxFileSizeMB", 1), AccessDeniedException);
    daqClearErrorInfo();
    ASSERT_THROW(fb.setPropertyValue("RecordingMode", 1), AccessDeniedException);
    daqClearErrorInfo();
    ASSERT_THROW(fb.setPropertyValue("SampleCount", 10), AccessDeniedException);
    daqClearErrorInfo();
    ASSERT_THROW(fb.setPropertyValue("DurationSeconds", 1.0), AccessDeniedException);
    daqClearErrorInfo();

    ASSERT_EQ(fb.getPropertyValue("Path"), scratch.get().string());

    recorder->stopRecording();

    // Settable again once the recording is over.
    ASSERT_NO_THROW(fb.setPropertyValue("MaxFileSizeMB", 1));
    ASSERT_EQ(fb.getPropertyValue("MaxFileSizeMB"), 1);
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

TEST_F(FileRecorderModuleTest, StopsAfterConfiguredSampleCount)
{
    const ScratchDirectory scratch("file_recorder_sample_count");
    constexpr std::size_t limit = 250;

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");
    const auto signal = createSignal(fb.getContext(), linearDomainDescriptor(1000));

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    fb.setPropertyValue("Path", scratch.get().string());
    fb.setPropertyValue("RecordingMode", 1);
    fb.setPropertyValue("SampleCount", static_cast<Int>(limit));

    const auto recorder = fb.asPtr<IRecorder>(true);
    recorder->startRecording();

    // Twice as many samples as asked for, sent in packets which do not line up with the limit,
    // so that the packet crossing it has to be cut in half.
    for (std::size_t i = 0; i < 5; ++i)
        sendImplicitPacket(signal, 100, static_cast<double>(i * 100), static_cast<Int>(i * 100 * 1000));

    ASSERT_TRUE(waitUntilStopped(recorder));

    const auto files = recordingsIn(scratch.get());
    ASSERT_EQ(files.size(), 1u);

    const auto values = replayRecording(module, files.front(), limit);
    ASSERT_EQ(values.size(), limit);

    for (std::size_t i = 0; i < values.size(); ++i)
        ASSERT_DOUBLE_EQ(values[i], static_cast<double>(i)) << "at sample " << i;
}

TEST_F(FileRecorderModuleTest, SampleCountModeWaitsForEverySignal)
{
    const ScratchDirectory scratch("file_recorder_sample_count_signals");
    constexpr std::size_t limit = 100;

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");

    const auto first = createSignal(fb.getContext(), linearDomainDescriptor(1000));
    const auto second = createSignal(fb.getContext(), linearDomainDescriptor(1000));

    // The signals are recorded into files named after them, so they need different names.
    first.setName("first_signal");
    second.setName("second_signal");

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(first);
    fb.getInputPorts().getItemAt(1).asPtr<IInputPortConfig>().connect(second);

    fb.setPropertyValue("Path", scratch.get().string());
    fb.setPropertyValue("RecordingMode", 1);
    fb.setPropertyValue("SampleCount", static_cast<Int>(limit));

    const auto recorder = fb.asPtr<IRecorder>(true);
    recorder->startRecording();

    sendImplicitPacket(first, limit, 0.0, 0);

    // The limit is per signal, so the recording waits for the one which has not reached it.
    ASSERT_FALSE(waitUntilStopped(recorder, std::chrono::milliseconds(300)));

    sendImplicitPacket(second, limit, 0.0, 0);

    ASSERT_TRUE(waitUntilStopped(recorder));
    ASSERT_EQ(recordingsIn(scratch.get()).size(), 2u);
}

TEST_F(FileRecorderModuleTest, StopsAfterConfiguredDuration)
{
    const ScratchDirectory scratch("file_recorder_duration");
    constexpr std::size_t sampleCount = 100;

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");
    const auto signal = createSignal(fb.getContext(), linearDomainDescriptor(1000));

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    fb.setPropertyValue("Path", scratch.get().string());
    fb.setPropertyValue("RecordingMode", 2);
    fb.setPropertyValue("DurationSeconds", 0.3);

    const auto recorder = fb.asPtr<IRecorder>(true);
    const auto started = std::chrono::steady_clock::now();
    recorder->startRecording();

    sendImplicitPacket(signal, sampleCount, 0.0, 0);

    ASSERT_TRUE(waitUntilStopped(recorder));
    ASSERT_GE(std::chrono::steady_clock::now() - started, std::chrono::milliseconds(300));

    // Anything arriving after the recording ended is dropped rather than appended.
    sendImplicitPacket(signal, sampleCount, static_cast<double>(sampleCount), static_cast<Int>(sampleCount * 1000));

    const auto files = recordingsIn(scratch.get());
    ASSERT_EQ(files.size(), 1u);
    ASSERT_EQ(replayRecording(module, files.front(), sampleCount).size(), sampleCount);
}

TEST_F(FileRecorderModuleTest, TimedRecordingEndsLikeAnExplicitStop)
{
    const ScratchDirectory scratch("file_recorder_duration_state");

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");
    const auto signal = createSignal(fb.getContext(), linearDomainDescriptor(1000));

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    fb.setPropertyValue("Path", scratch.get().string());
    fb.setPropertyValue("RecordingMode", 2);
    fb.setPropertyValue("DurationSeconds", 0.2);

    const auto recorder = fb.asPtr<IRecorder>(true);
    recorder->startRecording();
    sendImplicitPacket(signal, 100, 0.0, 0);

    ASSERT_TRUE(waitUntilStopped(recorder));

    expectStoppedLikeAnExplicitStop(fb, recorder, "duration");
}

TEST_F(FileRecorderModuleTest, SampleCountRecordingEndsLikeAnExplicitStop)
{
    const ScratchDirectory scratch("file_recorder_sample_count_state");

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");
    const auto signal = createSignal(fb.getContext(), linearDomainDescriptor(1000));

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    fb.setPropertyValue("Path", scratch.get().string());
    fb.setPropertyValue("RecordingMode", 1);
    fb.setPropertyValue("SampleCount", 100);

    const auto recorder = fb.asPtr<IRecorder>(true);
    recorder->startRecording();
    sendImplicitPacket(signal, 100, 0.0, 0);

    ASSERT_TRUE(waitUntilStopped(recorder));

    expectStoppedLikeAnExplicitStop(fb, recorder, "samples");
}

TEST_F(FileRecorderModuleTest, ManualModeIgnoresTheLimits)
{
    const ScratchDirectory scratch("file_recorder_manual");
    constexpr std::size_t sampleCount = 500;

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");
    const auto signal = createSignal(fb.getContext(), linearDomainDescriptor(1000));

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    fb.setPropertyValue("Path", scratch.get().string());
    fb.setPropertyValue("RecordingMode", 0);
    fb.setPropertyValue("SampleCount", 10);
    fb.setPropertyValue("DurationSeconds", 0.05);

    const auto recorder = fb.asPtr<IRecorder>(true);
    recorder->startRecording();

    sendImplicitPacket(signal, sampleCount, 0.0, 0);

    ASSERT_FALSE(waitUntilStopped(recorder, std::chrono::milliseconds(300)));

    recorder->stopRecording();

    const auto files = recordingsIn(scratch.get());
    ASSERT_EQ(files.size(), 1u);
    ASSERT_EQ(replayRecording(module, files.front(), sampleCount).size(), sampleCount);
}

TEST_F(FileRecorderModuleTest, StopEndsATimedRecordingEarly)
{
    const ScratchDirectory scratch("file_recorder_duration_stop");

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");
    const auto signal = createSignal(fb.getContext(), linearDomainDescriptor(1000));

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    fb.setPropertyValue("Path", scratch.get().string());
    fb.setPropertyValue("RecordingMode", 2);
    fb.setPropertyValue("DurationSeconds", 30.0);

    const auto recorder = fb.asPtr<IRecorder>(true);
    recorder->startRecording();

    sendImplicitPacket(signal, 100, 0.0, 0);

    // The explicit stop returns without waiting for the deadline, and the properties unlock.
    ASSERT_EQ(recorder->stopRecording(), OPENDAQ_SUCCESS);

    Bool isRecording = True;
    recorder->getIsRecording(&isRecording);
    ASSERT_FALSE(isRecording);

    ASSERT_NO_THROW(fb.setPropertyValue("RecordingMode", 0));

    // A recording started again afterwards is unaffected by the abandoned deadline.
    ASSERT_EQ(recorder->startRecording(), OPENDAQ_SUCCESS);
    ASSERT_FALSE(waitUntilStopped(recorder, std::chrono::milliseconds(200)));
    ASSERT_EQ(recorder->stopRecording(), OPENDAQ_SUCCESS);
}

TEST_F(FileRecorderModuleTest, SignalsSharingANameGetSeparateFiles)
{
    const ScratchDirectory scratch("file_recorder_same_name");
    constexpr std::size_t sampleCount = 100;

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");

    // Two signals of one recording may well carry the same name, and neither may lose its data
    // to the other.
    const auto first = createSignal(fb.getContext(), linearDomainDescriptor(1000));
    const auto second = createSignal(fb.getContext(), linearDomainDescriptor(1000));
    ASSERT_EQ(first.getName(), second.getName());

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(first);
    fb.getInputPorts().getItemAt(1).asPtr<IInputPortConfig>().connect(second);
    fb.setPropertyValue("Path", scratch.get().string());

    const auto recorder = fb.asPtr<IRecorder>(true);
    recorder->startRecording();

    sendImplicitPacket(first, sampleCount, 0.0, 0);
    sendImplicitPacket(second, sampleCount, 1000.0, 0);

    recorder->stopRecording();

    const auto files = recordingsIn(scratch.get());
    ASSERT_EQ(files.size(), 2u);

    const auto firstValues = replayRecording(module, files.front(), sampleCount);
    const auto secondValues = replayRecording(module, files.back(), sampleCount);

    ASSERT_EQ(firstValues.size(), sampleCount);
    ASSERT_EQ(secondValues.size(), sampleCount);
    EXPECT_DOUBLE_EQ(firstValues.front(), 0.0);
    EXPECT_DOUBLE_EQ(secondValues.front(), 1000.0);
}

TEST_F(FileRecorderModuleTest, RestartedRecordingKeepsTheEarlierFiles)
{
    const ScratchDirectory scratch("file_recorder_restart");
    constexpr std::size_t sampleCount = 100;

    const auto module = createTestModule();
    const auto fb = module.createFunctionBlock(RECORDER_ID, nullptr, "fb");
    const auto signal = createSignal(fb.getContext(), linearDomainDescriptor(1000));

    fb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    fb.setPropertyValue("Path", scratch.get().string());

    const auto recorder = fb.asPtr<IRecorder>(true);

    // Both recordings fall in the same second, so they would share a filename were the stem
    // built from the timestamp alone.
    recorder->startRecording();
    sendImplicitPacket(signal, sampleCount, 0.0, 0);
    recorder->stopRecording();

    recorder->startRecording();
    sendImplicitPacket(signal, sampleCount, 1000.0, 0);
    recorder->stopRecording();

    const auto files = recordingsIn(scratch.get());
    ASSERT_EQ(files.size(), 2u);

    const auto firstValues = replayRecording(module, files.front(), sampleCount);
    const auto secondValues = replayRecording(module, files.back(), sampleCount);

    ASSERT_EQ(firstValues.size(), sampleCount);
    ASSERT_EQ(secondValues.size(), sampleCount);
    EXPECT_DOUBLE_EQ(firstValues.front(), 0.0);
    EXPECT_DOUBLE_EQ(secondValues.front(), 1000.0);
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

TEST_F(FileRecorderModuleTest, FixedRateSplitsOutputByPacketInterval)
{
    const ScratchDirectory scratch("file_recorder_packet_interval");

    // One recorded packet, longer than the interval the player is asked to emit.
    constexpr std::size_t sampleCount = 2000;
    constexpr double sampleRate = 10000.0;
    constexpr Int intervalMs = 20;
    constexpr std::size_t samplesPerInterval = static_cast<std::size_t>(sampleRate * intervalMs / 1000);

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
    playerFb.setPropertyValue("SampleRate", sampleRate);
    playerFb.setPropertyValue("OutputPacketIntervalMs", intervalMs);

    const auto packetReader = PacketReader(playerFb.getSignals().getItemAt(0));

    ProcedurePtr(playerFb.getPropertyValue("StartPlayback"))();

    std::size_t total = 0;
    std::size_t packets = 0;
    std::size_t longest = 0;

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (total < sampleCount && std::chrono::steady_clock::now() < deadline)
    {
        for (const auto& packet : packetReader.readAll())
        {
            if (packet.getType() != PacketType::Data)
                continue;

            const auto count = static_cast<std::size_t>(packet.asPtr<IDataPacket>().getSampleCount());
            total += count;
            longest = std::max(longest, count);
            ++packets;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    ProcedurePtr(playerFb.getPropertyValue("StopPlayback"))();

    ASSERT_EQ(total, sampleCount);

    // The recorded packet is emitted in interval-sized pieces rather than in one burst.
    EXPECT_LE(longest, samplesPerInterval);
    EXPECT_GE(packets, sampleCount / samplesPerInterval);
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

TEST_F(FileRecorderModuleTest, ContinuesIntoFollowingParts)
{
    const ScratchDirectory scratch("file_recorder_parts");

    // A 1 MiB limit holds about 131k Float64 samples, so this recording spans several parts.
    constexpr std::size_t packetSamples = 10000;
    constexpr std::size_t packets = 30;
    constexpr std::size_t totalSamples = packetSamples * packets;

    const auto module = createTestModule();

    const auto recorderFb = module.createFunctionBlock(RECORDER_ID, nullptr, "recorder");
    const auto signal = createSignal(recorderFb.getContext(), linearDomainDescriptor(1000));

    recorderFb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    recorderFb.setPropertyValue("Path", scratch.get().string());
    recorderFb.setPropertyValue("MaxFileSizeMB", 1);

    const auto recorder = recorderFb.asPtr<IRecorder>(true);
    recorder->startRecording();

    for (std::size_t i = 0; i < packets; ++i)
        sendImplicitPacket(signal, packetSamples, static_cast<double>(i * packetSamples), static_cast<Int>(i * packetSamples * 1000));

    recorder->stopRecording();

    const auto files = recordingsIn(scratch.get());
    ASSERT_GE(files.size(), 2u) << "the recording is expected to span several parts";

    // Only the first part is named; the rest have to be picked up automatically.
    const auto playerFb = module.createFunctionBlock(PLAYER_ID, nullptr, "player");
    playerFb.setPropertyValue("FilePath", files.front().string());
    playerFb.setPropertyValue("PlaybackMode", 0);
    playerFb.setPropertyValue("SampleRate", 2000000.0);

    const auto reader = StreamReader(playerFb.getSignals().getItemAt(0), SampleType::Float64, SampleType::Int64);

    ProcedurePtr(playerFb.getPropertyValue("StartPlayback"))();

    std::vector<double> values(totalSamples);
    std::vector<Int> domain(totalSamples);
    const auto read = readSamples(reader, values.data(), domain.data(), totalSamples, std::chrono::seconds(20));

    ProcedurePtr(playerFb.getPropertyValue("StopPlayback"))();

    ASSERT_EQ(read, totalSamples);

    // One unbroken ramp across the part boundaries, in order and with nothing dropped.
    for (std::size_t i = 0; i < totalSamples; ++i)
        ASSERT_DOUBLE_EQ(values[i], static_cast<double>(i)) << "at sample " << i;

    // Looping a multi-part recording returns to the first part, not to the last one read.
    constexpr std::size_t intoSecondPass = 100;

    const auto loopingFb = module.createFunctionBlock(PLAYER_ID, nullptr, "looping_player");
    loopingFb.setPropertyValue("FilePath", files.front().string());
    loopingFb.setPropertyValue("PlaybackMode", 0);
    loopingFb.setPropertyValue("SampleRate", 2000000.0);
    loopingFb.setPropertyValue("Loop", True);

    const auto loopingReader = StreamReader(loopingFb.getSignals().getItemAt(0), SampleType::Float64, SampleType::Int64);

    ProcedurePtr(loopingFb.getPropertyValue("StartPlayback"))();

    std::vector<double> loopedValues(totalSamples + intoSecondPass);
    std::vector<Int> loopedDomain(totalSamples + intoSecondPass);
    const auto loopedRead =
        readSamples(loopingReader, loopedValues.data(), loopedDomain.data(), loopedValues.size(), std::chrono::seconds(20));

    ProcedurePtr(loopingFb.getPropertyValue("StopPlayback"))();

    ASSERT_EQ(loopedRead, loopedValues.size());

    for (std::size_t i = 0; i < intoSecondPass; ++i)
        ASSERT_DOUBLE_EQ(loopedValues[totalSamples + i], static_cast<double>(i)) << "at sample " << i << " of the second pass";
}

TEST_F(FileRecorderModuleTest, StopsAtNamedPartWhenContinuationDisabled)
{
    const ScratchDirectory scratch("file_recorder_single_part");

    constexpr std::size_t packetSamples = 10000;
    constexpr std::size_t packets = 30;
    constexpr std::size_t totalSamples = packetSamples * packets;

    const auto module = createTestModule();

    const auto recorderFb = module.createFunctionBlock(RECORDER_ID, nullptr, "recorder");
    const auto signal = createSignal(recorderFb.getContext(), linearDomainDescriptor(1000));

    recorderFb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    recorderFb.setPropertyValue("Path", scratch.get().string());
    recorderFb.setPropertyValue("MaxFileSizeMB", 1);

    const auto recorder = recorderFb.asPtr<IRecorder>(true);
    recorder->startRecording();

    for (std::size_t i = 0; i < packets; ++i)
        sendImplicitPacket(signal, packetSamples, static_cast<double>(i * packetSamples), static_cast<Int>(i * packetSamples * 1000));

    recorder->stopRecording();

    const auto files = recordingsIn(scratch.get());
    ASSERT_GE(files.size(), 2u) << "the recording is expected to span several parts";

    const auto playerFb = module.createFunctionBlock(PLAYER_ID, nullptr, "player");
    playerFb.setPropertyValue("FilePath", files.front().string());
    playerFb.setPropertyValue("PlaybackMode", 0);
    playerFb.setPropertyValue("SampleRate", 2000000.0);
    playerFb.setPropertyValue("ContinueIntoNextParts", False);

    const auto reader = StreamReader(playerFb.getSignals().getItemAt(0), SampleType::Float64, SampleType::Int64);

    ProcedurePtr(playerFb.getPropertyValue("StartPlayback"))();

    // Asking for the whole recording; only the named part is expected to arrive, so the read
    // runs out of samples and returns short.
    std::vector<double> values(totalSamples);
    std::vector<Int> domain(totalSamples);
    const auto read = readSamples(reader, values.data(), domain.data(), totalSamples, std::chrono::seconds(1));

    ProcedurePtr(playerFb.getPropertyValue("StopPlayback"))();

    ASSERT_GE(read, packetSamples) << "the named part should still be replayed in full";
    ASSERT_LT(read, totalSamples) << "playback should not have continued into the following parts";

    for (std::size_t i = 0; i < read; ++i)
        ASSERT_DOUBLE_EQ(values[i], static_cast<double>(i)) << "at sample " << i;
}

TEST_F(FileRecorderModuleTest, PlaybackEndsLikeAnExplicitStopWhenTheRecordingRunsOut)
{
    const ScratchDirectory scratch("file_recorder_playback_end");
    constexpr std::size_t sampleCount = 200;

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
    playerFb.setPropertyValue("SampleRate", 100000.0);

    std::vector<double> values(sampleCount);
    std::vector<Int> domain(sampleCount);

    {
        const auto reader = StreamReader(playerFb.getSignals().getItemAt(0), SampleType::Float64, SampleType::Int64);
        ProcedurePtr(playerFb.getPropertyValue("StartPlayback"))();
        ASSERT_EQ(readSamples(reader, values.data(), domain.data(), sampleCount), sampleCount);
    }

    // The recording runs out and the playback ends by itself, with nobody stopping it.
    ASSERT_TRUE(waitUntilStatus(playerFb, "Playback finished"));

    EXPECT_NO_THROW(playerFb.setPropertyValue("SampleRate", 50000.0));

    // And the next playback starts without the finished one having to be stopped first.
    const auto reader = StreamReader(playerFb.getSignals().getItemAt(0), SampleType::Float64, SampleType::Int64);
    ProcedurePtr(playerFb.getPropertyValue("StartPlayback"))();
    EXPECT_EQ(readSamples(reader, values.data(), domain.data(), sampleCount), sampleCount);
    ProcedurePtr(playerFb.getPropertyValue("StopPlayback"))();
}

TEST_F(FileRecorderModuleTest, PlayerPropertiesAreReadOnlyWhilePlaying)
{
    const ScratchDirectory scratch("file_recorder_player_readonly");

    const auto module = createTestModule();

    const auto recorderFb = module.createFunctionBlock(RECORDER_ID, nullptr, "recorder");
    const auto signal = createSignal(recorderFb.getContext(), linearDomainDescriptor(1000));

    recorderFb.getInputPorts().getItemAt(0).asPtr<IInputPortConfig>().connect(signal);
    recorderFb.setPropertyValue("Path", scratch.get().string());

    const auto recorder = recorderFb.asPtr<IRecorder>(true);
    recorder->startRecording();
    sendImplicitPacket(signal, 20000, 0.0, 0);
    recorder->stopRecording();

    const auto files = recordingsIn(scratch.get());
    ASSERT_EQ(files.size(), 1u);

    const auto playerFb = module.createFunctionBlock(PLAYER_ID, nullptr, "player");
    playerFb.setPropertyValue("FilePath", files.front().string());
    playerFb.setPropertyValue("SampleRate", 1000.0);

    ProcedurePtr(playerFb.getPropertyValue("StartPlayback"))();

    ASSERT_THROW(playerFb.setPropertyValue("FilePath", "somewhere_else.daqrec"), AccessDeniedException);
    daqClearErrorInfo();
    ASSERT_THROW(playerFb.setPropertyValue("SampleRate", 2000.0), AccessDeniedException);
    daqClearErrorInfo();
    ASSERT_THROW(playerFb.setPropertyValue("PlaybackMode", 1), AccessDeniedException);
    daqClearErrorInfo();
    ASSERT_THROW(playerFb.setPropertyValue("Loop", True), AccessDeniedException);
    daqClearErrorInfo();

    ASSERT_EQ(playerFb.getPropertyValue("FilePath"), files.front().string());

    ProcedurePtr(playerFb.getPropertyValue("StopPlayback"))();

    // Settable again once playback is over.
    ASSERT_NO_THROW(playerFb.setPropertyValue("SampleRate", 2000.0));
    ASSERT_EQ(playerFb.getPropertyValue("SampleRate"), 2000.0);
}

TEST_F(FileRecorderModuleTest, ReplayReportsMissingFile)
{
    const auto module = createTestModule();
    const auto playerFb = module.createFunctionBlock(PLAYER_ID, nullptr, "player");

    playerFb.setPropertyValue("FilePath", (fs::current_path() / "no_such_recording.daqrec").string());
    ProcedurePtr(playerFb.getPropertyValue("StartPlayback"))();

    ASSERT_EQ(playerFb.getStatusContainer().getStatus("ComponentStatus"), "Error");
}
