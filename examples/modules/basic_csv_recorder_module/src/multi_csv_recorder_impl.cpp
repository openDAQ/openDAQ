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
    : ReaderFbBase(createType(), context, parent, localId)
{
    initProperties();
    fileBasename = static_cast<std::string>(objPtr.getPropertyValue(Props::BASENAME));
    timestampEnabled = static_cast<bool>(objPtr.getPropertyValue(Props::FILE_TIMESTAMP_ENABLED));

    params.setValueReadType(SampleType::Float64);
    params.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);  // an input that cannot be recorded is set aside
    createReader();
    addFreePort("CsvRecorderPort");
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

bool MultiCsvRecorderImpl::accepts(const ComponentPtr&, const DataDescriptorPtr& descriptor)
{
    return scalarNumeric(descriptor);
}

// Compares the status with the previous one, which the base still caches. A consumer with one column per input follows
// each input's error, not only resynchronized
bool MultiCsvRecorderImpl::onChanges(const MultiReader2StatusPtr& status)
{
    if (status.getStatus() == ComponentStatus::Error)
    {
        stopRecordingInternal(true);
        return false;
    }

    newHeader = status.getDomainDescriptorChanged() &&
                (!outputDomain.assigned() ||
                 !(MultiCsvWriter::getDomainMetadata(status.getDomainDescriptor()) == MultiCsvWriter::getDomainMetadata(outputDomain)));
    std::vector<std::string> next;
    for (const MultiReader2InputStatusPtr in : status.getInputs())
    {
        if (in.getError() != MultiReader2InputError::None)
            continue;
        const auto id = in.getInput().getGlobalId().toStdString();
        next.push_back(id);
        if (!in.getDescriptorChanged())
            continue;
        const auto it = cached.find(id);
        newHeader = newHeader || it == cached.end() || MultiCsvWriter::unitLabel(it->second) != MultiCsvWriter::unitLabel(in.getDescriptor());
        if (const auto signal = in.getInput().asPtr<IInputPort>(true).getSignal(); signal.assigned())
            names[id] = signal.getName();
    }
    newHeader = newHeader || next != recorded;
    recorded = std::move(next);
    return false;
}

void MultiCsvRecorderImpl::onPortRemoved(const InputPortConfigPtr& port)
{
    names.erase(port.getGlobalId().toStdString());
}

// One column per recorded input, in slot order
void MultiCsvRecorderImpl::processAndSend(SizeT count, SizeT packetOffset)
{
    if (!recordingActive || !writer.has_value())
        return;
    std::vector<std::unique_ptr<double[]>> columns;
    for (const void* buffer : buffers)
    {
        if (!buffer)
            continue;
        columns.push_back(std::make_unique<double[]>(count));
        std::copy_n(static_cast<const double*>(buffer), count, columns.back().get());
    }
    writer.value().writeSamples(std::move(columns), count, static_cast<Int>(packetOffset));
}

// The recorder's output is its file: a new header starts the next file, and a recovery resumes the recording even when
// the header stays the same
void MultiCsvRecorderImpl::rebuildOutputDescriptor()
{
    if ((newHeader && writer.has_value()) || recoverToActive)
        configureWriter();
}

void MultiCsvRecorderImpl::configureWriter()
{
    if (!filePath.has_value() || recorded.empty() || !outputDomain.assigned())
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
    writer.value().setHeaderInformation(outputDomain, descriptors, signalNames, writeDomain);

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
