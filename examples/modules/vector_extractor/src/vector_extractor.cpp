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


void VectorExtractorImpl::initProperties()
{
    const auto typeSelected =
        SelectionProperty("OutputType", List<SampleType>(SampleType::UInt8, SampleType::UInt16, SampleType::UInt32, SampleType::UInt64), 1);
    objPtr.addProperty(typeSelected);
    objPtr.getOnPropertyValueWrite("OutpuType") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(false); };

    const auto indexOfMemoryBeginning = IntProperty("BeginningSpace", 0);
    objPtr.addProperty(indexOfMemoryBeginning);
    objPtr.getOnPropertyValueWrite("BeginningSpace") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(false); };

    readProperties();
}

void VectorExtractorImpl::readProperties()
{
    outputType = objPtr.getPropertyValue("typeSelected");
    beginningIndex = objPtr.getPropertyValue("BeginningSpace");
}

daq::FunctionBlockTypePtr VectorExtractorImpl::CreateType()
{
    return daq::FunctionBlockType("VectorExtractor", "CAN FD data decoder", "CAN FD data array decoder");
}

void VectorExtractorImpl::onDataReceived()
{
    SizeT cnt = reader.getAvailableCount();
    std::unique_ptr bufferData = std::make_unique<uint8_t[]>(cnt);

    auto status = reader.read(&bufferData, &cnt);

    if (cnt > 0)
    {
        const auto domainPacket = DataPacket(outputDomainSignal.getDescriptor(), cnt, status.getOffset());
        const auto valuePacket = DataPacketWithDomain(domainPacket, outputSignal.getDescriptor(), cnt);

        // Missing check for smaples being divisable

        castingFunction(valuePacket.getDataDescriptor(), &bufferData, valuePacket.getData(), cnt);

        outputDomainSignal.sendPacket(domainPacket);
        outputSignal.sendPacket(valuePacket);
    }

    if (status.getReadStatus() == ReadStatus::Event)
    {
        const auto eventPacket = status.getEventPacket();
        if (eventPacket.assigned())
        {
            DataDescriptorPtr domainDescriptor;
            DataDescriptorPtr dataDescriptor;

            bool domainChanged = false;

            getDataDescriptor(status.getEventPacket(), dataDescriptor);
            domainChanged |= descriptorNotNull(dataDescriptor);

            getDomainDescriptor(status.getEventPacket(), domainDescriptor);


            if (dataDescriptor.assigned() || domainDescriptor.assigned() || domainChanged)
            {
                outputDataDescriptor = dataDescriptor;
                outputDomainDescriptor = domainDescriptor;
                configure();
            }
        }

        if (!status.getValid())
        {
            reader = StreamReaderFromExisting(reader, SampleType::UInt8, SampleType ::Int64);
        }
    }
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
    auto lock = this->getRecursiveConfigLock2();

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
    // This need further examination...
    if (valueDescriptorChanged)
        this->inputDataDescriptor = newValueDescriptor;
    if (domainDescriptorChanged)
        this->inputDomainDescriptor = newDomainDescriptor;

    configure();
}

void VectorExtractorImpl::createReader()
{
    reader.dispose();

    reader = StreamReader(inputPort);

    reader.setExternalListener(this->objPtr);
    auto thisWeakRef = this->template getWeakRefInternal<IFunctionBlock>();
    reader.setOnDataAvailable([this, thisWeakRef = std::move(thisWeakRef)]
        {
            const auto thisFb = thisWeakRef.getRef();
            if (thisFb.assigned())
                this->onDataReceived();
        });
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

        /*
        * Note: This is now outside of scope because the input paradigm has shifted away from raw CAN dataframes
        * 
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
        } */

        // Changes need to be handled in they come from outside, use of function arguments should be fine
        outputDataDescriptor = daq::DataDescriptorBuilder()
                                    .setSampleType(outputType)
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


// Q: Why is this one no longer used in sum_reader_fb??
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

void VectorExtractorImpl::castingFunction(const DataDescriptorPtr& dataDescriptor, void* bufferData, void* destinationData, SizeT count)
{
    auto type = dataDescriptor.getSampleType();

    // This is a bit C-like solution to casting and don't particulary like it, but ok...

    auto check = reinterpret_cast<uint8_t*>(bufferData);

    auto sizeOfSample = inputDataDescriptor.getRawSampleSize();

    switch (type)
    {
        case (SampleType::UInt16):
        {
            auto ff = reinterpret_cast<uint16_t*>(destinationData);
            for (int i = 0; i< count; i++)
                ff[i] = (check[i*sizeOfSample + beginningIndex] << 8) |
                        (check[i*sizeOfSample + beginningIndex + 1] << 0);
            break;
        }
        case (SampleType::UInt32):
        {
            auto ff = reinterpret_cast<uint32_t*>(destinationData);
            for (int i = 0; i< count; i++)
                ff[i] = (check[i*sizeOfSample + beginningIndex] << 24) |
                        (check[i*sizeOfSample + beginningIndex + 1] << 16) |
                        (check[i*sizeOfSample + beginningIndex + 2] << 8) |
                        (check[i*sizeOfSample + beginningIndex + 3] << 0);
            break;
        }
        case (SampleType::UInt64):
        {
            auto ff = reinterpret_cast<uint64_t*>(destinationData);
            for (int i = 0; i < count; i++)
                ff[i] = (check[i*sizeOfSample + beginningIndex] << 56) |
                        (check[i*sizeOfSample + beginningIndex + 1] << 48) |
                        (check[i*sizeOfSample + beginningIndex + 2] << 40) |
                        (check[i*sizeOfSample + beginningIndex + 3] << 32) |
                        (check[i*sizeOfSample + beginningIndex + 4] << 24) |
                        (check[i*sizeOfSample + beginningIndex + 5] << 16) |
                        (check[i*sizeOfSample + beginningIndex + 6] << 8) |
                        (check[i*sizeOfSample + beginningIndex + 7] << 0);
            break;
        }
    }
}

END_NAMESPACE_VECTOR_EXTRACTOR_MODULE

