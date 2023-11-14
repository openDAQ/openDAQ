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

#define timeMs(x) (1000 * x)

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
    const auto epochMsProp = IntProperty("EpochMs", 100);
    objPtr.addProperty(epochMsProp);
    objPtr.getOnPropertyValueWrite("EpochMs") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto ClassCountProp = IntProperty("ClassCount", 1);
    objPtr.addProperty(ClassCountProp);
    objPtr.getOnPropertyValueWrite("ClassCount") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto customHighValueProp = FloatProperty("InputHighValue", 10.0);
    objPtr.addProperty(customHighValueProp);
    objPtr.getOnPropertyValueWrite("InputHighValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto customLowValueProp = FloatProperty("InputLowValue", -10.0);
    objPtr.addProperty(customLowValueProp);
    objPtr.getOnPropertyValueWrite("InputLowValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto outputNameProp = StringProperty("OutputName", "");
    objPtr.addProperty(outputNameProp);
    objPtr.getOnPropertyValueWrite("OutputName") +=
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
    classCount = objPtr.getPropertyValue("ClassCount");
    inputHighValue = objPtr.getPropertyValue("InputHighValue");
    inputLowValue = objPtr.getPropertyValue("InputLowValue");
    outputName = static_cast<std::string>(objPtr.getPropertyValue("OutputName"));

    assert(epochMs > 0);
    assert(classCount > 0);

}

FunctionBlockTypePtr ClassifierFbImpl::CreateType()
{
    return FunctionBlockType("ref_fb_module_classifier", "Classifier", "Signal classifing");
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

        inputLowValue = static_cast<Float>(inputDataDescriptor.getValueRange().getLowValue());
        inputHighValue = static_cast<Float>(inputDataDescriptor.getValueRange().getHighValue());

        if (inputLowValue > inputHighValue)
            std::swap(inputLowValue, inputHighValue);
        
        RangePtr outputRange;
        {
            size_t rangeSize = inputHighValue - inputLowValue;
            rangeSize = (rangeSize / classCount) + (rangeSize % classCount != 0) + 1;
            auto dimensions = List<IDimension>();
            dimensions.pushBack(Dimension(LinearDimensionRule(classCount, (Int)inputLowValue, rangeSize)));
            outputDataDescriptorBuilder.setDimensions(dimensions);

            outputRange = Range(0, 1);
        }

        outputDataDescriptorBuilder.setValueRange(outputRange);

        if (outputName.empty())
            outputDataDescriptorBuilder.setName(inputDataDescriptor.getName().toStdString() + "/Classifired");
        else
            outputDataDescriptorBuilder.setName(outputName);

        outputDataDescriptorBuilder.setUnit(Unit("%"));

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

inline bool ClassifierFbImpl::timeInInterval(UInt startTime, UInt endTime) {
    return (endTime - startTime) < timeMs(epochMs);
}

template <SampleType InputSampleType>
void ClassifierFbImpl::processDataPacket(const DataPacketPtr& packet)
{
    using InputType = typename SampleTypeToType<InputSampleType>::Type;
    using OutputType = Float;

    // if packet does not have samples - ignore
    if (packet.getSampleCount() == 0) {
        return;
    }

    packets.push_back(packet);

    size_t outputPackages = 0;
    {
        auto inputDomainData = static_cast<UInt*>(packet.getDomainPacket().getData());
        UInt lastTime = inputDomainData[packet.getSampleCount() - 1];

        // initialize members
        if (packetStarted == UInt{}) {
            packetStarted = inputDomainData[0];
            sampleStarted = 0; 
        }

        outputPackages = (lastTime - packetStarted) / timeMs(epochMs);
    }

    if (!outputPackages)
        return;

    auto outputDomainPacket = DataPacket(outputDomainDataDescriptor, 1);
    auto outputDomainData = static_cast<UInt*>(outputDomainPacket.getData());

    DataPacketPtr outputPacket = DataPacketWithDomain(outputDomainPacket, outputDataDescriptor, 1);
    OutputType* outputData = static_cast<OutputType*>(outputPacket.getData());

    auto rangeSize = outputDataDescriptor.getDimensions()[0].getSize();
    Int offset = static_cast<Int>(outputDataDescriptor.getDimensions()[0].getRule().getParameters().get("start"));

    size_t packageVals = 0;
    while (outputPackages && packets.size()) {
        auto listPacket = packets.front();

        auto sampleCnt = listPacket.getSampleCount();
        auto inputData = static_cast<InputType*>(listPacket.getData());
        auto inputDomainData = static_cast<UInt*>(listPacket.getDomainPacket().getData());
       
        // reset array for new package
        if (packageVals == 0) {
            memset(outputData, 0, rangeSize * sizeof(OutputType));
        }

        bool packetInEpoch = timeInInterval(packetStarted, inputDomainData[sampleCnt - 1]);

        size_t sampleIdx = sampleStarted;
        for (; sampleIdx < sampleCnt; sampleIdx++) {
            // if there are values from another interval - check each value
            if (!packetInEpoch) {
                if (!timeInInterval(packetStarted, inputDomainData[sampleIdx]))
                    break;
            }

            auto& rawData = inputData[sampleIdx];

            // filter values
            if (rawData < inputLowValue)
                rawData = inputLowValue;
            else if (rawData > inputHighValue)
                rawData = inputHighValue;

            Int index = (static_cast<Int>(rawData) - offset) / classCount;
            outputData[index] += 1;
            outputDomainData[0] = inputDomainData[sampleIdx];
            packageVals++;
        }

        if (packetInEpoch) {
            sampleStarted = 0;
            packets.pop_front();
        } else {
            if (packageVals) {
                for (size_t i = 0; i < rangeSize; i++) {
                    outputData[i] /= packageVals;
                }
            }

            outputSignal.sendPacket(outputPacket);
            outputDomainSignal.sendPacket(outputDomainPacket);

            sampleStarted = sampleIdx;
            packageVals = 0;
            outputPackages--;
            packetStarted += timeMs(epochMs);
        }
    }
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
