#include <functional>
#include <iomanip>
#include <memory>
#include <string>

#include <coretypes/filesystem.h>
#include <coretypes/function_factory.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/multi_reader2_factory.h>
#include <opendaq/opendaq.h>

#include <basic_csv_recorder_module/common.h>
#include <basic_csv_recorder_module/multi_csv_recorder_impl.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

namespace
{

bool scalarNumeric(const DataDescriptorPtr& descriptor)
{
    if (!descriptor.assigned())
        return false;
    const int sampleType = static_cast<int>(descriptor.getSampleType());
    if (sampleType == 0 || sampleType > static_cast<int>(SampleType::Int64))
        return false;
    const auto dimensions = descriptor.getDimensions();
    return !dimensions.assigned() || dimensions.getCount() == 0;
}

// An Error whose inputs in error are all only Disconnected is shown as Warning, as in ReaderFbBase::blockStatus
ComponentStatus blockStatus(const MultiReader2StatusPtr& status)
{
    if (status.getStatus() != ComponentStatus::Error)
        return status.getStatus();
    for (const MultiReader2InputStatusPtr in : status.getInputs())
        if (in.getError() != MultiReader2InputError::None && in.getError() != MultiReader2InputError::Disconnected)
            return ComponentStatus::Error;
    return ComponentStatus::Warning;
}

fs::path getNextCsvFilename(const fs::path& dir, const std::string& basename, bool timestampEnabled)
{
    std::string timestamp = "";
    if (timestampEnabled)
    {
        // Get system time
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);

        // Convert to local time in a safe, cross-platform way
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif

        // Format using iostreams
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
        timestamp = "_" + oss.str();
    }

    fs::path fname = basename + timestamp + ".csv";

    // If file exists, add numeric suffix
    int index = 1;
    while (fs::exists(dir / fname))
    {
        fname = basename + timestamp + fmt::format("_{:03}", index) + ".csv";
        index++;
    }

    return (dir / fname).string();
}

}

FunctionBlockTypePtr MultiCsvRecorderImpl::createType()
{
    return FunctionBlockType(TYPE_ID, "MultiCsvRecorder", "Multi Reader CSV recording functionality");
}

MultiCsvRecorderImpl::MultiCsvRecorderImpl(const ContextPtr& context,
                                           const ComponentPtr& parent,
                                           const StringPtr& localId,
                                           const PropertyObjectPtr&)
    : FunctionBlockImpl<IFunctionBlock, IRecorder>(createType(), context, parent, localId, nullptr)
    , params(MultiReader2Params())
{
    initComponentStatus();
    initProperties();
    fileBasename = static_cast<std::string>(objPtr.getPropertyValue(Props::BASENAME));
    timestampEnabled = static_cast<bool>(objPtr.getPropertyValue(Props::FILE_TIMESTAMP_ENABLED));

    params.setValueReadType(SampleType::Float64);
    params.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);  // an input that cannot be recorded is set aside
    createReader();
    addFreePort();
}

ErrCode MultiCsvRecorderImpl::startRecording()
{
    auto lock = getRecursiveConfigLock2();
    configureWriter();
    if (!writer.has_value())
    {
        LOG_I("Start recording FAILED.")
        return OPENDAQ_ERR_INVALIDSTATE;
    }

    LOG_I("Recording to: {}", writer.value().getFilename());
    startRecordingInternal();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiCsvRecorderImpl::stopRecording()
{
    auto lock = getRecursiveConfigLock2();
    stopRecordingInternal(false);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiCsvRecorderImpl::getIsRecording(Bool* isRecording)
{
    OPENDAQ_PARAM_NOT_NULL(isRecording);
    auto lock = getRecursiveConfigLock2();
    *isRecording = recordingActive;
    return OPENDAQ_SUCCESS;
}

void MultiCsvRecorderImpl::activeChanged()
{
    if (!active)
        stopRecording();
}

void MultiCsvRecorderImpl::removed()
{
    reader.release();
    FunctionBlockImpl<IFunctionBlock, IRecorder>::removed();
}

void MultiCsvRecorderImpl::initProperties()
{
    this->tags.add(Tags::RECORDER);

    objPtr.addProperty(StringProperty(Props::DIR, ""));
    objPtr.getOnPropertyValueWrite(Props::DIR) += std::bind(&MultiCsvRecorderImpl::onPropertiesChanged, this);

    objPtr.addProperty(StringProperty(Props::BASENAME, "output"));
    objPtr.getOnPropertyValueWrite(Props::BASENAME) += std::bind(&MultiCsvRecorderImpl::onPropertiesChanged, this);

    objPtr.addProperty(BoolProperty(Props::FILE_TIMESTAMP_ENABLED, True));
    objPtr.getOnPropertyValueWrite(Props::FILE_TIMESTAMP_ENABLED) += std::bind(&MultiCsvRecorderImpl::onPropertiesChanged, this);

    objPtr.addProperty(BoolProperty(Props::WRITE_DOMAIN, False));
    objPtr.getOnPropertyValueWrite(Props::WRITE_DOMAIN) += std::bind(&MultiCsvRecorderImpl::onPropertiesChanged, this);
}

void MultiCsvRecorderImpl::onPropertiesChanged()
{
    auto lock = getRecursiveConfigLock2();
    filePath = static_cast<std::string>(objPtr.getPropertyValue(Props::DIR));
    fileBasename = static_cast<std::string>(objPtr.getPropertyValue(Props::BASENAME));
    timestampEnabled = static_cast<bool>(objPtr.getPropertyValue(Props::FILE_TIMESTAMP_ENABLED));
    writeDomain = static_cast<bool>(objPtr.getPropertyValue(Props::WRITE_DOMAIN));
    if (writer.has_value())
        configureWriter();
}

// The judgement and the onDataAvailable handler hold the block weakly, so neither keeps a removed block alive
void MultiCsvRecorderImpl::createReader()
{
    params.setAcceptsDescriptor(Function([](ComponentPtr, DataDescriptorPtr value, DataDescriptorPtr, ListPtr<IDataDescriptor>) -> bool
                                         { return scalarNumeric(value); }));
    reader = MultiReader2(params);
    reader.getOnDataAvailable() += [this, thisWeakRef = this->getWeakRefInternal<IFunctionBlock>()](BaseObjectPtr&, EventArgsPtr<>&)
    {
        const auto thisFb = thisWeakRef.getRef();
        if (thisFb.assigned())
            drain();
    };
    SizeT count = 0;
    SizeT offset = 0;
    onChanges(reader.read(nullptr, &count, 0, offset));
}

// Reads until nothing is deliverable and nothing changed; the recorded inputs are scalar, one Float64 per sample
void MultiCsvRecorderImpl::drain()
{
    auto lock = getAcquisitionLock2();
    if (!reader.assigned())
        return;

    for (;;)
    {
        SizeT count = reader.getAvailableCount();
        const auto inputs = params.getInputs();
        std::vector<std::unique_ptr<double[]>> columns;
        std::vector<void*> buffers(inputs.getCount(), nullptr);
        for (SizeT i = 0; i < inputs.getCount() && count > 0; i++)
        {
            if (std::find(recorded.begin(), recorded.end(), inputs[i].getGlobalId().toStdString()) == recorded.end())
                continue;
            columns.push_back(std::make_unique<double[]>(count));
            buffers[i] = columns.back().get();
        }

        SizeT offset = 0;
        const MultiReader2StatusPtr status = reader.read(count > 0 ? buffers.data() : nullptr, &count, 0, offset);
        if (count > 0 && recordingActive && writer.has_value())
            writer.value().writeSamples(std::move(columns), count, static_cast<Int>(offset));

        if (status.getHasChanges())
            onChanges(status);
        else if (count == 0)
            return;
    }
}

// Caches what the status reports, starts a new file when the header changes, reports the status, and removes ports that
// disconnected. A consumer with one column per input follows each input's error, not only resynchronized
void MultiCsvRecorderImpl::onChanges(const MultiReader2StatusPtr& status)
{
    bool newHeader = false;
    if (status.getStatus() == ComponentStatus::Error)
    {
        stopRecordingInternal(true);
    }
    else
    {
        if (status.getDomainDescriptorChanged())
        {
            const auto next = status.getDomainDescriptor();
            newHeader = !domain.assigned() || !(MultiCsvWriter::getDomainMetadata(next) == MultiCsvWriter::getDomainMetadata(domain));
            domain = next;
        }
        std::vector<std::string> next;
        for (const MultiReader2InputStatusPtr in : status.getInputs())
        {
            if (in.getError() != MultiReader2InputError::None)
                continue;
            const auto id = in.getInput().getGlobalId().toStdString();
            next.push_back(id);
            if (!in.getDescriptorChanged())
                continue;
            const auto descriptor = in.getDescriptor();
            const auto it = cached.find(id);
            newHeader = newHeader || it == cached.end() || MultiCsvWriter::unitLabel(it->second) != MultiCsvWriter::unitLabel(descriptor);
            cached[id] = descriptor;
            if (const auto signal = in.getInput().asPtr<IInputPort>(true).getSignal(); signal.assigned())
                names[id] = signal.getName();
        }
        newHeader = newHeader || next != recorded;
        recorded = std::move(next);
        if ((newHeader && writer.has_value()) || recoverToActive)
            configureWriter();  // a recovery resumes the recording even when the header stays the same
    }

    if (params.getInputs().getCount() == 0)
        setComponentStatusWithMessage(ComponentStatus::Warning, "No signals connected!");
    else
        setComponentStatusWithMessage(blockStatus(status), status.getStatusMessage());

    std::vector<InputPortConfigPtr> disconnected;
    for (const MultiReader2InputStatusPtr in : status.getInputs())
        if (in.getError() == MultiReader2InputError::Disconnected && !in.getInput().asPtr<IInputPort>(true).getSignal().assigned())
            disconnected.push_back(in.getInput().asPtr<IInputPortConfig>(true));  // a port connected again since stays
    if (disconnected.empty())
        return;

    // The reader lets go of a disconnected port before the port is removed, so it never sees the removal
    auto inputs = List<IComponent>();
    for (const auto& input : params.getInputs())
        if (std::find(disconnected.begin(), disconnected.end(), input) == disconnected.end())
            inputs.pushBack(input);
    params.setInputs(inputs);
    const auto next = reader.configure(params);
    for (const auto& port : disconnected)
    {
        const auto id = port.getGlobalId().toStdString();
        cached.erase(id);
        names.erase(id);
        removeInputPort(port);
    }
    onChanges(next);
}

void MultiCsvRecorderImpl::addFreePort()
{
    freePort = createAndAddInputPort(fmt::format("CsvRecorderPort_{}", nextPortId++), PacketReadyNotification::Scheduler);
}

ListPtr<IComponent> MultiCsvRecorderImpl::readerPorts() const
{
    auto list = List<IComponent>();
    for (const auto& port : inputPorts.getItems())
        if (port != freePort)
            list.pushBack(port);
    return list;
}

// A signal connected to the free port, so the port joins the reader and the next free port is offered
void MultiCsvRecorderImpl::onConnected(const InputPortPtr& port)
{
    auto lock = getAcquisitionLock2();
    if (!reader.assigned() || !freePort.assigned() || port != freePort)
        return;
    LOG_I("Multi CSV Recorder: Input port {} connected", port.getLocalId())
    addFreePort();
    params.setInputs(readerPorts());
    onChanges(reader.configure(params));
}

void MultiCsvRecorderImpl::configureWriter()
{
    if (!filePath.has_value() || recorded.empty() || !domain.assigned())
    {
        writer = std::nullopt;
        return;
    }

    auto descriptors = List<IDataDescriptor>();
    auto signalNames = List<IString>();
    for (const auto& id : recorded)
    {
        descriptors.pushBack(cached[id]);
        signalNames.pushBack(names[id]);
    }
    // The previous writer closes its file first: its thread creates the file, and the next name skips existing files
    writer = std::nullopt;
    writer.emplace(getNextCsvFilename(filePath.value(), fileBasename, timestampEnabled));
    writer.value().setHeaderInformation(domain, descriptors, signalNames, writeDomain);

    // Auto resume recording if recording was stopped internally
    if (recoverToActive)
        startRecordingInternal();
}

void MultiCsvRecorderImpl::stopRecordingInternal(bool recover)
{
    // This is the only method recording should be stopped by. If already stopped, nothing to do.
    if (!recordingActive)
        return;

    LOG_I("Recording stopped.")

    // Close the file
    writer = std::nullopt;

    // Recover flag on only when turning off the recording.
    recoverToActive = recordingActive && recover;
    recordingActive = false;
}

void MultiCsvRecorderImpl::startRecordingInternal()
{
    recordingActive = true;
    recoverToActive = false;
}

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
