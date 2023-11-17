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
    objPtr.addProperty(BoolProperty("UseExplicitDomain", false));
    objPtr.getOnPropertyValueWrite("UseExplicitDomain") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    auto explcitDimensionProp = ListPropertyBuilder("ExplcitDimension", List<Float>()).setVisible(EvalValue("$UseExplicitDomain")).build();
    objPtr.addProperty(explcitDimensionProp);
    objPtr.getOnPropertyValueWrite("ExplcitDimension") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    auto blockSizeMsProp = IntPropertyBuilder("BlockSizeMs", 10).setVisible(EvalValue("!$UseExplicitDomain")).build();
    objPtr.addProperty(blockSizeMsProp);
    objPtr.getOnPropertyValueWrite("BlockSizeMs") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    auto classCountProp = IntPropertyBuilder("ClassCount", 1).setVisible(EvalValue("!$UseExplicitDomain")).build();
    objPtr.addProperty(classCountProp);
    objPtr.getOnPropertyValueWrite("ClassCount") +=
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
    useExplicitDomain = objPtr.getPropertyValue("UseExplicitDomain");
    explicitDimension =  objPtr.getPropertyValue("ExplcitDimension");
    blockSizeMs = objPtr.getPropertyValue("BlockSizeMs");
    classCount = objPtr.getPropertyValue("ClassCount");
    outputName = static_cast<std::string>(objPtr.getPropertyValue("OutputName"));

    assert(blockSizeMs > 0);
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

        const auto domainRule = inputDomainDataDescriptor.getRule();
        if (domainRule.getType() != DataRuleType::Linear)
        {
            throw std::runtime_error("Domain rule type is not Linear");
        }

        const auto domainRuleParams = domainRule.getParameters();

        inputDeltaTicks = domainRuleParams.get("delta");

        auto outputDataDescriptorBuilder = DataDescriptorBuilder().setSampleType(SampleType::Float64);
        
        auto dimensions = List<IDimension>();
        if (useExplicitDomain) 
            dimensions.pushBack(Dimension(ListDimensionRule(explicitDimension)));
        else 
        {
            auto inputLowValue = static_cast<Float>(inputDataDescriptor.getValueRange().getLowValue());
            auto inputHighValue = static_cast<Float>(inputDataDescriptor.getValueRange().getHighValue());

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
    return time * inputDeltaTicks;
}

inline bool ClassifierFbImpl::timeInInterval(UInt startTime, UInt endTime) 
{
    return (endTime - startTime) < timeMs(blockSizeMs);
}

template <SampleType InputSampleType>
void ClassifierFbImpl::processDataPacket(const DataPacketPtr& packet)
{
    using InputType = typename SampleTypeToType<InputSampleType>::Type;
    using OutputType = Float;

    // if packet does not have samples - ignore
    if (packet.getSampleCount() == 0)
        return;

    packets.push_back(packet);

    size_t outputPackages = 0;
    {
        auto inputDomainData = static_cast<UInt*>(packet.getDomainPacket().getData());
        UInt lastTime = inputDomainData[packet.getSampleCount() - 1];

        // initialize members
        if (packetStarted == UInt{})
        {
            packetStarted = inputDomainData[0];
            sampleStarted = 0; 
        }

        outputPackages = (lastTime - packetStarted) / timeMs(blockSizeMs);
    }

    if (!outputPackages)
        return;

    auto outputDomainPacket = DataPacket(outputDomainDataDescriptor, 1);
    auto outputDomainData = static_cast<UInt*>(outputDomainPacket.getData());

    DataPacketPtr outputPacket {};
    OutputType* outputData {};

    auto rangeSize = outputDataDescriptor.getDimensions()[0].getSize();
    auto labels = outputDataDescriptor.getDimensions()[0].getLabels();
    if (labels.getCount() == 0) 
        return;

    size_t packageVals = 0;
    while (outputPackages && packets.size()) 
    {
        auto listPacket = packets.front();

        auto sampleCnt = listPacket.getSampleCount();
        auto inputData = static_cast<InputType*>(listPacket.getData());
        auto inputDomainData = static_cast<UInt*>(listPacket.getDomainPacket().getData());
       
        // reset array for new package
        if (packageVals == 0)
        {
            outputPacket = DataPacketWithDomain(outputDomainPacket, outputDataDescriptor, 1);
            outputData = static_cast<OutputType*>(outputPacket.getData());
            memset(outputData, 0, rangeSize * sizeof(OutputType));
        }

        bool packetInEpoch = timeInInterval(packetStarted, inputDomainData[sampleCnt - 1]);

        size_t sampleIdx = sampleStarted;
        for (; sampleIdx < sampleCnt; sampleIdx++) 
        {
            // if there are values from another interval - check each value
            if (!packetInEpoch && !timeInInterval(packetStarted, inputDomainData[sampleIdx]))
                    break;

            outputDomainData[0] = inputDomainData[sampleIdx];
            packageVals++;

            auto& rawData = inputData[sampleIdx];

            // filter values
            if (static_cast<Float>(rawData) < static_cast<Float>(labels[0]))
                continue;
            else if (static_cast<Float>(rawData) > static_cast<Float>(labels[rangeSize - 1]))
                continue;

            if (labels.getCount() == 1) 
                outputData[0] += 1;
            else 
            {
                // binary search 
                size_t low = 0;
                size_t high = labels.getCount() - 1;

                while (low <= high) 
                {
                    size_t mid = low + (high - low) / 2;

                    if (rawData >= static_cast<Float>(labels[mid]) && rawData < static_cast<Float>(labels[mid + 1])) 
                    {
                        outputData[mid] += 1;
                        break;
                    } 
                    else if (rawData < static_cast<Float>(labels[mid])) 
                        high = mid - 1;
                    else 
                        low = mid + 1;
                }
            }
        }

        if (packetInEpoch) 
        {
            sampleStarted = 0;
            packets.pop_front();
        } 
        else 
        {
            packetStarted += timeMs(blockSizeMs);
            
            if (packageVals) 
            {
                for (size_t i = 0; i < rangeSize; i++) 
                    outputData[i] /= packageVals;
                 outputDomainData[0] = packetStarted;
            }
            outputSignal.sendPacket(outputPacket);
            outputDomainSignal.sendPacket(outputDomainPacket);

            sampleStarted = sampleIdx;
            packageVals = 0;
            outputPackages--;
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
