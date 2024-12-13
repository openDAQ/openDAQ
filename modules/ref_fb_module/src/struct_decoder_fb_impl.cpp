#include <ref_fb_module/struct_decoder_fb_impl.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/custom_log.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/data_packet.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/packet_factory.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/search_filter_factory.h>

BEGIN_NAMESPACE_REF_FB_MODULE
namespace StructDecoder
{

static const char* InputDisconnected = "Disconnected";
static const char* InputConnected = "Connected";
static const char* InputInvalid = "Invalid";

StructDecoderFbImpl::StructDecoderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , configured(false)
{
    initComponentErrorStateStatus();
    createInputPorts();
    initStatuses();
}

FunctionBlockTypePtr StructDecoderFbImpl::CreateType()
{
    return FunctionBlockType("RefFBModuleStructDecoder", "Struct decoder", "Decodes signals with struct data type and outputs signal for each struct component.");
}

void StructDecoderFbImpl::processSignalDescriptorsChangedEventPacket(const EventPacketPtr& eventPacket)
{
    const auto [valueDescriptorChanged, domainDescriptorChanged, newValueDescriptor, newDomainDescriptor] =
        parseDataDescriptorEventPacket(eventPacket);
    if (valueDescriptorChanged)
        this->inputDataDescriptor = newValueDescriptor;
    if (domainDescriptorChanged)
        this->inputDomainDataDescriptor = newDomainDescriptor;

    configure();
}

void StructDecoderFbImpl::configure()
{
    if (!inputDataDescriptor.assigned() || !inputDomainDataDescriptor.assigned())
    {
        configured = false;
        setInputStatus(InputInvalid);
        return;
    }

    try
    {
        if (inputDataDescriptor.getDimensions().getCount() > 0)
        {
            setComponentErrorStateStatusWithMessage(ComponentErrorState::Error, "Arrays not supported");
            throw std::runtime_error("Arrays not supported");
        }

        const auto inputSampleType = inputDataDescriptor.getSampleType();
        if (inputSampleType != SampleType::Struct)
        {
            setComponentErrorStateStatusWithMessage(ComponentErrorState::Error, "Invalid sample type");
            throw std::runtime_error("Invalid sample type");
        }

        signals.clear();

        constexpr std::array<SampleType, 11> validFieldTypes{
            SampleType::Int8,
            SampleType::Int16,
            SampleType::Int32,
            SampleType::Int64,
            SampleType::UInt8,
            SampleType::UInt16,
            SampleType::UInt32,
            SampleType::UInt64,
            SampleType::Float32,
            SampleType::Float64,
            SampleType::Struct,
        };

        const auto domainSignal = createAndAddSignal("__domain", inputDomainDataDescriptor, false);

        const auto structFields = inputDataDescriptor.getStructFields();
        for (const auto& field: structFields)
        {
            const auto fieldSampleType = field.getSampleType();
            if (std::find(validFieldTypes.begin(), validFieldTypes.end(), fieldSampleType) == validFieldTypes.end())
            {
                setComponentErrorStateStatusWithMessage(ComponentErrorState::Error, "Field has invalid sample type");
                throw std::runtime_error(fmt::format("Field \"{}\" has invalid sample type", field.getName()));
            }

            const auto signal = createAndAddSignal(field.getName(), field);
            signal.setDomainSignal(domainSignal);
        }

        structSize = inputDataDescriptor.getSampleSize();

        configured = true;
        setInputStatus(InputConnected);
    }
    catch (const std::exception& e)
    {
        configured = false;
        setInputStatus(InputInvalid);
        setComponentErrorStateStatusWithMessage(ComponentErrorState::Warning, "Failed to configure output signals");
        LOG_W("Failed to configure output signals: {}", e.what())
        signals.clear();
    }
}


void StructDecoderFbImpl::onPacketReceived(const InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock();

    const auto connection = inputPort.getConnection();
    if (!connection.assigned())
        return;

    PacketPtr packet = connection.dequeue();

    while (packet.assigned())
    {
        switch (packet.getType())
        {
            case PacketType::Event:
                processEventPacket(packet);
                break;

            case PacketType::Data:
                processDataPacket(packet);
                break;

            default:
                break;
        }

        packet = connection.dequeue();
    }
}

void StructDecoderFbImpl::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
        processSignalDescriptorsChangedEventPacket(packet);
}

template <typename T>
void StructDecoderFbImpl::copySample(uint8_t* dest, const uint8_t* source)
{
    *(reinterpret_cast<T*>(dest)) = *(reinterpret_cast<const T*>(source));
}

void StructDecoderFbImpl::copySamples(uint8_t* dest, uint8_t* source, const size_t fieldSampleSize, size_t sampleCount) const
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

void StructDecoderFbImpl::processDataPacket(const DataPacketPtr& packet) const
{
    if (!configured)
        return;

    const auto inputData = static_cast<uint8_t*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();
    const auto domainPacket = packet.getDomainPacket();

    const auto structFields = inputDataDescriptor.getStructFields();
    const auto signals = this->signals.getItems(search::Any());

    signals[0].asPtr<ISignalConfig>(true).sendPacket(domainPacket);

    size_t offset = 0;
    for (size_t i = 0; i < structFields.getCount(); ++i)
    {
        const auto field = structFields[i];
        const auto signal = signals[i + 1].asPtr<ISignalConfig>(true);

        const auto outputPacket = DataPacketWithDomain(domainPacket, field, sampleCount);
        const auto outputData = static_cast<uint8_t*>(outputPacket.getRawData());

        const auto fieldSampleSize = field.getRawSampleSize();
        copySamples(outputData, inputData + offset, fieldSampleSize, sampleCount);

        signal.sendPacket(outputPacket);

        offset += fieldSampleSize;
    }
}

void StructDecoderFbImpl::createInputPorts()
{
    inputPort = createAndAddInputPort("Input", PacketReadyNotification::SchedulerQueueWasEmpty);
}

void StructDecoderFbImpl::initStatuses() const
{
    const auto inputStatusType = EnumerationType("InputStatusType", List<IString>(InputDisconnected, InputConnected, InputInvalid));

    try
    {
        this->context.getTypeManager().addType(inputStatusType);
    }
    catch (const std::exception& e)
    {
        setComponentErrorStateStatusWithMessage(ComponentErrorState::Warning, "Couldn't add type to type manager");
        const auto loggerComponent = this->context.getLogger().getOrAddComponent("ScalingFunctionBlock");
        LOG_W("Couldn't add type {} to type manager: {}", inputStatusType.getName(), e.what());
    }
    catch (...)
    {
        setComponentErrorStateStatusWithMessage(ComponentErrorState::Warning, "Couldn't add type to type manager");
        const auto loggerComponent = this->context.getLogger().getOrAddComponent("ScalingFunctionBlock");
        LOG_W("Couldn't add type {} to type manager!", inputStatusType.getName());
    }

    const auto thisStatusContainer = this->statusContainer.asPtr<IComponentStatusContainerPrivate>();

    const auto inputStatusValue = Enumeration("InputStatusType", InputDisconnected, context.getTypeManager());
    thisStatusContainer.addStatus("InputStatus", inputStatusValue);
}

void StructDecoderFbImpl::setInputStatus(const StringPtr& value) const
{
    const auto thisStatusContainer = this->statusContainer.asPtr<IComponentStatusContainerPrivate>();

    const auto inputStatusValue = Enumeration("InputStatusType", value, context.getTypeManager());
    thisStatusContainer.setStatus("InputStatus", inputStatusValue);
}

void StructDecoderFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    auto lock = this->getRecursiveConfigLock();

    signals.clear();

    configured = false;

    setInputStatus(InputDisconnected);
}

}

END_NAMESPACE_REF_FB_MODULE
