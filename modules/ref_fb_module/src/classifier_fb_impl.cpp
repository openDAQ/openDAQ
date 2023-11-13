#include <ref_fb_module/classifier_fb_impl.h>
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
#include <opendaq/dimension_factory.h>

#include <iostream>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace Classifier
{

ClassifierFbImpl::ClassifierFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    createInputPorts();
    createSignals();
    initProperties();
}

void ClassifierFbImpl::initProperties()
{
    const auto epochMsProp = IntProperty("EpochMs", 1);
    objPtr.addProperty(epochMsProp);
    objPtr.getOnPropertyValueWrite("EpochMs") +=
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

void ClassifierFbImpl::propertyChanged(bool configure)
{
    std::scoped_lock lock(sync);
    readProperties();
    if (configure)
        this->configure();
}

void ClassifierFbImpl::readProperties()
{
    epochMs = objPtr.getPropertyValue("EpochMs");
    useCustomOutputRange = objPtr.getPropertyValue("UseCustomOutputRange");
    outputHighValue = objPtr.getPropertyValue("OutputHighValue");
    outputLowValue = objPtr.getPropertyValue("OutputLowValue");
    outputUnit = static_cast<std::string>(objPtr.getPropertyValue("OutputUnit"));
    outputName = static_cast<std::string>(objPtr.getPropertyValue("OutputName"));
}

FunctionBlockTypePtr ClassifierFbImpl::CreateType()
{
    return FunctionBlockType("ref_fb_module_classifer", "Classifier", "Signal classifing");
}

void ClassifierFbImpl::processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor,
                                                   const DataDescriptorPtr& inputDomainDataDescriptor)
{
    if (inputDataDescriptor.assigned())
        this->inputDataDescriptor = inputDataDescriptor;
    if (inputDomainDataDescriptor.assigned())
        this->inputDomainDataDescriptor = inputDomainDataDescriptor;

    configure();
}

void ClassifierFbImpl::configure()
{
    if (!inputDataDescriptor.assigned() || !inputDomainDataDescriptor.assigned())
    {
        LOG_D("Incomplete signal descriptors")
        return;
    }

    try
    {
        if (inputDataDescriptor.getDimensions().getCount() > 0)
            throw std::runtime_error("Arrays not supported");

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
            throw std::runtime_error("Invalid sample type");

        auto outputDataDescriptorBuilder = DataDescriptorBuilder().setSampleType(SampleType::Float64);

        if (!useCustomOutputRange)
        {
            outputLowValue = static_cast<Float>(inputDataDescriptor.getValueRange().getLowValue());
            outputHighValue = static_cast<Float>(inputDataDescriptor.getValueRange().getHighValue());
        }

        if (outputLowValue > outputHighValue)
            std::swap(outputLowValue, outputHighValue);
        
        RangePtr outputRange;
        {
            auto rangeSize = size_t((Int)outputHighValue - (Int)outputLowValue + 1);
            auto dimensions = List<IDimension>();
            dimensions.pushBack(Dimension(LinearDimensionRule(1, (Int)outputLowValue, rangeSize)));
            outputDataDescriptorBuilder.setDimensions(dimensions);

            outputRange = Range(0, 1);
        }

        outputDataDescriptorBuilder.setValueRange(outputRange);

        if (outputName.empty())
            outputDataDescriptorBuilder.setName(inputDataDescriptor.getName().toStdString() + "/Classifired");
        else
            outputDataDescriptorBuilder.setName(outputName);

        if (outputUnit.empty())
            outputDataDescriptorBuilder.setUnit(Unit("freq/val", -1, "frequency of each value"));
        else
            outputDataDescriptorBuilder.setUnit(Unit(outputUnit));

        outputDataDescriptor = outputDataDescriptorBuilder.build();
        outputSignal.setDescriptor(outputDataDescriptor);
        
        outputDomainDataDescriptor = DataDescriptorBuilderCopy(inputDomainDataDescriptor).setRule(ExplicitDataRule()).build();
        outputDomainSignal.setDescriptor(outputDomainDataDescriptor);
    }
    catch (const std::exception& e)
    {
        LOG_W("Failed to set descriptor for power signal: {}", e.what())
        outputSignal.setDescriptor(nullptr);
    }
}


void ClassifierFbImpl::onPacketReceived(const InputPortPtr& port)
{
    std::scoped_lock lock(sync);

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

void ClassifierFbImpl::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        DataDescriptorPtr inputDataDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr inputDomainDataDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        processSignalDescriptorChanged(inputDataDescriptor, inputDomainDataDescriptor);
    }
}

template <SampleType InputSampleType>
void ClassifierFbImpl::processDataPacket(const DataPacketPtr& packet)
{
    using InputType = typename SampleTypeToType<InputSampleType>::Type;

    if (packet.getSampleCount() == 0) {
        return;
    }
    
    //checking reaching of a new epoch
    {
        auto inputDomainData = static_cast<UInt*>(packet.getDomainPacket().getData());

        UInt cur_time = inputDomainData[0];

        if (packetStarted == UInt{}) {
            packetStarted = cur_time;
                sampleStarted = 0; 
        }

        auto diff = cur_time - packetStarted;
            if (diff < 1000 * epochMs) {
            packets.push_back(packet);
            return;
        }
    }

    if (packets.empty()) {
        return;
    }

    auto rangeSize = outputDataDescriptor.getDimensions()[0].getSize();

    auto outputDomainPacket = DataPacket(outputDomainDataDescriptor, 1);
    auto outputDomainData = static_cast<UInt*>(outputDomainPacket.getData());

    auto outputPacket = DataPacketWithDomain(outputDomainPacket, outputDataDescriptor, 1);
    auto outputData = static_cast<InputType*>(outputPacket.getData());
    memset(outputData, 0, rangeSize * sizeof(InputType));

    // calculating end block for classifier
    size_t lastPacketSamples = 1;
    {   
        auto& lastPacket = packets.back();
        auto inputDomainData = static_cast<UInt*>(lastPacket.getDomainPacket().getData());
        bool wasBreaked = false;

        for(; lastPacketSamples < lastPacket.getSampleCount(); lastPacketSamples++) {
            if (inputDomainData[lastPacketSamples] - packetStarted >= 1000 * epochMs) {
                wasBreaked = true;
                break;
            }
        }
        lastPacketSamples += wasBreaked;

        *outputDomainData = inputDomainData[lastPacketSamples - 1];
        packetStarted += 1000 * epochMs;
    }

    Int offset = static_cast<Int>(outputDataDescriptor.getDimensions()[0].getRule().getParameters().get("start"));
    double valCnt = 0;

    for (const auto & list_packet: packets) {
        auto inputData = static_cast<InputType*>(list_packet.getData());
        size_t sampleIdx = list_packet == packets.front() ? sampleStarted : 0; 
        size_t packetSamples = list_packet == packets.back() ? lastPacketSamples : list_packet.getSampleCount();

        for (; sampleIdx < packetSamples; sampleIdx++) {
            auto& rawData = inputData[sampleIdx];
            
            if (rawData < outputLowValue)
                rawData = outputLowValue;
            else if (rawData > outputHighValue)
                rawData = outputHighValue;

            Int index = static_cast<Int>(rawData) - offset;
            outputData[index] += 1;
            valCnt++;
        }
    }
    
    if (valCnt == 0.0) valCnt = 1.0;
    for (size_t i = 0; i < rangeSize; i++) {
        outputData[i] /= valCnt;
    }

    outputSignal.sendPacket(outputPacket);
    outputDomainSignal.sendPacket(outputDomainPacket);

    auto lastPacket = packets.back();
    packets.clear();
    sampleStarted = 0;
    if (lastPacketSamples != lastPacket.getSampleCount()) {
        sampleStarted = lastPacketSamples;
        packets.push_back(lastPacket);
    }
    packets.push_back(packet);
}

void ClassifierFbImpl::createInputPorts()
{
    inputPort = createAndAddInputPort("input", PacketReadyNotification::Scheduler);
}

void ClassifierFbImpl::createSignals()
{
    outputSignal = createAndAddSignal(String("output"));
    outputDomainSignal = createAndAddSignal(String("output_domain"));
    outputSignal.setDomainSignal(outputDomainSignal);
}

}

END_NAMESPACE_REF_FB_MODULE
