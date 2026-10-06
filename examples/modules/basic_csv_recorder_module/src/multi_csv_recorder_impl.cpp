#include <functional>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

#include <coretypes/filesystem.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <basic_csv_recorder_module/common.h>
#include <basic_csv_recorder_module/multi_csv_recorder_impl.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

namespace
{
    fs::path getNextCsvFilename(const fs::path& dir, const std::string& basename, bool timestampEnabled)
    {
        std::string timestamp;
        if (timestampEnabled)
        {
            const auto now = std::chrono::system_clock::now();
            const std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
#ifdef _WIN32
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
            timestamp = "_" + oss.str();
        }

        fs::path fname = basename + timestamp + ".csv";
        int index = 1;
        while (fs::exists(dir / fname))
        {
            fname = basename + timestamp + fmt::format("_{:03}", index);
            fname += ".csv";
            index++;
        }
        return (dir / fname).string();
    }

    const char* errorName(MultiReader2InputError error)
    {
        switch (error)
        {
            case MultiReader2InputError::Disconnected: return "not connected";
            case MultiReader2InputError::ValueDescriptorInvalid: return "value descriptor not readable";
            case MultiReader2InputError::DomainDescriptorInvalid: return "domain descriptor not compatible";
            case MultiReader2InputError::SyncFailed: return "not synchronized";
            default: return "ok";
        }
    }
}

FunctionBlockTypePtr MultiCsvRecorderImpl::createType()
{
    return FunctionBlockType(TYPE_ID, "MultiCsvRecorder", "Multi Reader CSV recording functionality");
}

MultiCsvRecorderImpl::MultiCsvRecorderImpl(const ContextPtr& context,
                                           const ComponentPtr& parent,
                                           const StringPtr& localId,
                                           const PropertyObjectPtr& /*config*/)
    : FunctionBlockImpl<IFunctionBlock, IRecorder>(createType(), context, parent, localId, nullptr)
    , params(MultiReader2Params())
{
    initComponentStatus();
    setComponentStatusWithMessage(ComponentStatus::Warning, "No signals connected!");

    initProperties();
    fileBasename = static_cast<std::string>(objPtr.getPropertyValue(Props::BASENAME));
    timestampEnabled = static_cast<bool>(objPtr.getPropertyValue(Props::FILE_TIMESTAMP_ENABLED));

    params.setInputs(List<IComponent>());
    params.setValueReadType(SampleType::Float64);
    params.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);  // a rejected or misaligned input is set aside, the others are recorded
    createReader();
    addFreePort();
}

ErrCode MultiCsvRecorderImpl::startRecording()
{
    auto lock = getRecursiveConfigLock();
    configureWriter();
    if (!filePath.has_value() || !writer.has_value())
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
    auto lock = getRecursiveConfigLock();
    stopRecordingInternal(false);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiCsvRecorderImpl::getIsRecording(Bool* isRecording)
{
    OPENDAQ_PARAM_NOT_NULL(isRecording);
    auto lock = getRecursiveConfigLock();
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
    filePath = static_cast<std::string>(objPtr.getPropertyValue(Props::DIR));
    fileBasename = static_cast<std::string>(objPtr.getPropertyValue(Props::BASENAME));
    timestampEnabled = static_cast<bool>(objPtr.getPropertyValue(Props::FILE_TIMESTAMP_ENABLED));
    writeDomain = static_cast<bool>(objPtr.getPropertyValue(Props::WRITE_DOMAIN));
    configureWriter();
}

// ---------------------------------------------------------------- ports and reader

void MultiCsvRecorderImpl::addFreePort()
{
    freePort = createAndAddInputPort(fmt::format("CsvRecorderPort_{}", nextPortId++), PacketReadyNotification::Scheduler);
}

ListPtr<IComponent> MultiCsvRecorderImpl::readerPorts() const
{
    auto list = List<IComponent>();
    for (const auto& port : inputPorts.getItems())
    {
        if (port != freePort)
            list.pushBack(port);
    }
    return list;
}

// A signal connected to the free port: the port joins the reader, which judges the descriptor, and the next free
// port is offered. Under the block's lock, since drain reconfigures and reads under the same lock
void MultiCsvRecorderImpl::onConnected(const InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock2();
    if (!reader.assigned() || port != freePort)
        return;
    if (const SignalPtr signal = freePort.getSignal(); signal.assigned())
        cachedSignalNames[freePort.getGlobalId().toStdString()] = signal.getName();
    addFreePort();
    params.setInputs(readerPorts());
    reader.configure(params);
}

void MultiCsvRecorderImpl::createReader()
{
    params.setAcceptsDescriptor(Function([](ComponentPtr, DataDescriptorPtr value, DataDescriptorPtr) -> bool { return accepts(value); }));
    reader = MultiReader2(params);
    reader.getOnDataAvailable() += [this, thisWeakRef = this->getWeakRefInternal<IFunctionBlock>()](InputPortPtr&, EventArgsPtr<>&)
    {
        const auto thisFb = thisWeakRef.getRef();
        if (thisFb.assigned())
            drain();
    };
}

bool MultiCsvRecorderImpl::accepts(const DataDescriptorPtr& descriptor)
{
    if (!descriptor.assigned())
        return false;
    const int sampleType = static_cast<int>(descriptor.getSampleType());
    if (sampleType == 0 || sampleType > static_cast<int>(SampleType::Int64))
        return false;  // scalar numeric types only
    const auto dimensions = descriptor.getDimensions();
    return !dimensions.assigned() || dimensions.getCount() == 0;
}

// Reads until nothing is deliverable and nothing changed; samples go to the writer, statuses to onChanges
void MultiCsvRecorderImpl::drain()
{
    auto lock = this->getAcquisitionLock2();
    if (!reader.assigned())
        return;

    for (;;)
    {
        SizeT count = reader.getAvailableCount();
        const SizeT slots = params.getInputs().getCount();
        storage.resize(slots);
        buffers.resize(slots);
        for (SizeT i = 0; i < slots; i++)
        {
            storage[i].resize(count);
            buffers[i] = storage[i].data();
        }

        SizeT offset = 0;
        const MultiReader2StatusPtr status = reader.read(count > 0 ? buffers.data() : nullptr, &count, offset);
        if (count > 0)
            writeSamples(count, offset);

        if (status.getHasChanges())
        {
            if (onChanges(status))
                return;
        }
        else if (count == 0)
        {
            return;
        }
    }
}

bool MultiCsvRecorderImpl::onChanges(const MultiReader2StatusPtr& status)
{
    for (const MultiReader2InputStatusPtr in : status.getInputs())
    {
        const InputPortConfigPtr port = in.getInput().asPtr<IInputPortConfig>(true);
        if (in.getError() == MultiReader2InputError::Disconnected)
        {
            LOG_I("Multi CSV Recorder: Input port {} disconnected, removing it", port.getLocalId())
            const auto id = port.getGlobalId().toStdString();
            cachedDescriptors.erase(id);
            cachedSignalNames.erase(id);
            removeInputPort(port);
            params.setInputs(readerPorts());
            reader.configure(params);
            return true;
        }
    }

    // The slot order and the contributing set, for the data that follows this status
    const auto inputs = status.getInputs();
    slotInputs.clear();
    activeSlots.assign(inputs.getCount(), false);
    std::ostringstream message;
    bool anyActive = false;
    for (SizeT i = 0; i < inputs.getCount(); i++)
    {
        const MultiReader2InputStatusPtr in = inputs[i];
        slotInputs.push_back(in.getInput());
        activeSlots[i] = status.getValid() && in.getError() == MultiReader2InputError::None;
        anyActive = anyActive || activeSlots[i];
        if (in.getError() != MultiReader2InputError::None)
            message << (message.tellp() > 0 ? "; " : "") << in.getInput().getLocalId() << " " << errorName(in.getError());
    }

    if (!status.getValid())
    {
        stopRecordingInternal(true);
        setComponentStatusWithMessage(ComponentStatus::Warning, "Waiting for inputs: " + message.str());
        return false;
    }

    bool headerChanged = false;
    if (status.getDomainDescriptorChanged())
    {
        // Valid with nothing used has no main domain: the descriptor is then unassigned
        const auto domain = status.getDomainDescriptor();
        headerChanged |= !domain.assigned() || !recorderDomainDataDescriptor.assigned() ||
                         !(MultiCsvWriter::getDomainMetadata(domain) == MultiCsvWriter::getDomainMetadata(recorderDomainDataDescriptor));
        recorderDomainDataDescriptor = domain;
    }
    for (const MultiReader2InputStatusPtr in : inputs)
    {
        if (!in.getDescriptorChanged())
            continue;
        const auto id = in.getInput().getGlobalId().toStdString();
        const auto it = cachedDescriptors.find(id);
        headerChanged |= it == cachedDescriptors.end() || !it->second.assigned() ||
                         MultiCsvWriter::unitLabel(in.getDescriptor()) != MultiCsvWriter::unitLabel(it->second);
        cachedDescriptors[id] = in.getDescriptor();
    }

    if (!anyActive)
        setComponentStatusWithMessage(ComponentStatus::Warning, "No signals connected!");
    else if (message.tellp() > 0)
        setComponentStatusWithMessage(ComponentStatus::Warning, "Inputs set aside: " + message.str());
    else
        setComponentStatus(ComponentStatus::Ok);

    if (headerChanged || status.getResynchronized())
        configureWriter();
    return false;
}

void MultiCsvRecorderImpl::writeSamples(SizeT count, SizeT packetOffset)
{
    if (!recordingActive || !writer.has_value())
        return;

    std::vector<std::unique_ptr<double[]>> samples;
    for (SizeT slot = 0; slot < buffers.size(); slot++)
    {
        if (!(slot < activeSlots.size() && activeSlots[slot]))
            continue;
        auto column = std::make_unique<double[]>(count);
        std::copy_n(static_cast<const double*>(buffers[slot]), count, column.get());
        samples.push_back(std::move(column));
    }
    writer.value().writeSamples(std::move(samples), count, static_cast<Int>(packetOffset));
}

// ---------------------------------------------------------------- writer

void MultiCsvRecorderImpl::configureWriter()
{
    try
    {
        auto valueDescriptors = List<IDataDescriptor>();
        auto signalNames = List<IString>();
        for (SizeT slot = 0; slot < slotInputs.size(); slot++)
        {
            if (!(slot < activeSlots.size() && activeSlots[slot]))
                continue;
            const auto id = slotInputs[slot].getGlobalId().toStdString();
            const auto descriptor = cachedDescriptors.find(id);
            if (descriptor == cachedDescriptors.end() || !descriptor->second.assigned())
                throw std::runtime_error("Missing input value descriptors!");
            valueDescriptors.pushBack(descriptor->second);
            const auto name = cachedSignalNames.find(id);
            signalNames.pushBack(name != cachedSignalNames.end() ? name->second : String(slotInputs[slot].getLocalId()));
        }

        if (valueDescriptors.getCount() == 0)
            throw std::runtime_error("No signals connected!");
        if (!recorderDomainDataDescriptor.assigned())
            throw std::runtime_error("Input domain descriptor is not set");

        if (!filePath.has_value())
            return;
        const fs::path outputFile = getNextCsvFilename(filePath.value(), fileBasename, timestampEnabled);

        // A new file per configuration; the header describes the inputs recorded from here on
        writer.emplace(outputFile);
        writer.value().setHeaderInformation(recorderDomainDataDescriptor, valueDescriptors, signalNames, writeDomain);

        if (recoverToActive)
            startRecordingInternal();
    }
    catch (const std::exception& e)
    {
        stopRecordingInternal(true);
        setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Failed to configure CSV recorder: {}", e.what()));
    }
}

void MultiCsvRecorderImpl::stopRecordingInternal(bool recover)
{
    if (!recordingActive)
        return;
    LOG_I("Recording stopped.")
    writer = std::nullopt;
    recoverToActive = recordingActive && recover;
    recordingActive = false;
}

void MultiCsvRecorderImpl::startRecordingInternal()
{
    recordingActive = true;
    recoverToActive = false;
}

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
