#include <ref_fb_module/scaling_fb_impl.h>
#include <ref_fb_module/dispatch.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/data_descriptor_ptr.h>

#include <opendaq/event_packet_ptr.h>
#include <opendaq/signal_factory.h>

#include <opendaq/custom_log.h>
#include <opendaq/event_packet_params.h>

#include "coreobjects/unit_factory.h"
#include "opendaq/data_packet.h"
#include "opendaq/data_packet_ptr.h"
#include "opendaq/event_packet_ids.h"
#include "opendaq/packet_factory.h"
#include "opendaq/range_factory.h"
#include "opendaq/sample_type_traits.h"
#include <coreobjects/eval_value_factory.h>
#include <opendaq/reusable_data_packet_ptr.h>
#include <opendaq/component_status_container_private_ptr.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace Scaling
{

ScalingFbImpl::ScalingFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initComponentStatus();
    createInputPorts();
    createSignals();
    initProperties();
}

void ScalingFbImpl::initProperties()
{
    const auto scaleProp = FloatProperty("scale", 1.0);
    objPtr.addProperty(scaleProp);
    objPtr.getOnPropertyValueWrite("scale") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto offsetProp = FloatProperty("offset", 0.0);
    objPtr.addProperty(offsetProp);
    objPtr.getOnPropertyValueWrite("offset") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto useCustomOutputRangeProp = BoolProperty("UseCustomOutputRange", False);
    objPtr.addProperty(useCustomOutputRangeProp);
    objPtr.getOnPropertyValueWrite("UseCustomOutputRange") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto customHighValueProp = FloatProperty("OutputHighValue", 10.0, EvalValue("$UseCustomOutputRange"));
    objPtr.addProperty(customHighValueProp);
    objPtr.getOnPropertyValueWrite("OutputHighValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto customLowValueProp = FloatProperty("OutputLowValue", -10.0, EvalValue("$UseCustomOutputRange"));
    objPtr.addProperty(customLowValueProp);
    objPtr.getOnPropertyValueWrite("OutputLowValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto outputNameProp = StringProperty("OutputName", "");
    objPtr.addProperty(outputNameProp);
    objPtr.getOnPropertyValueWrite("OutputName") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto outputUnitProp = StringProperty("OutputUnit", "");
    objPtr.addProperty(outputUnitProp);
    objPtr.getOnPropertyValueWrite("OutputUnit") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    readProperties();
}

void ScalingFbImpl::propertyChanged(bool configure)
{
    readProperties();
    if (configure)
        this->configure();
}

void ScalingFbImpl::readProperties()
{
    scale = objPtr.getPropertyValue("scale");
    offset = objPtr.getPropertyValue("offset");
    useCustomOutputRange = objPtr.getPropertyValue("UseCustomOutputRange");
    outputHighValue = objPtr.getPropertyValue("OutputHighValue");
    outputLowValue = objPtr.getPropertyValue("OutputLowValue");
    outputUnit = static_cast<std::string>(objPtr.getPropertyValue("OutputUnit"));
    outputName = static_cast<std::string>(objPtr.getPropertyValue("OutputName"));
}

FunctionBlockTypePtr ScalingFbImpl::CreateType()
{
    return FunctionBlockType("RefFBModuleScaling", "Scaling", "Signal scaling");
}

void ScalingFbImpl::processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor,
                                                   const DataDescriptorPtr& inputDomainDataDescriptor)
{
    if (inputDataDescriptor.assigned())
        this->inputDataDescriptor = inputDataDescriptor;
    if (inputDomainDataDescriptor.assigned())
        this->inputDomainDataDescriptor = inputDomainDataDescriptor;

    configure();
}

void ScalingFbImpl::configure()
{
    setComponentStatus(ComponentStatus::Ok);
    try
    {
        if (inputDomainDataDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("No domain input");
        }

        if (inputDataDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("No value input");
        }

        if (inputDataDescriptor.getDimensions().getCount() > 0)
        {
            throw std::runtime_error("Arrays not supported");
        }

        inputSampleType = inputDataDescriptor.getSampleType();
        if (inputSampleType != SampleType::Float64 &&
            inputSampleType != SampleType::Float32 &&
            inputSampleType != SampleType::Int8 &&
            inputSampleType != SampleType::Int16 &&
            inputSampleType != SampleType::Int32 &&
            inputSampleType != SampleType::Int64 &&
            inputSampleType != SampleType::UInt8 &&
            inputSampleType != SampleType::UInt16 &&
            inputSampleType != SampleType::UInt32 &&
            inputSampleType != SampleType::UInt64)
        {
            throw std::runtime_error("Invalid sample type");
        }

        auto outputDataDescriptorBuilder = DataDescriptorBuilder().setSampleType(SampleType::Float64);

        RangePtr outputRange;
        if (useCustomOutputRange)
            outputRange = Range(outputLowValue, outputHighValue);
        else
        {
            auto outputHigh = scale * static_cast<Float>(inputDataDescriptor.getValueRange().getLowValue()) + offset;
            auto outputLow = scale * static_cast<Float>(inputDataDescriptor.getValueRange().getHighValue()) + offset;
            if (outputLow > outputHigh)
                std::swap(outputLow, outputHigh);
            outputRange = Range(outputLow, outputHigh);
        }

        outputDataDescriptorBuilder.setValueRange(outputRange);

        if (outputName.empty())
            outputDataDescriptorBuilder.setName(inputDataDescriptor.getName().toStdString() + "/Scaled");
        else
            outputDataDescriptorBuilder.setName(outputName);

        if (outputUnit.empty())
            outputDataDescriptorBuilder.setUnit(inputDataDescriptor.getUnit());
        else
            outputDataDescriptorBuilder.setUnit(Unit(outputUnit));
        outputDataDescriptor = outputDataDescriptorBuilder.build();
        outputSignal.setDescriptor(outputDataDescriptor);
        outputDomainSignal.setDescriptor(inputDomainDataDescriptor);
    }
    catch (const std::exception& e)
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Failed to set descriptor for output signal: {}", e.what()));
        outputSignal.setDescriptor(nullptr);
    }
}


void ScalingFbImpl::onPacketReceived(const InputPortPtr& port)
{
    auto outQueue = List<IPacket>();
    auto outDomainQueue = List<IPacket>();
    
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
                SAMPLE_TYPE_DISPATCH(inputSampleType, processDataPacket, std::move(packet), outQueue, outDomainQueue);
                break;

            default:
                break;
        }

        packet = connection.dequeue();
    };

    outputSignal.sendPackets(std::move(outQueue));
    outputDomainSignal.sendPackets(std::move(outDomainQueue));
}

void ScalingFbImpl::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        DataDescriptorPtr inputDataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr inputDomainDataDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        processSignalDescriptorChanged(inputDataDescriptor, inputDomainDataDescriptor);
    }
}

/*
template <SampleType InputSampleType>
void ScalingFbImpl::processDataPacket(const DataPacketPtr& packet)
{
    const size_t sampleCount = packet.getSampleCount();

    const auto outputPacket = DataPacketWithDomain(packet.getDomainPacket(), outputDataDescriptor, sampleCount);
    outputSignal.sendPacket(outputPacket);
    outputDomainSignal.sendPacket(packet.getDomainPacket());
}
*/

 template <SampleType InputSampleType>
void ScalingFbImpl::processDataPacket(DataPacketPtr&& packet, ListPtr<IPacket>& outQueue, ListPtr<IPacket>& outDomainQueue)
{
    using InputType = typename SampleTypeToType<InputSampleType>::Type;
    auto inputData = static_cast<InputType*>(packet.getData());
    const size_t sampleCount = packet.getSampleCount();

    DataPacketPtr outputPacket;
    DataPacketPtr outputDomainPacket = packet.getDomainPacket();

    const auto reusablePacket = packet.asPtrOrNull<IReusableDataPacket>(true);
    if (reusablePacket.assigned() && packet.getRefCount() == 1 && reusablePacket.reuse(outputDataDescriptor, std::numeric_limits<SizeT>::max(), nullptr, nullptr, false))
    {
        outputPacket = std::move(packet);
    }
    else
    {
        outputPacket = DataPacketWithDomain(outputDomainPacket, outputDataDescriptor, sampleCount);
    }

    auto outputData = static_cast<Float*>(outputPacket.getData());

    for (size_t i = 0; i < sampleCount; i++)
        *outputData++ = scale * static_cast<Float>(*inputData++) + offset;

    outQueue.pushBack(std::move(outputPacket));
    outDomainQueue.pushBack(std::move(outputDomainPacket));
}

void ScalingFbImpl::createInputPorts()
{
    inputPort = createAndAddInputPort("Input", PacketReadyNotification::SchedulerQueueWasEmpty);
}

void ScalingFbImpl::createSignals()
{
    outputSignal = createAndAddSignal("output");
    outputDomainSignal = createAndAddSignal("output_domain", nullptr, false);
    outputSignal.setDomainSignal(outputDomainSignal);
}

}

END_NAMESPACE_REF_FB_MODULE
