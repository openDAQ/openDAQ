#include <functional>
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
    initProperties();

    if (config.assigned())
        notificationMode = static_cast<PacketReadyNotification>(config.getPropertyValue("ReaderNotificationMode"));
    else
        notificationMode = PacketReadyNotification::Scheduler;

    updateInputPorts();
}

ErrCode MultiCsvRecorderImpl::startRecording()
{
    auto lock = getRecursiveConfigLock();
    recordingActive = true;
    reconfigure();

    return OPENDAQ_SUCCESS;
}

ErrCode MultiCsvRecorderImpl::stopRecording()
{
    auto lock = getRecursiveConfigLock();
    recordingActive = false;
    reconfigure();

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

    objPtr.addProperty(StringProperty(Props::PATH, ""));
    objPtr.getOnPropertyValueWrite(Props::PATH) += std::bind(&MultiCsvRecorderImpl::onPathChanged, this);
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

bool MultiCsvRecorderImpl::updateInputPorts()
{
    bool rebuildReader = false;
    if (disconnectedPort.assigned() && disconnectedPort.getConnection().assigned())
    {
        connectedPorts.emplace_back(disconnectedPort);
        cachedDescriptors.insert(std::make_pair(disconnectedPort.getGlobalId(), NullDataDescriptor()));
        disconnectedPort.release();
        rebuildReader = true;
    }

    for (auto it = connectedPorts.begin(); it != connectedPorts.end();)
    {
        if (!it->getConnection().assigned())
        {
            cachedDescriptors.erase(it->getGlobalId().toStdString());
            this->inputPorts.removeItem(*it);
            it = connectedPorts.erase(it);
            rebuildReader = true;
        }
        else
        {
            ++it;
        }
    }

    if (!disconnectedPort.assigned())
    {
        std::string id = getNextPortID();
        auto inputPort = createAndAddInputPort(id, notificationMode);
        inputPort.setListener(this->thisPtr<InputPortNotificationsPtr>());
        disconnectedPort = inputPort;
    }

    if (connectedPorts.empty())
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, "No signals connected!");
        reader = nullptr;
        return false;
    }

    return rebuildReader;
}

// Reader must currently be rebuilt to add/remove input ports
void MultiCsvRecorderImpl::updateReader()
{
    // Disposing the reader is necessary to release port ownership
    reader.dispose();
    auto builder = MultiReaderBuilder()
                       .setDomainReadType(SampleType::Int64)
                       .setValueReadType(SampleType::Float64)
                       .setAllowDifferentSamplingRates(false)
                       .setInputPortNotificationMethod(notificationMode);

    for (const auto& port : connectedPorts)
        builder.addInputPort(port);

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

    if (!reader.asPtr<IReaderConfig>(true).getIsValid())
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, "Unable to create a valid reader.");
    }
}

void MultiCsvRecorderImpl::configure(const DataDescriptorPtr& domainDescriptor, const ListPtr<IDataDescriptor>& valueDescriptors)
{
    // TODO: These value descripters need to be compiled into a header
    try
    {
        if (!domainDescriptor.assigned() || domainDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("Input domain descriptor is not set");
        }

        if (valueDescriptors.getCount() != connectedPorts.size())
        {
            throw std::runtime_error("Missing input value descriptors!");
        }

        sumDomainDataDescriptor = domainDescriptor;

        if (reader.asPtr<IReaderConfig>(true).getIsValid())
        {
            setComponentStatus(ComponentStatus::Ok);
            reader.setActive(True);
        }
        else
        {
            setComponentStatusWithMessage(ComponentStatus::Warning, "Configure unsuccessful, signal reader invalid.");
        }
    }
    catch (const std::exception& e)
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Failed to configure CSV recorder: {}", e.what()));
        reader.setActive(False);
    }

    if (this->statusContainer.getStatus("ComponentStatus") == ComponentStatus::Warning || !filePath.has_value())
    {
        writer = std::nullopt;
        return;
    }
    // TODO: Compute CSV header info

    // Replace the csv writer (can it ever survive a reconfigure?)
    writer.emplace(filePath.value());
}

void MultiCsvRecorderImpl::reconfigure()
{
    auto descriptorList = List<IDataDescriptor>();
    for (const auto& descriptor : cachedDescriptors)
        descriptorList.pushBack(descriptor.second);
    configure(sumDomainDataDescriptor, descriptorList);
}

void MultiCsvRecorderImpl::onPathChanged()
{
    filePath = static_cast<std::string>(objPtr.getPropertyValue(Props::PATH));
    reconfigure();
}

void MultiCsvRecorderImpl::onConnected(const InputPortPtr& inputPort)
{
    auto lock = this->getAcquisitionLock2();

    LOG_I("Sum Reader FB: Input port {} connected", inputPort.getLocalId())

    if (updateInputPorts())
    {
        updateReader();
    }

    if (!reader.asPtr<IReaderConfig>(true).getIsValid())
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, "Connected a signal with invalid domain.");
    }
}

void MultiCsvRecorderImpl::onDisconnected(const InputPortPtr& inputPort)
{
    auto lock = this->getAcquisitionLock2();

    LOG_I("Sum Reader FB: Input port {} disconnected", inputPort.getLocalId())
    if (updateInputPorts())
    {
        updateReader();
        reconfigure();
    }
}

void MultiCsvRecorderImpl::onDataReceived()
{
    auto lock = this->getAcquisitionLock2();

    SizeT cnt = reader.getAvailableCount();

    auto numPorts = connectedPorts.size();
    std::vector<std::unique_ptr<double[]>> data;
    data.reserve(numPorts);

    for (size_t i = 0; i < numPorts; ++i)
        data.push_back(std::make_unique<double[]>(cnt));

    const MultiReaderStatusPtr status = reader.read(data.data(), &cnt);

    if (cnt > 0)
    {
        if (recordingActive && writer.has_value())
        {
            writer.value().writeSamples(std::move(data), cnt);
        }
    }

    if (status.getReadStatus() != ReadStatus::Event)
    {
        return;
    }

    const auto eventPackets = status.getEventPackets();
    if (eventPackets.getCount() == 0)
    {
        return;
    }

    DataDescriptorPtr domainDescriptor;
    ListPtr<IDataDescriptor> valueDescriptors = List<IDataDescriptor>();

    bool domainChanged = false;
    bool valueSigChanged = false;

    for (const auto& port : connectedPorts)
    {
        auto portGlobalId = port.getGlobalId();
        DataDescriptorPtr valueDescriptor;
        if (eventPackets.hasKey(portGlobalId))
        {
            getDataDescriptors(eventPackets.get(portGlobalId), valueDescriptor, domainDescriptor);

            if (descriptorNotNull(valueDescriptor))
            {
                valueSigChanged = true;
                valueDescriptors.pushBack(valueDescriptor);
                cachedDescriptors[portGlobalId] = valueDescriptor;
            }

            domainChanged |= descriptorNotNull(domainDescriptor);
        }

        if (!descriptorNotNull(valueDescriptor))
            valueDescriptors.pushBack(cachedDescriptors[portGlobalId]);
    }

    getDomainDescriptor(status.getMainDescriptor(), domainDescriptor);

    if (valueSigChanged || domainChanged)
        configure(domainDescriptor, valueDescriptors);

    if (!status.getValid())
    {
        LOG_D("Multi CSV Recorder: Attempting reader recovery")
        reader = MultiReaderFromExisting(reader, SampleType::Float64, SampleType::Int64);
        if (!reader.asPtr<IReaderConfig>().getIsValid())
        {
            setComponentStatusWithMessage(ComponentStatus::Warning, "Reader failed to recover from invalid state!");
        }
    }
}

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
