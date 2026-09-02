#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <coreobjects/eval_value_factory.h>

#include <opendaq/custom_log.h>
#include <opendaq/opendaq.h>

#include <file_recorder_module/common.h>
#include <file_recorder_module/file_recorder_fb_impl.h>

BEGIN_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE

namespace
{
constexpr std::uint64_t BYTES_PER_MEGABYTE = 1024ull * 1024ull;

/*!
 * The shortest timed recording which can be asked for. It only keeps the property away from zero,
 * where a recording would end before any data reached it.
 */
constexpr double MIN_DURATION_SECONDS = 0.001;
}

FunctionBlockTypePtr FileRecorderFbImpl::createType()
{
    return FunctionBlockType(TYPE_ID, "FileRecorder", "Records the connected signals into binary files", PropertyObject());
}

FileRecorderFbImpl::FileRecorderFbImpl(const ContextPtr& context,
                                       const ComponentPtr& parent,
                                       const StringPtr& localId,
                                       const PropertyObjectPtr& config)
    : FunctionBlockImpl<IFunctionBlock, IRecorder>(createType(), context, parent, localId, nullptr)
{
    tags.add(Tags::RECORDER);

    initComponentStatus();
    initProperties();
    readProperties();
    updateInputPorts();
}

FileRecorderFbImpl::~FileRecorderFbImpl()
{
    {
        auto lock = getRecursiveConfigLock();

        // Cleared before the thread is joined, so that one already on its way into a stop finds
        // nothing left to do rather than working on a function block being destroyed.
        recording = false;
        cancelAutoStop();
    }

    if (autoStopThread.joinable())
        autoStopThread.join();

    clearWriters();
}

ErrCode FileRecorderFbImpl::startRecording()
{
    // The thread of the previous recording has already been cancelled and is only joined here,
    // before the configuration lock is taken, because that is the lock it may still be waiting
    // for on its way out.
    auto previous = takeAutoStopThread();
    if (previous.joinable())
        previous.join();

    auto lock = getRecursiveConfigLock();

    if (recording)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Recording is already active.");

    readProperties();

    autoStop = recordingMode == RecordingMode::Manual ? nullptr : std::make_shared<AutoStop>();
    recording = true;

    // Only the stems of this recording are reserved: those of earlier ones are protected by the
    // files they left on disk, and one which wrote nothing is free to be reused.
    usedFilenameStems.clear();
    createWriters();

    if (autoStop)
        autoStopThread = std::thread(
            &FileRecorderFbImpl::autoStopMain, this, autoStop, recordingMode, std::chrono::steady_clock::now() + duration);

    LOG_I("File recorder: recording to {}", path.string())

    {
        std::lock_guard statusLock(statusMutex);
        recordingStatusMessage = fmt::format("Recording to {}", path.string());
        setComponentStatusWithMessage(ComponentStatus::Ok, recordingStatusMessage);
    }

    return OPENDAQ_SUCCESS;
}

ErrCode FileRecorderFbImpl::stopRecording()
{
    if (!stopRecording(StopReason::Explicit, nullptr))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Recording is not active.");

    return OPENDAQ_SUCCESS;
}

bool FileRecorderFbImpl::stopRecording(StopReason reason, const std::shared_ptr<AutoStop>& from)
{
    auto lock = getRecursiveConfigLock();

    // A thread which outlived its own recording finds a different state here and leaves the
    // recording it is looking at alone.
    if (!recording || (from && autoStop != from))
        return false;

    recording = false;
    cancelAutoStop();

    // A signal which gave up mid-recording is still reported once the recording is over, rather
    // than being tidied away by the stop.
    const auto failure = clearWriters();

    LOG_I("File recorder: {}", describe(reason))

    std::lock_guard statusLock(statusMutex);
    recordingStatusMessage.clear();

    if (failure.assigned())
        setComponentStatusWithMessage(ComponentStatus::Error, failure);
    else
        setComponentStatusWithMessage(ComponentStatus::Ok, describe(reason));

    return true;
}

const char* FileRecorderFbImpl::describe(StopReason reason)
{
    switch (reason)
    {
        case StopReason::SampleCountReached:
            return "Recording stopped: every signal recorded the configured number of samples";
        case StopReason::DurationElapsed:
            return "Recording stopped: the configured duration elapsed";
        case StopReason::Explicit:
        default:
            return "Recording stopped";
    }
}

ErrCode FileRecorderFbImpl::getIsRecording(Bool* isRecording)
{
    OPENDAQ_PARAM_NOT_NULL(isRecording);

    auto lock = getRecursiveConfigLock();
    *isRecording = recording;

    return OPENDAQ_SUCCESS;
}

void FileRecorderFbImpl::onConnected(const InputPortPtr& port)
{
    auto lock = getRecursiveConfigLock();

    LOG_D("File recorder: input port {} connected", port.getLocalId())

    updateInputPorts();

    if (recording)
        createWriters();
}

void FileRecorderFbImpl::onDisconnected(const InputPortPtr& port)
{
    auto lock = getRecursiveConfigLock();

    LOG_D("File recorder: input port {} disconnected", port.getLocalId())

    std::shared_ptr<SignalFileWriter> closing;
    {
        std::lock_guard writersLock(writersMutex);
        const auto it = writers.find(port.getObject());
        if (it != writers.end())
        {
            closing = std::move(it->second);
            writers.erase(it);
        }
    }

    // Releasing the last reference flushes and closes the signal's current file.
    closing.reset();

    updateInputPorts();

    // One fewer signal to wait for, which may be the last one the recording was waiting on.
    wakeAutoStop();
}

void FileRecorderFbImpl::onPacketReceived(const InputPortPtr& port)
{
    const auto connection = port.getConnection();
    if (!connection.assigned())
        return;

    // The connection is drained even when not recording, so that packets do not pile up in it
    // while the recorder sits idle.
    const auto packets = connection.dequeueAll();

    if (!recording || !active || !packets.assigned())
        return;

    const auto writer = findWriter(port.getObject());
    if (!writer)
        return;

    for (const auto& packet : packets)
        writer->post(packet);
}

void FileRecorderFbImpl::activeChanged()
{
    if (!active)
        stopRecording(StopReason::Explicit, nullptr);
}

EvalValuePtr FileRecorderFbImpl::lockedWhileRecording()
{
    // EvalValueFunc resolves the bare identifier through the callback, which lets the flag read
    // the recording state directly instead of needing a property to mirror it.
    return EvalValueFunc("recording", Function([this](const BaseObjectPtr& /*name*/) { return BooleanPtr(recording.load()); }));
}

void FileRecorderFbImpl::initProperties()
{
    objPtr.addProperty(StringPropertyBuilder(Props::PATH, "").setReadOnly(lockedWhileRecording()).build());
    objPtr.getOnPropertyValueWrite(Props::PATH) += std::bind(&FileRecorderFbImpl::readProperties, this);

    objPtr.addProperty(IntPropertyBuilder(Props::MAX_FILE_SIZE_MB, 100).setReadOnly(lockedWhileRecording()).build());
    objPtr.getOnPropertyValueWrite(Props::MAX_FILE_SIZE_MB) += std::bind(&FileRecorderFbImpl::readProperties, this);

    objPtr.addProperty(IntPropertyBuilder(Props::MAX_BUFFER_MB, 4).setMinValue(0).setReadOnly(lockedWhileRecording()).build());
    objPtr.getOnPropertyValueWrite(Props::MAX_BUFFER_MB) += std::bind(&FileRecorderFbImpl::readProperties, this);

    objPtr.addProperty(SelectionPropertyBuilder(Props::RECORDING_MODE, List<IString>("Manual", "SampleCount", "Duration"), 0)
                           .setReadOnly(lockedWhileRecording())
                           .build());
    objPtr.getOnPropertyValueWrite(Props::RECORDING_MODE) += std::bind(&FileRecorderFbImpl::readProperties, this);

    objPtr.addProperty(IntPropertyBuilder(Props::SAMPLE_COUNT, 10000)
                           .setMinValue(1)
                           .setVisible(EvalValue("$RecordingMode == 1"))
                           .setReadOnly(lockedWhileRecording())
                           .build());
    objPtr.getOnPropertyValueWrite(Props::SAMPLE_COUNT) += std::bind(&FileRecorderFbImpl::readProperties, this);

    objPtr.addProperty(FloatPropertyBuilder(Props::DURATION_SECONDS, 10.0)
                           .setMinValue(MIN_DURATION_SECONDS)
                           .setVisible(EvalValue("$RecordingMode == 2"))
                           .setReadOnly(lockedWhileRecording())
                           .setUnit(Unit("s", -1, "seconds", "time"))
                           .build());
    objPtr.getOnPropertyValueWrite(Props::DURATION_SECONDS) += std::bind(&FileRecorderFbImpl::readProperties, this);
}

void FileRecorderFbImpl::updateInputPorts()
{
    // The list is snapshotted because removeInputPort() mutates the function block's port list.
    const auto currentPorts = borrowPtr<FunctionBlockPtr>().getInputPorts();

    for (const auto& port : currentPorts)
    {
        const auto sig = port.getSignal();
        if (!sig.assigned())
            removeInputPort(port);
    }

    createAndAddInputPort(fmt::format("Input{}", portCount++), PacketReadyNotification::SameThread);
}

void FileRecorderFbImpl::createWriters()
{
    const auto sampleLimit = recordingMode == RecordingMode::SampleCount ? sampleCount : 0;

    // Reaching back into the function block is safe because a writer's destructor joins the
    // thread which calls this, and this function block outlives every writer it owns. The
    // recording's auto-stop state travels with the callback, so that a writer never has to read
    // a member the configuration lock guards.
    const auto onStateChanged = [this, state = autoStop] { onWriterStateChanged(state); };

    for (const auto& port : borrowPtr<FunctionBlockPtr>().getInputPorts())
    {
        const auto signal = port.getSignal();
        if (!signal.assigned())
            continue;

        {
            // Checked before a writer is built, because building one starts a thread and claims
            // a filename stem which a discarded writer would waste.
            std::lock_guard writersLock(writersMutex);
            if (writers.count(port.getObject()) != 0)
                continue;
        }

        auto writer = std::make_shared<SignalFileWriter>(path,
                                                        signal,
                                                        uniqueFilenameStem(signal),
                                                        maxFileSizeBytes,
                                                        sampleLimit,
                                                        maxBufferBytes,
                                                        onStateChanged,
                                                        loggerComponent);

        std::lock_guard writersLock(writersMutex);
        writers.emplace(port.getObject(), std::move(writer));
    }
}

std::string FileRecorderFbImpl::uniqueFilenameStem(const SignalPtr& signal)
{
    const auto base = SignalFileWriter::buildFilenameStem(signal.getName());

    auto stem = base;
    for (int discriminator = 2; !isFilenameStemFree(stem); ++discriminator)
        stem = base + "_" + std::to_string(discriminator);

    usedFilenameStems.insert(stem);
    return stem;
}

bool FileRecorderFbImpl::isFilenameStemFree(const std::string& stem) const
{
    if (usedFilenameStems.count(stem) != 0)
        return false;

    // A writer only ever creates its first part when its first packet arrives, so an earlier
    // recording which wrote nothing leaves its stem free, as there is nothing to overwrite.
    std::error_code error;
    return !fs::exists(SignalFileWriter::partPath(path, stem, 1), error);
}

bool FileRecorderFbImpl::allWritersFinished() const
{
    std::lock_guard writersLock(writersMutex);

    if (writers.empty())
        return false;

    for (const auto& [port, writer] : writers)
        if (!writer->getStatus().finished)
            return false;

    return true;
}

void FileRecorderFbImpl::onWriterStateChanged(const std::shared_ptr<AutoStop>& state)
{
    if (state)
    {
        {
            std::lock_guard stopLock(state->mutex);
            state->wake = true;
        }
        state->cv.notify_all();
    }

    updateRecordingStatus();
}

void FileRecorderFbImpl::updateRecordingStatus()
{
    // A recording which is over has already reported how it ended, and the writers being torn
    // down have nothing to add to it.
    if (!recording)
        return;

    std::vector<std::shared_ptr<SignalFileWriter>> current;
    {
        std::lock_guard writersLock(writersMutex);

        current.reserve(writers.size());
        for (const auto& [port, writer] : writers)
            current.push_back(writer);
    }

    StringPtr failure;
    bool backlog = false;

    for (const auto& writer : current)
    {
        const auto status = writer->getStatus();

        if (status.failed && !failure.assigned())
            failure = String(status.message);

        backlog = backlog || status.queueHigh;
    }

    std::lock_guard statusLock(statusMutex);

    if (failure.assigned())
        setComponentStatusWithMessage(ComponentStatus::Error, failure);
    else if (backlog)
        setComponentStatusWithMessage(ComponentStatus::Warning, "Data is being buffered faster than it can be written");
    else
        setComponentStatusWithMessage(ComponentStatus::Ok, recordingStatusMessage);
}

void FileRecorderFbImpl::autoStopMain(std::shared_ptr<AutoStop> state,
                                      RecordingMode mode,
                                      std::chrono::steady_clock::time_point deadline)
{
    while (true)
    {
        {
            std::unique_lock stopLock(state->mutex);

            if (mode == RecordingMode::Duration)
            {
                // Returns the predicate, so a true result is a cancellation and a false one is
                // the deadline the recording was waiting for.
                if (state->cv.wait_until(stopLock, deadline, [&state] { return state->cancelled; }))
                    return;
            }
            else
            {
                state->cv.wait(stopLock, [&state] { return state->cancelled || state->wake; });

                if (state->cancelled)
                    return;

                state->wake = false;
            }
        }

        // Checked outside the state's mutex, which a writer's thread takes to report itself done.
        if (mode == RecordingMode::SampleCount && !allWritersFinished())
            continue;

        // The very same stop an explicit one performs, so that nothing is left behind for the
        // user to stop a second time. It does nothing if the recording is already over.
        stopRecording(mode == RecordingMode::Duration ? StopReason::DurationElapsed : StopReason::SampleCountReached, state);

        return;
    }
}

void FileRecorderFbImpl::cancelAutoStop()
{
    if (!autoStop)
        return;

    {
        std::lock_guard stopLock(autoStop->mutex);
        autoStop->cancelled = true;
    }
    autoStop->cv.notify_all();

    // The thread's handle stays behind to be joined by the next start or by the destructor: this
    // is also reached from the auto-stop thread itself, and from callbacks holding the lock it
    // would need to notice the cancellation.
    autoStop.reset();
}

std::thread FileRecorderFbImpl::takeAutoStopThread()
{
    auto lock = getRecursiveConfigLock();

    cancelAutoStop();

    return std::move(autoStopThread);
}

void FileRecorderFbImpl::wakeAutoStop()
{
    if (!autoStop)
        return;

    {
        std::lock_guard stopLock(autoStop->mutex);
        autoStop->wake = true;
    }
    autoStop->cv.notify_all();
}

StringPtr FileRecorderFbImpl::clearWriters()
{
    decltype(writers) closing;

    {
        std::lock_guard writersLock(writersMutex);
        closing.swap(writers);
    }

    // Closed outside the lock: each writer blocks until it has drained its queue, so no samples
    // accepted before the stop are lost, and the data path is not held up by it.
    StringPtr failure;
    for (const auto& [port, writer] : closing)
    {
        writer->close();

        const auto status = writer->getStatus();
        if (status.failed && !failure.assigned())
            failure = String(status.message);
    }

    closing.clear();
    return failure;
}

std::shared_ptr<SignalFileWriter> FileRecorderFbImpl::findWriter(IInputPort* port)
{
    std::lock_guard writersLock(writersMutex);

    const auto it = writers.find(port);
    return it != writers.end() ? it->second : nullptr;
}

void FileRecorderFbImpl::readProperties()
{
    auto lock = getRecursiveConfigLock();

    path = fs::path(static_cast<std::string>(objPtr.getPropertyValue(Props::PATH))).lexically_normal();

    const Int maxFileSizeMb = objPtr.getPropertyValue(Props::MAX_FILE_SIZE_MB);
    maxFileSizeBytes = maxFileSizeMb > 0 ? static_cast<std::uint64_t>(maxFileSizeMb) * BYTES_PER_MEGABYTE : 0;

    const Int maxBufferMb = objPtr.getPropertyValue(Props::MAX_BUFFER_MB);
    maxBufferBytes = maxBufferMb > 0 ? static_cast<std::uint64_t>(maxBufferMb) * BYTES_PER_MEGABYTE : 0;

    recordingMode = static_cast<RecordingMode>(static_cast<Int>(objPtr.getPropertyValue(Props::RECORDING_MODE)));

    const Int samples = objPtr.getPropertyValue(Props::SAMPLE_COUNT);
    sampleCount = samples > 0 ? static_cast<std::uint64_t>(samples) : 0;

    const double seconds = objPtr.getPropertyValue(Props::DURATION_SECONDS);
    duration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(std::max(MIN_DURATION_SECONDS, seconds)));
}

END_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE
