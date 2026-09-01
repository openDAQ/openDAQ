#include <functional>
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

    initProperties();
    readProperties();
    updateInputPorts();
}

FileRecorderFbImpl::~FileRecorderFbImpl()
{
    clearWriters();
}

ErrCode FileRecorderFbImpl::startRecording()
{
    auto lock = getRecursiveConfigLock();

    if (recording)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Recording is already active.");

    readProperties();

    recording = true;
    createWriters();

    LOG_I("File recorder: recording to {}", path.string())

    return OPENDAQ_SUCCESS;
}

ErrCode FileRecorderFbImpl::stopRecording()
{
    auto lock = getRecursiveConfigLock();

    if (!recording)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Recording is not active.");

    recording = false;
    clearWriters();

    LOG_I("File recorder: recording stopped")

    return OPENDAQ_SUCCESS;
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
    if (!active && recording)
        stopRecording();
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
    for (const auto& port : borrowPtr<FunctionBlockPtr>().getInputPorts())
    {
        const auto signal = port.getSignal();
        if (!signal.assigned())
            continue;

        auto writer = std::make_shared<SignalFileWriter>(path, signal, maxFileSizeBytes, loggerComponent);

        std::lock_guard writersLock(writersMutex);
        writers.emplace(port.getObject(), std::move(writer));
    }
}

void FileRecorderFbImpl::clearWriters()
{
    decltype(writers) closing;

    {
        std::lock_guard writersLock(writersMutex);
        closing.swap(writers);
    }

    // Destroyed outside the lock: each destructor blocks until its writer has drained the queue,
    // so no samples accepted before the stop are lost, and the data path is not held up by it.
    closing.clear();
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
}

END_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE
