#include <ref_fb_module/classifier_fb_impl.h>
#include <ref_fb_module/dispatch.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/signal_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/event_packet_params.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/data_packet.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/sample_type_traits.h>
#include <coreobjects/eval_value_factory.h>
#include <opendaq/dimension_factory.h>

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
    objPtr.addProperty(BoolPropertyBuilder("UseExplicitDimension", false)
        .setDescription("Choose classification based on input signal min max with step of ClassCount or custom marks").build());
    objPtr.getOnPropertyValueWrite("UseExplicitDimension") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    auto explcitDimensionProp = ListPropertyBuilder("ExplicitDimension", List<Float>()).setVisible(EvalValue("$UseExplicitDimension"))
        .setDescription("Set custom list for classification rule").build();
    objPtr.addProperty(explcitDimensionProp);
    objPtr.getOnPropertyValueWrite("ExplicitDimension") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    auto blockSizeProp = IntPropertyBuilder("BlockSize", 1).setVisible(EvalValue("!$UseExplicitDimension")).setUnit(Unit("ms")).build();
    objPtr.addProperty(blockSizeProp);
    objPtr.getOnPropertyValueWrite("BlockSize") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    auto classCountProp = IntPropertyBuilder("ClassCount", 1).setVisible(EvalValue("!$UseExplicitDimension")).build();
    objPtr.addProperty(classCountProp);
    objPtr.getOnPropertyValueWrite("ClassCount") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto useCustomInputRangeProp = BoolProperty("UseCustomInputRange", False);
    objPtr.addProperty(useCustomInputRangeProp);
    objPtr.getOnPropertyValueWrite("UseCustomInputRange") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto customHighValueProp = FloatProperty("InputHighValue", 10.0, EvalValue("$UseCustomInputRange"));
    objPtr.addProperty(customHighValueProp);
    objPtr.getOnPropertyValueWrite("InputHighValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto customLowValueProp = FloatProperty("inputLowValue", -10.0, EvalValue("$UseCustomInputRange"));
    objPtr.addProperty(customLowValueProp);
    objPtr.getOnPropertyValueWrite("inputLowValue") +=
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
    useExplicitDomain = objPtr.getPropertyValue("UseExplicitDimension");
    explicitDimension = objPtr.getPropertyValue("ExplicitDimension");
    blockSize = objPtr.getPropertyValue("BlockSize");
    classCount = objPtr.getPropertyValue("ClassCount");
    useCustomInputRange = objPtr.getPropertyValue("UseCustomInputRange");
    inputHighValue = objPtr.getPropertyValue("InputHighValue");
    inputLowValue = objPtr.getPropertyValue("inputLowValue");
    outputName = static_cast<std::string>(objPtr.getPropertyValue("OutputName"));

    assert(blockSize > 0);
    if (!useExplicitDomain)
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
    if (!inputDataDescriptor.assigned())
    {
        LOG_D("ClassifierFb: Incomplete input data signal descriptor")
        return;
    }

    if (!inputDomainDataDescriptor.assigned())
    {
        LOG_D("ClassifierFb: Incomplete input domain signal descriptor")
        return;
    }

    try
    {
        if (inputDataDescriptor.isStructDescriptor() || inputDataDescriptor.getDimensions().getCount() > 0)
            throw std::runtime_error("Incompatible input value data descriptor");

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

        if (inputDomainDataDescriptor.getSampleType() != SampleType::Int64 && inputDomainDataDescriptor.getSampleType() != SampleType::UInt64)
        {
            throw std::runtime_error("Incompatible domain data sample type");
        }

        auto domainUnit = inputDomainDataDescriptor.getUnit();
        if (domainUnit.getSymbol() != "s" && domainUnit.getSymbol() != "seconds")
        {
            throw std::runtime_error("Domain unit expected in seconds");
        }

        const auto domainRule = inputDomainDataDescriptor.getRule();
        domainLinear = domainRule.getType() == DataRuleType::Linear;
        
        if (domainLinear)
        {
            const auto domainRuleParams = domainRule.getParameters();

            inputDeltaTicks = domainRuleParams.get("delta");
            auto resolution = inputDomainDataDescriptor.getTickResolution();
            // packets per second
            linearBlockCount = resolution.getDenominator() / resolution.getNumerator() / inputDeltaTicks;
            // packets per BlockSize
            linearBlockCount = blockSize * linearBlockCount / 1000;
        }

        auto outputDataDescriptorBuilder = DataDescriptorBuilder().setSampleType(SampleType::Float64);
        
        auto dimensions = List<IDimension>();
        if (useExplicitDomain) 
            dimensions.pushBack(Dimension(ListDimensionRule(explicitDimension)));
        else 
        {
            if (!useCustomInputRange)
            {
                inputLowValue = static_cast<Float>(inputDataDescriptor.getValueRange().getLowValue());
                inputHighValue = static_cast<Float>(inputDataDescriptor.getValueRange().getHighValue());
                if (inputLowValue > inputHighValue)
                    std::swap(inputLowValue, inputHighValue);
            }

            size_t rangeSize = inputHighValue - inputLowValue;
            rangeSize = (rangeSize / classCount) + (rangeSize % classCount != 0) + 1;
            dimensions.pushBack(Dimension(LinearDimensionRule(classCount, (Int)inputLowValue, rangeSize)));
        }
        outputDataDescriptorBuilder.setDimensions(dimensions);

        outputDataDescriptorBuilder.setValueRange(Range(0, 1));

        if (outputName.empty())
            outputDataDescriptorBuilder.setName(inputDataDescriptor.getName().toStdString() + "/Classified");
        else
            outputDataDescriptorBuilder.setName(outputName);

        outputDataDescriptorBuilder.setUnit(Unit("%"));

        outputDataDescriptor = outputDataDescriptorBuilder.build();
        outputSignal.setDescriptor(outputDataDescriptor);
        
        if (domainLinear)
        {
            const auto domainRuleParams = domainRule.getParameters();
            outputDomainDataDescriptor = DataDescriptorBuilderCopy(inputDomainDataDescriptor).setRule(LinearDataRule(static_cast<UInt>(linearBlockCount * inputDeltaTicks), domainRuleParams.get("start"))).build();
        }
        else
            outputDomainDataDescriptor = DataDescriptorBuilderCopy(inputDomainDataDescriptor).setRule(ExplicitDataRule()).build();
        
        outputDomainSignal.setDescriptor(outputDomainDataDescriptor);
    }
    catch (const std::exception& e)
    {
        LOG_W("ClassifierFb: Failed to set descriptor for classification signal: {}", e.what())
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

inline UInt ClassifierFbImpl::timeMs(UInt time) 
{
    return time * 1000;
}

inline bool ClassifierFbImpl::timeInInterval(UInt startTime, UInt endTime) 
{
    return (endTime - startTime) < timeMs(blockSize);
}

Int ClassifierFbImpl::binarySearch(float value, const ListPtr<IBaseObject>& labels) 
{
    // filter values
    if (value < static_cast<Float>(labels[0]))
        return -1;
    else if (value > static_cast<Float>(labels[labels.getCount() - 1]))
        return -1;

    if (labels.getCount() == 1) 
        return 0;

    // binary search 
    Int low = 0;
    Int high = labels.getCount() - 1;

    while (low <= high) 
    {
        Int mid = low + (high - low) / 2;

        if (value >= static_cast<Float>(labels.getItemAt(mid)) && value < static_cast<Float>(labels.getItemAt(mid+1))) 
            return mid;
        else if (value < static_cast<Float>(labels.getItemAt(mid))) 
            high = mid - 1;
        else 
            low = mid + 1;
    }
    return -1;
}

template <SampleType InputSampleType>
void ClassifierFbImpl::processLinearDataPacket(const DataPacketPtr& packet)
{
    using InputType = typename SampleTypeToType<InputSampleType>::Type;
    using OutputType = Float;
    
    if (linearBlockCount == 0)
    {
        LOG_D("blockSize is too small");
        return;
    }

    // if packet does not have samples - ignore
    if (packet.getSampleCount() == 0)
        return;

    packets.push_back(packet);
    samplesInPacketList += packet.getSampleCount();

    // initialize members
    if (packetStarted == UInt{})
    {
        auto inputDomainData = static_cast<UInt*>(packet.getDomainPacket().getData());
        packetStarted = inputDomainData[0];
        lastReadSampleInBlock = 0;
    }

    size_t outputPackets = samplesInPacketList / linearBlockCount;
    if (!outputPackets)
        return;

    auto labels = outputDataDescriptor.getDimensions()[0].getLabels();
    if (labels.getCount() == 0) 
        return;

    DataPacketPtr outputDomainPacket {};
    UInt* outputDomainData {};

    DataPacketPtr outputPacket {};
    OutputType* outputData {};

    size_t packetValueCount = 0;

    while (samplesInPacketList >= linearBlockCount) 
    {
        auto listPacket = packets.front();

        auto sampleCnt = listPacket.getSampleCount();
        auto inputData = static_cast<InputType*>(listPacket.getData());
       
        // reset array for new package
        if (packetValueCount == 0)
        {
            outputDomainPacket = DataPacket(outputDomainDataDescriptor, 1, packetStarted);
            outputDomainData = static_cast<UInt*>(outputDomainPacket.getData());

            outputPacket = DataPacketWithDomain(outputDomainPacket, outputDataDescriptor, 1);
            outputData = static_cast<OutputType*>(outputPacket.getData());
            memset(outputData, 0, labels.getCount() * sizeof(OutputType));
        }

        size_t sampleIdx = lastReadSampleInBlock;
        for (; sampleIdx < sampleCnt; sampleIdx++) 
        {
            if (packetValueCount == linearBlockCount)
                break;
    
            packetValueCount++;

            auto& rawData = inputData[sampleIdx];

            auto idx = binarySearch(static_cast<Float>(rawData), labels);
            if (idx != -1)
                outputData[idx] += 1;
        }

        if (packetValueCount != linearBlockCount) 
        {
            lastReadSampleInBlock = 0;
            packets.pop_front();
        } 
        else 
        {
            for (size_t i = 0; i < labels.getCount(); i++) 
                outputData[i] /= packetValueCount;

            outputSignal.sendPacket(outputPacket);
            outputDomainSignal.sendPacket(outputDomainPacket);

            lastReadSampleInBlock = sampleIdx;
            packetValueCount = 0;
            samplesInPacketList -= linearBlockCount;
        }
    }
}

template <SampleType InputSampleType>
void ClassifierFbImpl::processDataPacket(const DataPacketPtr& packet)
{
    if (domainLinear)
    {
        processLinearDataPacket<InputSampleType>(packet);
        return;
    }

    using InputType = typename SampleTypeToType<InputSampleType>::Type;
    using OutputType = Float;

    // if packet does not have samples - ignore
    if (packet.getSampleCount() == 0)
        return;

    packets.push_back(packet);

    UInt outputPackets = 0;
    {
        auto inputDomainData = static_cast<UInt*>(packet.getDomainPacket().getData());
        UInt lastTime = inputDomainData[packet.getSampleCount() - 1];

        // initialize members
        if (packetStarted == UInt{})
        {
            packetStarted = inputDomainData[0];
            lastReadSampleInBlock = 0; 
        }

        outputPackets = (lastTime - packetStarted) / timeMs(blockSize);
    }

    if (!outputPackets)
        return;

    auto labels = outputDataDescriptor.getDimensions()[0].getLabels();
    if (labels.getCount() == 0) 
        return;

    DataPacketPtr outputDomainPacket {};
    UInt* outputDomainData {};

    DataPacketPtr outputPacket {};
    OutputType* outputData {};

    size_t packetValueCount = 0;
    while (outputPackets && packets.size()) 
    {
        auto listPacket = packets.front();

        auto sampleCnt = listPacket.getSampleCount();
        auto inputData = static_cast<InputType*>(listPacket.getData());
        auto inputDomainData = static_cast<UInt*>(listPacket.getDomainPacket().getData());
       
        // reset array for new package
        if (packetValueCount == 0)
        {
            outputDomainPacket = DataPacket(outputDomainDataDescriptor, 1);
            outputDomainData = static_cast<UInt*>(outputDomainPacket.getData());

            outputPacket = DataPacketWithDomain(outputDomainPacket, outputDataDescriptor, 1);
            outputData = static_cast<OutputType*>(outputPacket.getData());
            memset(outputData, 0, labels.getCount() * sizeof(OutputType));
        }

        bool packetInEpoch = timeInInterval(packetStarted, inputDomainData[sampleCnt - 1]);

        size_t sampleIdx = lastReadSampleInBlock;
        for (; sampleIdx < sampleCnt; sampleIdx++) 
        {
            // if there are values from another interval - check each value
            if (!packetInEpoch && !timeInInterval(packetStarted, inputDomainData[sampleIdx]))
                break;

            outputDomainData[0] = inputDomainData[sampleIdx];
            packetValueCount++;

            auto& rawData = inputData[sampleIdx];

            auto idx = binarySearch(static_cast<Float>(rawData), labels);
            if (idx != -1)
                outputData[idx] += 1;
        }

        if (packetInEpoch) 
        {
            lastReadSampleInBlock = 0;
            packets.pop_front();
        } 
        else 
        {
            packetStarted += timeMs(blockSize);

            if (!packetValueCount) 
            {
                if (outputDomainData)
                    outputDomainData[0] = packetStarted;
                packetValueCount = 1;   
            }

            for (size_t i = 0; i < labels.getCount(); i++) 
                outputData[i] /= packetValueCount;

            outputSignal.sendPacket(outputPacket);
            outputDomainSignal.sendPacket(outputDomainPacket);

            lastReadSampleInBlock = sampleIdx;
            packetValueCount = 0;
            outputPackets--;
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
