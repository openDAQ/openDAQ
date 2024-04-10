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
#include <opendaq/reader_factory.h>

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
    objPtr.addProperty(BoolPropertyBuilder("UseCustomClasses", false)
        .setDescription("Choose classification based on input signal min max with step of ClassCount or custom marks").build());
    objPtr.getOnPropertyValueWrite("UseCustomClasses") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    auto explcitDimensionProp = ListPropertyBuilder("CustomClassList", List<Float>()).setVisible(EvalValue("$UseCustomClasses"))
        .setDescription("Set custom list for classification rule").build();
    objPtr.addProperty(explcitDimensionProp);
    objPtr.getOnPropertyValueWrite("CustomClassList") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    auto blockSizeProp = IntPropertyBuilder("BlockSize", 1).setVisible(EvalValue("!$UseCustomClasses")).setUnit(Unit("ms")).build();
    objPtr.addProperty(blockSizeProp);
    objPtr.getOnPropertyValueWrite("BlockSize") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    auto classCountProp = IntPropertyBuilder("ClassCount", 1).setVisible(EvalValue("!$UseCustomClasses")).build();
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
    useCustomClasses = objPtr.getPropertyValue("UseCustomClasses");
    customClassList = objPtr.getPropertyValue("CustomClassList");
    blockSize = objPtr.getPropertyValue("BlockSize");
    classCount = objPtr.getPropertyValue("ClassCount");
    useCustomInputRange = objPtr.getPropertyValue("UseCustomInputRange");
    inputHighValue = objPtr.getPropertyValue("InputHighValue");
    inputLowValue = objPtr.getPropertyValue("inputLowValue");
    outputName = static_cast<std::string>(objPtr.getPropertyValue("OutputName"));

    if (blockSize == 0)
        throw InvalidParameterException("Classifier property BlockSize must be greater than 0");

    if (!useCustomClasses)
    {
        assert(classCount > 0);
    }
    else if (customClassList.empty())
    {
        LOG_W("Classifier property CustomClassList is empty");
    }
    else
    {
        Float lastValue = customClassList[0];
        for (const auto& el : customClassList)
        {
            if (static_cast<Float>(el) < lastValue)
            {
                LOG_W("Classifier property CustomClassList is not incremental");
                break;
            }
            lastValue = el;
        }
    }
}

FunctionBlockTypePtr ClassifierFbImpl::CreateType()
{
    return FunctionBlockType("ref_fb_module_classifier", "Classifier", "Signal classifing");
}

bool ClassifierFbImpl::processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor, const DataDescriptorPtr& inputDomainDataDescriptor)
{
    if (inputDataDescriptor.assigned())
        this->inputDataDescriptor = inputDataDescriptor;
    if (inputDomainDataDescriptor.assigned())
        this->inputDomainDataDescriptor = inputDomainDataDescriptor;

    if (!inputDataDescriptor.assigned() && !inputDomainDataDescriptor.assigned())
        return false;

    configure();
    return true;
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
        if (inputDataDescriptor.getSampleType() == SampleType::Struct || inputDataDescriptor.getDimensions().getCount() > 0)
            throw std::runtime_error("Incompatible input value data descriptor");

        auto inputSampleType = inputDataDescriptor.getSampleType();
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

        {
            auto resolution = inputDomainDataDescriptor.getTickResolution();
            inputResolution = resolution.getDenominator() / resolution.getNumerator();
        }
        
        if (domainLinear)
        {
            const auto domainRuleParams = domainRule.getParameters();

            inputDeltaTicks = domainRuleParams.get("delta");
            // packets per second
            linearBlockCount = inputResolution / inputDeltaTicks;
            // packets per BlockSize
            linearBlockCount = blockSize * linearBlockCount / 1000;

            linearReader = BlockReaderFromExisting(linearReader, linearBlockCount, SampleType::Float64, SampleType::UInt64);
        }
        
        auto dimensions = List<IDimension>();
        if (useCustomClasses) 
        {
            dimensions.pushBack(Dimension(ListDimensionRule(customClassList)));
        }
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
            rangeSize = (rangeSize + classCount - 1) / classCount + 1;
            dimensions.pushBack(Dimension(LinearDimensionRule(classCount, (Int)inputLowValue, rangeSize)));
        }

        auto outputDataDescriptorBuilder = DataDescriptorBuilder().setSampleType(SampleType::Float64);
        outputDataDescriptorBuilder.setDimensions(dimensions);
        outputDataDescriptorBuilder.setValueRange(Range(0, 1));
        outputDataDescriptorBuilder.setUnit(Unit("%"));
        if (outputName.empty())
            outputDataDescriptorBuilder.setName(inputDataDescriptor.getName().toStdString() + "/Classified");
        else
            outputDataDescriptorBuilder.setName(outputName);

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

void ClassifierFbImpl::processData()
{
    while (linearReader.getAvailableCount())
    {
        if (linearBlockCount == 0)
        {
            LOG_D("blockSize have to more than zero");
            return;
        }
        std::vector<Float> inputData(linearBlockCount);
        std::vector<UInt> inputDomainData(linearBlockCount);

        size_t blocksToRead = 1;
        auto status = linearReader.readWithDomain(inputData.data(), inputDomainData.data(), &blocksToRead);

        if (blocksToRead == 1)
        {
            if(domainLinear)
                processLinearData(inputData, inputDomainData);
            else
                processExplicitData(inputData[0], inputDomainData[0]);
        }

        if (auto eventPacket = status.getEventPacket(); eventPacket.assigned())
        {
            processEventPacket(eventPacket);
        }
    }
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
        if (low == high)
            return low;

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

void ClassifierFbImpl::processLinearData(const std::vector<Float>& inputData, const std::vector<UInt>& inputDomainData)
{
    auto labels = outputDataDescriptor.getDimensions()[0].getLabels();
    if (labels.getCount() == 0) 
    {
        LOG_E("Classifier labels are not set correctly");
        return;
    }

    auto outputDomainPacket = DataPacket(outputDomainDataDescriptor, 1);
    UInt* outputDomainData = static_cast<UInt*>(outputDomainPacket.getData());
    auto outputPacket = DataPacketWithDomain(outputDomainPacket, outputDataDescriptor, 1);
    auto outputData = static_cast<Float*>(outputPacket.getData());
    memset(outputData, 0, outputPacket.getRawDataSize());

    for (const auto & value : inputData)
    {
        auto idx = binarySearch(value, labels);
        if (idx != -1)
            outputData[idx] += 1;
    }

    for (size_t i = 0; i < labels.getCount(); i++) 
        outputData[i] /= inputData.size();

    outputDomainData[0] = inputDomainData.back();

    outputSignal.sendPacket(outputPacket);
    outputDomainSignal.sendPacket(outputDomainPacket);
}

inline UInt ClassifierFbImpl::blockSizeToTimeDuration() 
{
    return blockSize * inputResolution / 1000;
}

void ClassifierFbImpl::processExplicitData(Float inputData, UInt inputDomainData)
{
    // set packetStarted with first domain data
    if (firstPacket)
    {
        packetStarted = inputDomainData;
        firstPacket = false;
    }

    if (inputDomainData < packetStarted + blockSizeToTimeDuration())
    {
        cachedSamples.push_back(inputData);
        return;
    }

    auto labels = outputDataDescriptor.getDimensions()[0].getLabels();
    if (labels.getCount() == 0) 
    {
        LOG_E("Classifier labels are not set correctly");
        cachedSamples.push_back(inputData);
        return;
    }

    DataPacketPtr outputDomainPacket = DataPacket(outputDomainDataDescriptor, 1);
    UInt* outputDomainData = static_cast<UInt*>(outputDomainPacket.getData());

    DataPacketPtr outputPacket = DataPacketWithDomain(outputDomainPacket, outputDataDescriptor, 1);
    Float* outputData = static_cast<Float*>(outputPacket.getData());
    memset(outputData, 0, labels.getCount() * sizeof(Float));

    for (const auto & value : cachedSamples)
    {
        auto idx = binarySearch(static_cast<Float>(value), labels);
        if (idx != -1)
            outputData[idx] += 1;
    }
    
    packetStarted += blockSizeToTimeDuration();
    outputDomainData[0] = packetStarted;

    if (cachedSamples.size())
    {
        for (size_t i = 0; i < labels.getCount(); i++) 
            outputData[i] /= cachedSamples.size();
    }

    outputSignal.sendPacket(outputPacket);
    outputDomainSignal.sendPacket(outputDomainPacket);

    cachedSamples.clear();
    cachedSamples.push_back(inputData);
}

void ClassifierFbImpl::createInputPorts()
{
    inputPort = createAndAddInputPort("input", PacketReadyNotification::Scheduler);
    
    linearReader = BlockReaderFromPort(inputPort, linearBlockCount, SampleType::Float64, SampleType::UInt64);
    linearReader.setOnDataAvailable([this] { processData(); });
}

void ClassifierFbImpl::createSignals()
{
    outputSignal = createAndAddSignal("output");
    outputDomainSignal = createAndAddSignal("output_domain", nullptr, false);
    outputSignal.setDomainSignal(outputDomainSignal);
}

}

END_NAMESPACE_REF_FB_MODULE
