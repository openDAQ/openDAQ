#include <opendaq/event_packet_params.h>
#include <ref_fb_module/dispatch.h>
#include <ref_fb_module/trigger_fb_impl.h>
#include "opendaq/packet_factory.h"
#include "opendaq/sample_type_traits.h"

BEGIN_NAMESPACE_REF_FB_MODULE

namespace Trigger
{

TriggerFbImpl::TriggerFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initComponentStatus();

    state = false;

    if (config.assigned() && config.hasProperty("UseMultiThreadedScheduler") && !config.getPropertyValue("UseMultiThreadedScheduler"))
        packetReadyNotification = PacketReadyNotification::SameThread;
    else
        packetReadyNotification = PacketReadyNotification::Scheduler;

    createInputPorts();
    createSignals();
    initProperties();
}

void TriggerFbImpl::initProperties()
{
    const auto thresholdProp = FloatProperty("Threshold", 0.5);
    objPtr.addProperty(thresholdProp);
    objPtr.getOnPropertyValueWrite("Threshold") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    readProperties();
}

void TriggerFbImpl::propertyChanged()
{
    readProperties();
}

void TriggerFbImpl::readProperties()
{
    threshold = objPtr.getPropertyValue("Threshold");
}

FunctionBlockTypePtr TriggerFbImpl::CreateType()
{
    auto defaultConfig = PropertyObject();
    defaultConfig.addProperty(BoolProperty("UseMultiThreadedScheduler", true));

    return FunctionBlockType("RefFBModuleTrigger", "Trigger", "Trigger", defaultConfig);
}

void TriggerFbImpl::processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor,
                                                   const DataDescriptorPtr& inputDomainDataDescriptor)
{
    if (inputDataDescriptor.assigned())
        this->inputDataDescriptor = inputDataDescriptor;
    if (inputDomainDataDescriptor.assigned())
        this->inputDomainDataDescriptor = inputDomainDataDescriptor;

    configure();
}

void TriggerFbImpl::configure()
{
    if (!inputDataDescriptor.assigned() || !inputDomainDataDescriptor.assigned())
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, "Incomplete signal descriptors");
        return;
    }

    try
    {
        if (inputDataDescriptor.getDimensions().getCount() > 0)
        {
            throw std::runtime_error("Arrays not supported");
        }

        inputSampleType = inputDataDescriptor.getSampleType();
        if (inputSampleType != SampleType::Float64 && inputSampleType != SampleType::Float32 && inputSampleType != SampleType::Int8 &&
            inputSampleType != SampleType::Int16 && inputSampleType != SampleType::Int32 && inputSampleType != SampleType::Int64 &&
            inputSampleType != SampleType::UInt8 && inputSampleType != SampleType::UInt16 && inputSampleType != SampleType::UInt32 &&
            inputSampleType != SampleType::UInt64)
        {
            throw std::runtime_error("Invalid sample type");
        }

        outputDataDescriptor = DataDescriptorBuilder().setSampleType(SampleType::UInt8).setValueRange(Range(0, 1)).build();
        outputSignal.setDescriptor(outputDataDescriptor);

        outputDomainDataDescriptor = DataDescriptorBuilderCopy(inputDomainDataDescriptor).setRule(ExplicitDataRule()).build();
        outputDomainSignal.setDescriptor(outputDomainDataDescriptor);

        setComponentStatus(ComponentStatus::Ok);
    }
    catch (const std::exception& e)
    {
        setComponentStatusWithMessage(ComponentStatus::Error, fmt::format("Failed to set descriptor for trigger signal: {}", e.what()));
        outputSignal.setDescriptor(nullptr);
    }
}

void TriggerFbImpl::onPacketReceived(const InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock();

    PacketPtr packet;
    const auto connection = inputPort.getConnection();
    if (!connection.assigned())
        return;

    packet = connection.dequeue();

    while (packet.assigned())
    {
        switch (packet.getType())
        {
            case PacketType::Event:
                processEventPacket(packet);
                break;

            case PacketType::Data:
                SAMPLE_TYPE_DISPATCH(inputSampleType, processDataPacket, packet);
                break;

            default:
                break;
        }

        packet = connection.dequeue();
    };
}

void TriggerFbImpl::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        // TODO handle Null-descriptor params ('Null' sample type descriptors)
        DataDescriptorPtr inputDataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr inputDomainDataDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        processSignalDescriptorChanged(inputDataDescriptor, inputDomainDataDescriptor);
    }
}

void TriggerFbImpl::trigger(const DataPacketPtr& inputPacket, size_t triggerIndex)
{
    // Flip state
    state = !state;

    Int triggeredAt = -1;
    auto inputDomainPacket = inputPacket.getDomainPacket();

    // Get value of domain packet data at sample i (when triggered)
    auto domainDataValues = static_cast<daq::Int*>(inputDomainPacket.getData());
    triggeredAt = static_cast<daq::Int>(domainDataValues[triggerIndex]);

    // Create output domain packet
    auto outputDomainPacket = DataPacket(outputDomainDataDescriptor, 1);
    auto domainPacketData = static_cast<daq::Int*>(outputDomainPacket.getData());
    *domainPacketData = triggeredAt;

    // Create output data packet
    auto dataPacket = DataPacketWithDomain(outputDomainPacket, outputDataDescriptor, 1);
    auto packetData = static_cast<daq::Bool*>(dataPacket.getData());
    *packetData = static_cast<daq::Bool>(state);

    // Send packets
    outputDomainSignal.sendPacket(outputDomainPacket);
    outputSignal.sendPacket(dataPacket);
}

template <SampleType InputSampleType>
void TriggerFbImpl::processDataPacket(const DataPacketPtr& packet)
{
    using InputType = typename SampleTypeToType<InputSampleType>::Type;
    auto inputData = static_cast<InputType*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();

    for (size_t i = 0; i < sampleCount; i++)
    {
        Float value = static_cast<Float>(*inputData++);
        if (state)
        {
            if (value < threshold)
            {
                trigger(packet, i);
            }
        }
        else
        {
            if (value >= threshold)
            {
                trigger(packet, i);
            }
        }
    }
}

void TriggerFbImpl::createInputPorts()
{
    inputPort = createAndAddInputPort("Input", packetReadyNotification);
}

void TriggerFbImpl::createSignals()
{
    outputSignal = createAndAddSignal(String("output"));
    outputDomainSignal = createAndAddSignal(String("output_domain"));
    outputSignal.setDomainSignal(outputDomainSignal);
}
}

END_NAMESPACE_REF_FB_MODULE
