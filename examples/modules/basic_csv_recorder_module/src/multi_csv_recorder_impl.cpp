#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string>

#include <coretypes/filesystem.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>
#include <opendaq/reader_config_ptr.h>
#include <opendaq/reader_factory.h>

#include <basic_csv_recorder_module/common.h>
#include <basic_csv_recorder_module/multi_csv_recorder_impl.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

namespace
{
bool descriptorNotNull(const DataDescriptorPtr& descriptor)
{
    return descriptor.assigned() && descriptor != NullDataDescriptor();
}

bool valueDescriptorsEqual(const DataDescriptorPtr& current, const DataDescriptorPtr& previous)
{
    if (current.assigned() && previous.assigned())
    {
        return MultiCsvWriter::unitLabel(current) == MultiCsvWriter::unitLabel(previous);
    }
    return !current.assigned() && !previous.assigned();
}

bool domainDescriptorsEqual(const DataDescriptorPtr& current, const DataDescriptorPtr& previous)
{
    if (current.assigned() && previous.assigned())
    {
        return MultiCsvWriter::getDomainMetadata(current) == MultiCsvWriter::getDomainMetadata(previous);
    }
    return !current.assigned() && !previous.assigned();
}

void getDataDescriptors(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc, DataDescriptorPtr& domainDesc)
{
    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        valueDesc = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        domainDesc = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
    }
}

bool getDomainDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& domainDesc)
{
    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        domainDesc = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        return true;
    }
    return false;
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
    auto config = PropertyObject();
    config.addProperty(
        SparseSelectionProperty("ReaderNotificationMode",
                                Dict<IInteger, IString>({{static_cast<Int>(PacketReadyNotification::SameThread), "SameThread"},
                                                         {static_cast<Int>(PacketReadyNotification::Scheduler), "Scheduler"}}),
                                2));

    return FunctionBlockType(TYPE_ID, "MultiCsvRecorder", "Multi Reader CSV recording functionality", config);
}

MultiCsvRecorderImpl::MultiCsvRecorderImpl(const ContextPtr& context,
                                           const ComponentPtr& parent,
                                           const StringPtr& localId,
                                           const PropertyObjectPtr& config)
    : FunctionBlockImpl<IFunctionBlock, IRecorder>(createType(), context, parent, localId, nullptr)
{
    initComponentStatus();
    setComponentStatusWithMessage(ComponentStatus::Warning, "No signals connected!");

    initProperties();
    fileBasename = static_cast<std::string>(objPtr.getPropertyValue(Props::BASENAME));
    timestampEnabled = static_cast<bool>(objPtr.getPropertyValue(Props::FILE_TIMESTAMP_ENABLED));

    if (config.assigned())
        notificationMode = static_cast<PacketReadyNotification>(config.getPropertyValue("ReaderNotificationMode"));
    else
        notificationMode = PacketReadyNotification::Scheduler;

    createDisconnectedPort();
    createReader();
}

ErrCode MultiCsvRecorderImpl::startRecording()
{
    auto lock = getRecursiveConfigLock();
    reconfigureWriter();
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

std::string MultiCsvRecorderImpl::getNextPortID() const
{
    int maxId = 0;
    for (const auto& port : connectedPorts)
    {
        std::string portId = port.getLocalId();
        auto pos = portId.find_last_of('_');
        int curId = std::stoi(portId.substr(pos + 1));
        maxId = curId > maxId ? curId : maxId;
    }

    return fmt::format("CsvRecorderPort_{}", maxId + 1);
}

void MultiCsvRecorderImpl::createDisconnectedPort()
{
    std::string id = getNextPortID();
    auto inputPort = createAndAddInputPort(id, notificationMode);
    disconnectedPort = inputPort;
}

bool MultiCsvRecorderImpl::updateInputPorts()
{
    bool connectedPortsChanged = false;
    if (disconnectedPort.assigned() && disconnectedPort.getConnection().assigned())
    {
        connectedPorts.emplace_back(disconnectedPort);
        cachedDescriptors.insert(std::make_pair(disconnectedPort.getGlobalId(), NullDataDescriptor()));
        SignalPtr signal = disconnectedPort.getSignal();
        cachedSignalNames.insert(std::make_pair(disconnectedPort.getGlobalId(), signal.getName()));

        // Activate the newly connected port
        reader.setInputUnused(disconnectedPort.getGlobalId(), false);
        disconnectedPort.release();
        connectedPortsChanged = true;
    }

    for (auto it = connectedPorts.begin(); it != connectedPorts.end();)
    {
        if (!it->getConnection().assigned())
        {
            reader.removeInput(it->getGlobalId());

            cachedDescriptors.erase(it->getGlobalId());
            cachedSignalNames.erase(it->getGlobalId());
            this->inputPorts.removeItem(*it);
            it = connectedPorts.erase(it);
            connectedPortsChanged = true;
        }
        else
        {
            ++it;
        }
    }

    if (!disconnectedPort.assigned())
    {
        createDisconnectedPort();

        // Add the empty port to the multi reader and mark it unused
        reader.addInput(disconnectedPort);
        reader.setInputUnused(disconnectedPort.getGlobalId(), true);
    }

    if (connectedPorts.empty())
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, "No signals connected!");
        return false;
    }

    return connectedPortsChanged;
}

void MultiCsvRecorderImpl::createReader()
{
    reader.dispose();
    auto builder = MultiReaderBuilder()
                       .setDomainReadType(SampleType::Int64)
                       .setValueReadType(SampleType::Float64)
                       .setAllowDifferentSamplingRates(false)
                       .setInputPortNotificationMethod(notificationMode);

    if (disconnectedPort.assigned())
    {
        builder.addInputPort(disconnectedPort);
    }
    reader = builder.build();

    reader.setExternalListener(this->thisPtr<InputPortNotificationsPtr>());
    auto thisWeakRef = this->template getWeakRefInternal<IFunctionBlock>();
    reader.setOnDataAvailable(
        [this, thisWeakRef = std::move(thisWeakRef)]
        {
            const auto thisFb = thisWeakRef.getRef();
            if (thisFb.assigned())
                this->onDataReceived();
        });
}

void MultiCsvRecorderImpl::configureWriter(const DataDescriptorPtr& domainDescriptor,
                                           const ListPtr<IDataDescriptor>& valueDescriptors,
                                           const ListPtr<IString>& signalNames)
{
    try
    {
        if (!recoverReaderIfNecessary())
        {
            throw std::runtime_error("Reader failed to recover from invalid state");
        }

        if (!domainDescriptor.assigned() || domainDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("Input domain descriptor is not set");
        }

        if (valueDescriptors.getCount() != connectedPorts.size())
        {
            throw std::runtime_error("Missing input value descriptors!");
        }

        recorderDomainDataDescriptor = domainDescriptor;

        if (!reader.asPtr<IReaderConfig>(true).getIsValid())
        {
            throw std::runtime_error("Signal reader invalid.");
        }

        setComponentStatus(ComponentStatus::Ok);
        reader.setActive(True);

        if (!filePath.has_value())
        {
            return;
        }
        fs::path outputFile = getNextCsvFilename(filePath.value(), fileBasename, timestampEnabled);

        // Replace the csv writer (can it ever survive a reconfigure?)
        writer.emplace(outputFile);
        writer.value().setHeaderInformation(recorderDomainDataDescriptor, valueDescriptors, signalNames, writeDomain);

        // Auto resume recording if recording was stopped internally.
        if (recoverToActive)
            startRecordingInternal();
    }
    catch (const std::exception& e)
    {
        stopRecordingInternal(true);
        setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Failed to configure CSV recorder: {}", e.what()));
        reader.setActive(False);
    }
}

void MultiCsvRecorderImpl::reconfigureWriter()
{
    auto descriptorList = List<IDataDescriptor>();
    auto signalNameList = List<IString>();
    for (const auto& descriptor : cachedDescriptors)
    {
        descriptorList.pushBack(descriptor.second);
        signalNameList.pushBack(cachedSignalNames[descriptor.first]);
    }
    if (descriptorList.getCount() > 0)
    {
        configureWriter(recorderDomainDataDescriptor, descriptorList, signalNameList);
    }
}

bool MultiCsvRecorderImpl::recoverReaderIfNecessary()
{
    if (reader.asPtr<IReaderConfig>().getIsValid())
        return true;

    LOG_D("Sum Reader FB: Attempting reader recovery")
    reader = MultiReaderFromExisting(reader, SampleType::Float64, SampleType::Int64);
    return reader.asPtr<IReaderConfig>().getIsValid();
}

void MultiCsvRecorderImpl::onPropertiesChanged()
{
    filePath = static_cast<std::string>(objPtr.getPropertyValue(Props::DIR));
    fileBasename = static_cast<std::string>(objPtr.getPropertyValue(Props::BASENAME));
    timestampEnabled = static_cast<bool>(objPtr.getPropertyValue(Props::FILE_TIMESTAMP_ENABLED));
    writeDomain = static_cast<bool>(objPtr.getPropertyValue(Props::WRITE_DOMAIN));

    reconfigureWriter();
}

void MultiCsvRecorderImpl::onConnected(const InputPortPtr& inputPort)
{
    auto lock = this->getAcquisitionLock2();

    LOG_I("Multi CSV Recorder: Input port {} connected", inputPort.getLocalId())

    updateInputPorts();
}

void MultiCsvRecorderImpl::onDisconnected(const InputPortPtr& inputPort)
{
    auto lock = this->getAcquisitionLock2();

    LOG_I("Sum Reader FB: Input port {} disconnected", inputPort.getLocalId())
    if (updateInputPorts())
    {
        reconfigureWriter();
    }
}

void MultiCsvRecorderImpl::onDataReceived()
{
    auto lock = this->getAcquisitionLock2();

    const MultiReaderStatusPtr status = attemptReadData();

    // Return if there is no event to handle
    if (status.getReadStatus() != ReadStatus::Event)
    {
        return;
    }

    DataDescriptorPtr domainDescriptor;
    ListPtr<IDataDescriptor> valueDescriptors = List<IDataDescriptor>();
    ListPtr<IString> signalNames = List<IString>();

    bool domainChanged = false;
    bool valueSigChanged = false;

    const auto eventPackets = status.getEventPackets();
    for (const auto& port : connectedPorts)
    {
        auto portGlobalId = port.getGlobalId();
        DataDescriptorPtr valueDescriptor;
        if (eventPackets.hasKey(portGlobalId))
        {
            getDataDescriptors(eventPackets.get(portGlobalId), valueDescriptor, domainDescriptor);

            if (descriptorNotNull(valueDescriptor))
            {
                valueSigChanged |= !valueDescriptorsEqual(valueDescriptor, cachedDescriptors[portGlobalId]);
                cachedDescriptors[portGlobalId] = valueDescriptor;
            }

            // NOTE: Domain descriptors of individual signals don't affect csv header
        }

        // Build a collection of all descriptors and corresponding signal names
        valueDescriptors.pushBack(cachedDescriptors[portGlobalId]);
        signalNames.pushBack(cachedSignalNames[portGlobalId]);
    }

    if (getDomainDescriptor(status.getMainDescriptor(), domainDescriptor))
    {
        domainChanged |= !domainDescriptorsEqual(domainDescriptor, recorderDomainDataDescriptor);
    }

    if (valueSigChanged || domainChanged || !status.getValid())
        configureWriter(domainDescriptor, valueDescriptors, signalNames);
}

void MultiCsvRecorderImpl::stopRecordingInternal(bool recover)
{
    // This is the only method recording should be stopped by. If already stopped, nothing to do.
    if (!recordingActive)
    {
        return;
    }
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

MultiReaderStatusPtr MultiCsvRecorderImpl::attemptReadData()
{
    SizeT cnt = reader.getAvailableCount();

    // +1: Disconnected port is added to the reader but unused
    auto numPorts = connectedPorts.size() + 1;
    std::vector<std::unique_ptr<double[]>> samples;
    samples.reserve(numPorts);

    for (size_t i = 0; i < numPorts; ++i)
        samples.push_back(std::make_unique<double[]>(cnt));

    MultiReaderStatusPtr status = reader.read(samples.data(), &cnt);

    // Write samples if read successful
    if (recordingActive && writer.has_value() && cnt > 0)
    {
        Int packetOffset = status.asPtr<IReaderStatus>().getOffset().getIntValue();
        samples.pop_back();  // Remove last buffer (unused disconnected port)
        writer.value().writeSamples(std::move(samples), cnt, packetOffset);
    }
    return status;
}

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
