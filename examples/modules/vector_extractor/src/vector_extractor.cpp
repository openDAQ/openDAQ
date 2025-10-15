#include <vector_extractor/vector_extractor.h>

BEGIN_NAMESPACE_VECTOR_EXTRACTOR_MODULE

VectorExtractorImpl::VectorExtractorImpl(const daq::ContextPtr& ctx,
                                                                        const daq::ComponentPtr& parent,
                                                                        const daq::StringPtr& localId)
    : daq::FunctionBlock(CreateType(), ctx, parent, localId)
{
    initComponentStatus();
    createInputPorts();
    createSignals();
    initStatues();
}


// Init Properties (add beginning of memory and the type of output)
// #include <bit> (for std::bit_cast)
// Implement reader

// Fix the configuration

// Rewrite onPacketReceived into onDataReceived

// Check if we are missing any important functions from the default FunctionBlock class

// The length of the data should be devisible by all available data types (afaik 64 bytes should do it (it is used now))

daq::FunctionBlockTypePtr VectorExtractorImpl::CreateType()
{
    return daq::FunctionBlockType("VectorExtractor", "CAN FD data decoder", "CAN FD data array decoder");
}

void VectorExtractorImpl::onDataReceived()
{

}

void VectorExtractorImpl::onPacketReceived(const daq::InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock();

    const auto connection = inputPort.getConnection();
    if (!connection.assigned())
        return;

    daq::PacketPtr packet = connection.dequeue();

    while (packet.assigned())
    {
        switch (packet.getType())
        {
            case daq::PacketType::Event:
                processEventPacket(packet);
                break;

            case daq::PacketType::Data:
                processDataPacket(packet);
                break;

            default:
                break;
        }

        packet = connection.dequeue();
    }
}

void VectorExtractorImpl::onDisconnected(const daq::InputPortPtr& port)
{
    auto lock = this->getRecursiveConfigLock();

    signals.clear();

    configured = false;

    setInputStatus(InputDisconnected);
}

void VectorExtractorImpl::createInputPorts()
{
    inputPort = daq::FunctionBlock::createAndAddInputPort("Input", daq::PacketReadyNotification::SchedulerQueueWasEmpty);
}

void VectorExtractorImpl::processDataPacket(const daq::DataPacketPtr& packet) const
{
    if (!configured)
        return;

    const auto inputData = static_cast<uint8_t*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();
    const auto domainPacket = packet.getDomainPacket();

    const auto structFields = inputDataDescriptor.getStructFields();

    outputDomainSignal.sendPacket(domainPacket);

    size_t offset = 0;
    for (size_t i = 0; i < structFields.getCount(); ++i)
    {
        const auto field = structFields[i];
        if (field.getName() == "Data")
        {
            const auto outputPacket = DataPacketWithDomain(domainPacket, outputDataDescriptor, sampleCount);
            const auto outputData = static_cast<uint8_t*>(outputPacket.getRawData());
            const auto fieldSampleSize = outputDataDescriptor.getRawSampleSize();
            copySamples(outputData, inputData + offset + 1, fieldSampleSize, sampleCount);
            outputSignal.sendPacket(outputPacket);
            offset += field.getRawSampleSize();
            continue;
        }

        const auto fieldSampleSize = field.getRawSampleSize();
        offset += fieldSampleSize;
    }
}

void VectorExtractorImpl::processEventPacket(const daq::EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        processSignalDescriptorsChangedEventPacket(packet);
    }
}

void VectorExtractorImpl::processSignalDescriptorsChangedEventPacket(const daq::EventPacketPtr& eventPacket)
{
    const auto [valueDescriptorChanged, domainDescriptorChanged, newValueDescriptor, newDomainDescriptor] =
        daq::parseDataDescriptorEventPacket(eventPacket);
    if (valueDescriptorChanged)
        this->inputDataDescriptor = newValueDescriptor;
    if (domainDescriptorChanged)
        this->inputDomainDescriptor = newDomainDescriptor;

    configure();
}

void VectorExtractorImpl::configure()
{
    if (!inputDataDescriptor.assigned() || !inputDomainDescriptor.assigned())
    {
        configured = false;
        setInputStatus(InputInvalid);
        return;
    }

    try
    {
        if (inputDataDescriptor.getDimensions().getCount() > 0)
        {
            throw std::runtime_error("Arrays not supported");
        }

        const auto inputSampleType = inputDataDescriptor.getSampleType();
        if (inputSampleType != daq::SampleType::Struct)
        {
            throw std::runtime_error("Invalid sample type");
        }

        // These SampleTypes should be reassessed

        constexpr std::array<daq::SampleType, 11> validFieldTypes{
            daq::SampleType::Int8,
            daq::SampleType::Int16,
            daq::SampleType::Int32,
            daq::SampleType::Int64,
            daq::SampleType::UInt8,
            daq::SampleType::UInt16,
            daq::SampleType::UInt32,
            daq::SampleType::UInt64,
            daq::SampleType::Float32,
            daq::SampleType::Float64,
            daq::SampleType::Struct,
        };

        const auto structFields = inputDataDescriptor.getStructFields();
        for (const auto& field : structFields)
        {
            const auto fieldSampleType = field.getSampleType();
            if (std::find(validFieldTypes.begin(), validFieldTypes.end(), fieldSampleType) == validFieldTypes.end())
            {
                throw std::runtime_error(fmt::format("Field \"{}\" has invalid sample type", field.getName()));
            }
        }

        // 
        // Here an output and unit check need to be constructed
        // 

        outputDataDescriptor = daq::DataDescriptorBuilder()
                                    .setSampleType(daq::SampleType::UInt8)
                                    .build();
        outputDomainDescriptor = inputDomainDescriptor;

        outputSignal.setDescriptor(outputDataDescriptor);
        outputDomainSignal.setDescriptor(outputDomainDescriptor);

        structSize = inputDataDescriptor.getSampleSize();

        configured = true;
        setInputStatus(InputConnected);
        setComponentStatus(daq::ComponentStatus::Ok);
    }
    catch (const std::exception& e)
    {
        configured = false;
        setInputStatus(InputInvalid);
        setComponentStatusWithMessage(daq::ComponentStatus::Error, fmt::format("Failed to configure output signals: {}", e.what()));
        signals.clear();
    }
}

void VectorExtractorImpl::initStatues() const
{
    const auto inputStatusType =
        daq::EnumerationType("InputStatusType", daq::List<daq::IString>(InputDisconnected, InputConnected, InputInvalid));

    try
    {
        this->context.getTypeManager().addType(inputStatusType);
    }
    catch (const std::exception& e)
    {
        setComponentStatusWithMessage(daq::ComponentStatus::Warning,
                                        fmt::format("Couldn't add type {} to type manager: {}", inputStatusType.getName(), e.what()));
    }
    catch (...)
    {
        setComponentStatusWithMessage(daq::ComponentStatus::Warning,
                                        fmt::format("Couldn't add type {} to type manager!", inputStatusType.getName()));
    }

    const auto thisStatusContainer = this->statusContainer.asPtr<daq::IComponentStatusContainerPrivate>();

    const auto inputStatusValue = daq::Enumeration("InputStatusType", InputDisconnected, context.getTypeManager());
    thisStatusContainer.addStatus("InputStatus", inputStatusValue);
}

void VectorExtractorImpl::setInputStatus(const daq::StringPtr& value) const
{
    const auto thisStatusContainer = this->statusContainer.asPtr<daq::IComponentStatusContainerPrivate>();

    const auto inputStatusValue = daq::Enumeration("InputStatusType", value, context.getTypeManager());
    thisStatusContainer.setStatus("InputStatus", inputStatusValue);
}

template <typename T>
static void VectorExtractorImpl::copySample(uint8_t* dest, const uint8_t* source)
{
    *(reinterpret_cast<T*>(dest)) = *(reinterpret_cast<const T*>(source));
}

void VectorExtractorImpl::copySamples(uint8_t* dest,
                                                        uint8_t* source,
                                                        const size_t fieldSampleSize,
                                                        size_t sampleCount) const
{
    switch (fieldSampleSize)
    {
        case 1:
        {
            for (size_t i = 0; i < sampleCount; i++)
            {
                copySample<uint8_t>(dest, source);
                dest += fieldSampleSize;
                source += structSize;
            }
            break;
        }
        case 2:
        {
            for (size_t i = 0; i < sampleCount; i++)
            {
                copySample<uint16_t>(dest, source);
                dest += fieldSampleSize;
                source += structSize;
            }
            break;
        }
        case 4:
        {
            for (size_t i = 0; i < sampleCount; i++)
            {
                copySample<uint32_t>(dest, source);
                dest += fieldSampleSize;
                source += structSize;
            }
            break;
        }
        case 8:
        {
            for (size_t i = 0; i < sampleCount; i++)
            {
                copySample<uint64_t>(dest, source);
                dest += fieldSampleSize;
                source += structSize;
            }
            break;
        }
        default:
            for (size_t i = 0; i < sampleCount; i++)
            {
                std::memcpy(dest, source, fieldSampleSize);
                dest += fieldSampleSize;
                source += structSize;
            }
    }
}

void VectorExtractorImpl::createSignals()
{
    outputSignal = createAndAddSignal("RPM_counter");
    outputDomainSignal = createAndAddSignal("VectorDecoderTime", inputDomainDescriptor, false);
    outputSignal.setDomainSignal(outputDomainSignal);
}

END_NAMESPACE_VECTOR_EXTRACTOR_MODULE

