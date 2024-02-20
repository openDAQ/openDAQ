#include <opendaq/custom_log.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>
#include <ref_fb_module/statistics_fb_impl.h>

// TODO remove unnecassary
#include <opendaq/opendaq.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace Statistics
{

StatisticsFbImpl::StatisticsFbImpl(const ContextPtr& ctx,
                                   const ComponentPtr& parent,
                                   const StringPtr& localId,
                                   const PropertyObjectPtr& config)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initProperties();

    avgSignal = createAndAddSignal("avg");
    rmsSignal = createAndAddSignal("rms");
    domainSignal = createAndAddSignal("domain", nullptr, false);
    avgSignal.setDomainSignal(domainSignal);
    rmsSignal.setDomainSignal(domainSignal);

    if (config.assigned() && config.hasProperty("UseMultiThreadedScheduler") && !config.getPropertyValue("UseMultiThreadedScheduler"))
        packetReadyNotification = PacketReadyNotification::SameThread;
    else
        packetReadyNotification = PacketReadyNotification::Scheduler;

    createAndAddInputPort("input", packetReadyNotification);
    triggerOutput = createAndAddInputPort("trigger", packetReadyNotification);
}

FunctionBlockTypePtr StatisticsFbImpl::CreateType()
{
    return FunctionBlockType("ref_fb_module_statistics",
                             "Statistics",
                             "Calculates statistics",
                             []()
                             {
                                 const auto obj = PropertyObject();
                                 obj.addProperty(BoolProperty("UseMultiThreadedScheduler", true));
                                 return obj;
                             });
}

void StatisticsFbImpl::initProperties()
{
    objPtr.addProperty(IntProperty("BlockSize", 10));
    objPtr.getOnPropertyValueWrite("BlockSize") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    objPtr.addProperty(SelectionProperty("DomainSignalType", List<IString>("Implicit", "Explicit", "ExplicitRange"), 0));
    objPtr.getOnPropertyValueWrite("DomainSignalType") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    objPtr.addProperty(BoolProperty("TriggerMode", false));
    objPtr.getOnPropertyValueWrite("TriggerMode") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
    {
        propertyChanged();
        triggerModeChanged();
    };

    readProperties();
}

void StatisticsFbImpl::propertyChanged()
{
    std::scoped_lock lock(sync);
    readProperties();
    configure();
}

void StatisticsFbImpl::triggerModeChanged()
{
    std::scoped_lock lock(sync);
    if (triggerMode)
    {
        // Configure Trigger UseMultiThreadedScheduler according to Statistics UseMultiThreadedScheduler
        ModuleManagerPtr moduleManager = context.getModuleManager();
        auto triggerConfig =
            moduleManager.getModules()[0].getAvailableFunctionBlockTypes().get("ref_fb_module_trigger").createDefaultConfig();  // TODO! FIX

        if (packetReadyNotification == PacketReadyNotification::SameThread)
            triggerConfig.setPropertyValue("UseMultiThreadedScheduler", false);

        // Use trigger, output signals depending on trigger
        nestedTriggerFunctionBlock = createAndAddNestedFunctionBlock("ref_fb_module_trigger", "nfbt", triggerConfig);

        // Connect trigger output
        triggerOutput.connect(nestedTriggerFunctionBlock.getSignals()[0]);
    }
    else
    {
        // Don't use trigger, output signals
        triggerOutput.disconnect();
        removeNestedFunctionBlock(nestedTriggerFunctionBlock);
    }
}

void StatisticsFbImpl::readProperties()
{
    blockSize = objPtr.getPropertyValue("BlockSize");
    domainSignalType = static_cast<DomainSignalType>(static_cast<Int>(objPtr.getPropertyValue("DomainSignalType")));
    triggerMode = objPtr.getPropertyValue("TriggerMode");
    LOG_D("Properties: BlockSize {}, DomainSignalType {}, TriggerMode {}",
          blockSize,
          objPtr.getPropertySelectionValue("DomainSignalType").toString(),
          triggerMode);
}

void StatisticsFbImpl::configure()
{
    valid = false;
    if (!inputValueDataDescriptor.assigned() || !inputDomainDataDescriptor.assigned())
    {
        LOG_W("Incomplete input signal descriptors");
        return;
    }

    if (inputDomainDataDescriptor.getSampleType() != SampleType::Int64 && inputDomainDataDescriptor.getSampleType() != SampleType::UInt64)
    {
        LOG_W("Incompatible domain data sample type {}", convertSampleTypeToString(inputDomainDataDescriptor.getSampleType()));
        return;
    }

    const auto domainRule = inputDomainDataDescriptor.getRule();
    if (domainRule.getType() != DataRuleType::Linear)
    {
        LOG_W("Domain rule type is not Linear");
        return;
    }
    const auto domainRuleParams = domainRule.getParameters();

    start = domainRuleParams.get("start");
    inputDeltaTicks = domainRuleParams.get("delta");
    outputDeltaTicks = inputDeltaTicks * static_cast<Int>(blockSize);

    const auto outputDomainDataDescriptor = DataDescriptorBuilderCopy(inputDomainDataDescriptor).setName("StatisticsDomain");

    switch (domainSignalType)
    {
        case DomainSignalType::implicit:
            outputDomainDataDescriptor.setRule(LinearDataRule(outputDeltaTicks, start));  // modify sample rate
            break;
        case DomainSignalType::explicit_:
            outputDomainDataDescriptor.setRule(ExplicitDataRule());  // set async
            break;
        case DomainSignalType::explicitRange:
            outputDomainDataDescriptor.setRule(ExplicitDataRule()).setSampleType(SampleType::RangeInt64);  // set async
            break;
    }
    this->outputDomainDataDescriptor = outputDomainDataDescriptor.build();
    domainSampleSize = domainSignalType == DomainSignalType::implicit ? 0 : getSampleSize(this->outputDomainDataDescriptor.getSampleType());

    domainSignal.setDescriptor(this->outputDomainDataDescriptor);

    if (inputValueDataDescriptor.getSampleType() == SampleType::Struct ||
        inputValueDataDescriptor.getDimensions().getCount() > 0)  // arrays not supported on the input
    {
        LOG_W("Incompatible input value data descriptor");
        return;
    }

    sampleType = inputValueDataDescriptor.getSampleType();
    if (!acceptSampleType(sampleType))
    {
        LOG_W("Incompatible input data sample type {}", convertSampleTypeToString(sampleType));
        return;
    }
    sampleSize = getSampleSize(sampleType);

    const auto outputAverageDataDescriptor = DataDescriptorBuilderCopy(inputValueDataDescriptor)
                                                 .setName(static_cast<std::string>(inputValueDataDescriptor.getName() + "/Avg"))
                                                 .setPostScaling(nullptr);
    this->outputAverageDataDescriptor = outputAverageDataDescriptor.build();

    avgSignal.setDescriptor(this->outputAverageDataDescriptor);

    const auto outputRmsDataDescriptor = DataDescriptorBuilderCopy(inputValueDataDescriptor)
                                             .setName(static_cast<std::string>(inputValueDataDescriptor.getName() + "/Rms"))
                                             .setPostScaling(nullptr)
                                             .setValueRange(Range(0, inputValueDataDescriptor.getValueRange().getHighValue()));
    this->outputRmsDataDescriptor = outputRmsDataDescriptor.build();

    rmsSignal.setDescriptor(this->outputRmsDataDescriptor);

    resetCalcBuf();
    nextExpectedDomainValue = std::numeric_limits<Int>::max();
    valid = true;

    LOG_T("Configured: Input data sample type {}", convertSampleTypeToString(sampleType))
}

bool StatisticsFbImpl::acceptSampleType(SampleType sampleType)
{
    switch (sampleType)  // NOLINT(clang-diagnostic-switch-enum)
    {
        case SampleType::Float32:
        case SampleType::Float64:
        case SampleType::UInt8:
        case SampleType::Int8:
        case SampleType::UInt16:
        case SampleType::Int16:
        case SampleType::UInt32:
        case SampleType::Int32:
        case SampleType::UInt64:
        case SampleType::Int64:
            return true;
        default:
            return false;
    }
}

void StatisticsFbImpl::checkCalcBuf(size_t newSamples)
{
    if (calcBufAllocatedSize < calcBufSize + newSamples)
    {
        calcBufAllocatedSize = 2 * (calcBufSize + newSamples);
        const auto newBuf = static_cast<uint8_t*>(std::realloc(calcBuf.release(), calcBufAllocatedSize * sampleSize));
        calcBuf.reset(newBuf);
    }
}

void StatisticsFbImpl::copyToCalcBuf(uint8_t* buf, size_t sampleCount)
{
    std::memcpy(calcBuf.get() + calcBufSize * sampleSize, buf, sampleCount * sampleSize);
    calcBufSize += sampleCount;
}

void StatisticsFbImpl::copyRemainingCalcBuf(size_t calculatedSampleCount)
{
    const auto remainingSamples = calcBufSize - calculatedSampleCount;

    if (remainingSamples > 0)
        std::memcpy(calcBuf.get(), calcBuf.get() + calculatedSampleCount * sampleSize, remainingSamples * sampleSize);

    calcBufSize = remainingSamples;
}

void StatisticsFbImpl::resetCalcBuf()
{
    calcBufSize = 0;
    calcBufAllocatedSize = 0;
    calcBuf.reset();
}

void StatisticsFbImpl::getNextOutputDomainValue(const DataPacketPtr& domainPacket, NumberPtr& outputPacketStartDomainValue, bool& haveGap)
{
    const auto sampleCount = domainPacket.getSampleCount();
    const auto packetStartDomainValue = domainPacket.getOffset();

    if (nextExpectedDomainValue == std::numeric_limits<Int>::max())
    {
        outputPacketStartDomainValue = packetStartDomainValue;
        haveGap = false;
    }
    else
    {
        if (packetStartDomainValue == nextExpectedDomainValue)
        {
            outputPacketStartDomainValue = addNumbers(packetStartDomainValue, -static_cast<Int>(calcBufSize) * inputDeltaTicks);
            haveGap = false;
        }
        else
        {
            outputPacketStartDomainValue = packetStartDomainValue;
            haveGap = true;
        }
    }

    nextExpectedDomainValue = addNumbers(packetStartDomainValue, static_cast<Int>(sampleCount) * inputDeltaTicks);
}

void StatisticsFbImpl::processSignalDescriptorChanged(const DataDescriptorPtr& valueDataDescriptor,
                                                      const DataDescriptorPtr& domainDataDescriptor)
{
    if (valueDataDescriptor.assigned())
        inputValueDataDescriptor = valueDataDescriptor;
    if (domainDataDescriptor.assigned())
        inputDomainDataDescriptor = domainDataDescriptor;

    configure();
}

void StatisticsFbImpl::processDataPacketTrigger(const DataPacketPtr& packet)
{
    const auto domainPacket = packet.getDomainPacket();
    auto data = static_cast<Bool*>(packet.getData());
    // Data packet from trigger only holds one value by design
    auto triggerData = data[0];
    // Domain packet from trigger only holds one value by design
    auto domainStamp = static_cast<Int*>(domainPacket.getData())[0];
    triggerHistory.addElement(triggerData, domainStamp);
}

void StatisticsFbImpl::processDataPacketInput(const DataPacketPtr& packet)
{
    if (!valid)
        return;

    const auto domainPacket = packet.getDomainPacket();
    if (!domainPacket.assigned())
        return;

    // TODO what if trigger condition changes multiple times in one packet???
    auto domainStamp = static_cast<Int*>(domainPacket.getData())[0];

    // TODO Check trigger mode and if on, check trigger state at domain stamp
    if (triggerMode && !triggerHistory.getTriggerStateFromDomainValue(domainStamp))
    {
        return;
    }

    bool haveGap;
    NumberPtr outputPacketStartDomainValue = 0;
    getNextOutputDomainValue(domainPacket, outputPacketStartDomainValue, haveGap);
    if (haveGap)
        resetCalcBuf();

    const auto availSamples = packet.getSampleCount();
    checkCalcBuf(availSamples);

    const auto packetBuf = static_cast<uint8_t*>(packet.getData());
    copyToCalcBuf(packetBuf, availSamples);

    const auto outSampleCount = calcBufSize / blockSize;

    if (outSampleCount == 0)
        return;

    const auto outDomainPacket = DataPacket(outputDomainDataDescriptor,
                                            outSampleCount,
                                            domainSignalType == DomainSignalType::implicit ? outputPacketStartDomainValue : nullptr);
    const auto outDomainPacketBuf = static_cast<uint8_t*>(outDomainPacket.getRawData());

    const bool calcAvg = avgSignal.getActive();
    const bool calcRms = rmsSignal.getActive();

    uint8_t* outAvgDataPacketBuf = nullptr;
    uint8_t* outRmsDataPacketBuf = nullptr;
    DataPacketPtr avgDataPacket;
    DataPacketPtr rmsDataPacket;

    if (calcAvg)
    {
        avgDataPacket = DataPacketWithDomain(outDomainPacket, outputAverageDataDescriptor, outSampleCount);

        outAvgDataPacketBuf = static_cast<uint8_t*>(avgDataPacket.getRawData());
    }

    if (calcRms)
    {
        rmsDataPacket = DataPacketWithDomain(outDomainPacket, outputRmsDataDescriptor, outSampleCount);

        outRmsDataPacketBuf = static_cast<uint8_t*>(rmsDataPacket.getRawData());
    }

    calculate(calcBuf.get(), outputPacketStartDomainValue, outAvgDataPacketBuf, outRmsDataPacketBuf, outDomainPacketBuf, outSampleCount);

    copyRemainingCalcBuf(outSampleCount * blockSize);

    if (calcAvg)
        avgSignal.sendPacket(avgDataPacket);

    if (calcRms)
        rmsSignal.sendPacket(rmsDataPacket);

    domainSignal.sendPacket(outDomainPacket);
}

NumberPtr StatisticsFbImpl::addNumbers(const NumberPtr a, const NumberPtr& b)
{
    if (a.getCoreType() == CoreType::ctFloat)
        return a.getFloatValue() + b.getFloatValue();
    else
        return a.getIntValue() + b.getIntValue();
}

template <SampleType ST, SampleType DST, SampleType AT, class SampleT, class AggT, class DomainSampleT>
void StatisticsFbImpl::calc(
    SampleT* data, int64_t firstTick, SampleT* outAvgData, SampleT* outRmsData, DomainSampleT* outDomainData, size_t avgCount)
{
    for (size_t i = 0; i < avgCount; ++i)
    {
        AggT sumAvg = 0;
        AggT sumRms = 0;
        for (size_t j = 0; j < blockSize; ++j)
        {
            const auto val = *data++;
            if (outAvgData != nullptr)
                sumAvg += val;
            if (outRmsData != nullptr)
                sumRms += val * val;
        }

        if (outAvgData != nullptr)
            *outAvgData++ = sumAvg / static_cast<AggT>(blockSize);
        if (outRmsData != nullptr)
            *outRmsData++ = std::sqrt(sumRms / static_cast<AggT>(blockSize));

        if (outDomainData)
        {
            if constexpr (DST == SampleType::Int64)
            {
                *outDomainData++ = firstTick + start;
                firstTick += outputDeltaTicks;
            }
            else if constexpr (DST == SampleType::RangeInt64)
            {
                outDomainData->start = firstTick + start;
                outDomainData->end = outDomainData->start + inputDeltaTicks * blockSize - 1;
                ++outDomainData;
                firstTick += outputDeltaTicks;
            }
        }
    }
}

template <SampleType ST, SampleType DST, SampleType AT, class SampleT, class AggT, class DomainSampleT>
void StatisticsFbImpl::calcUntyped(
    uint8_t* data, int64_t firstTick, uint8_t* outAvgData, uint8_t* outRmsData, uint8_t* outDomainData, size_t avgCount)
{
    auto* dataTyped = reinterpret_cast<SampleT*>(data);
    auto* outAvgDataTyped = reinterpret_cast<SampleT*>(outAvgData);
    auto* outRmsDataTyped = reinterpret_cast<SampleT*>(outRmsData);
    auto* outDomainDataTyped = reinterpret_cast<DomainSampleT*>(outDomainData);

    calc<ST, DST, AT, SampleT, AggT>(dataTyped, firstTick, outAvgDataTyped, outRmsDataTyped, outDomainDataTyped, avgCount);
}

void StatisticsFbImpl::calculate(
    uint8_t* data, int64_t firstTick, uint8_t* outAvgData, uint8_t* outRmsData, uint8_t* outDomainData, size_t avgCount)
{
    switch (domainSignalType)
    {
        case DomainSignalType::implicit:
            switch (sampleType)
            {
                case SampleType::Float32:
                    calcUntyped<SampleType::Float32, SampleType::Invalid>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Float64:
                    calcUntyped<SampleType::Float64, SampleType::Invalid>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::UInt8:
                    calcUntyped<SampleType::UInt8, SampleType::Invalid>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Int8:
                    calcUntyped<SampleType::Int8, SampleType::Invalid>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::UInt16:
                    calcUntyped<SampleType::UInt16, SampleType::Invalid>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Int16:
                    calcUntyped<SampleType::Int16, SampleType::Invalid>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::UInt32:
                    calcUntyped<SampleType::UInt32, SampleType::Invalid>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Int32:
                    calcUntyped<SampleType::Int32, SampleType::Invalid>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::UInt64:
                    calcUntyped<SampleType::UInt64, SampleType::Invalid>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Int64:
                    calcUntyped<SampleType::Int64, SampleType::Invalid>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                default:
                    LOG_C("Incompatible domain sample type {}", convertSampleTypeToString(sampleType));
                    assert(false);
            }
            break;
        case DomainSignalType::explicit_:
            switch (sampleType)
            {
                case SampleType::Float32:
                    calcUntyped<SampleType::Float32, SampleType::Int64>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Float64:
                    calcUntyped<SampleType::Float64, SampleType::Int64>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::UInt8:
                    calcUntyped<SampleType::UInt8, SampleType::Int64>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Int8:
                    calcUntyped<SampleType::Int8, SampleType::Int64>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::UInt16:
                    calcUntyped<SampleType::Int16, SampleType::Int64>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Int16:
                    calcUntyped<SampleType::Int16, SampleType::Int64>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::UInt32:
                    calcUntyped<SampleType::UInt32, SampleType::Int64>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Int32:
                    calcUntyped<SampleType::Int32, SampleType::Int64>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::UInt64:
                    calcUntyped<SampleType::UInt64, SampleType::Int64>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Int64:
                    calcUntyped<SampleType::Int64, SampleType::Int64>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                default:
                    LOG_C("Incompatible domain sample type {}", convertSampleTypeToString(sampleType));
                    assert(false);
            }
            break;
        case DomainSignalType::explicitRange:
            switch (sampleType)
            {
                case SampleType::Float32:
                    calcUntyped<SampleType::Float32, SampleType::RangeInt64>(
                        data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Float64:
                    calcUntyped<SampleType::Float64, SampleType::RangeInt64>(
                        data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::UInt8:
                    calcUntyped<SampleType::UInt8, SampleType::RangeInt64>(
                        data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Int8:
                    calcUntyped<SampleType::Int8, SampleType::RangeInt64>(data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::UInt16:
                    calcUntyped<SampleType::UInt16, SampleType::RangeInt64>(
                        data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Int16:
                    calcUntyped<SampleType::Int16, SampleType::RangeInt64>(
                        data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::UInt32:
                    calcUntyped<SampleType::UInt32, SampleType::RangeInt64>(
                        data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Int32:
                    calcUntyped<SampleType::Int32, SampleType::RangeInt64>(
                        data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::UInt64:
                    calcUntyped<SampleType::UInt64, SampleType::RangeInt64>(
                        data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                case SampleType::Int64:
                    calcUntyped<SampleType::Int64, SampleType::RangeInt64>(
                        data, firstTick, outAvgData, outRmsData, outDomainData, avgCount);
                    break;
                default:
                    LOG_C("Incompatible domain sample type {}", convertSampleTypeToString(sampleType));
                    assert(false);
            }
            break;
    }
}

void StatisticsFbImpl::onPacketReceived(const InputPortPtr& port)
{
    if (port.getLocalId() == "trigger")
        processTriggerPackets(port);
    else
        processInputPackets(port);
}

void StatisticsFbImpl::processTriggerPackets(const InputPortPtr& port)
{
    std::scoped_lock lock(sync);

    const auto conn = port.getConnection();
    if (!conn.assigned())
        return;

    auto packet = conn.dequeue();
    while (packet.assigned())
    {
        if (packet.getType() == PacketType::Data)
        {
            auto dataPacket = packet.asPtr<IDataPacket>();
            processDataPacketTrigger(dataPacket);
        }

        packet = conn.dequeue();
    }
}

void StatisticsFbImpl::processInputPackets(const InputPortPtr& port)
{
    std::scoped_lock lock(sync);

    const auto conn = port.getConnection();
    if (!conn.assigned())
        return;

    auto packet = conn.dequeue();
    while (packet.assigned())
    {
        const auto packetType = packet.getType();
        if (packetType == PacketType::Event)
        {
            auto eventPacket = packet.asPtr<IEventPacket>(true);
            LOG_T("Processing {} event", eventPacket.getEventId())
            if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                DataDescriptorPtr valueSignalDescriptor = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                DataDescriptorPtr domainSignalDescriptor = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
                processSignalDescriptorChanged(valueSignalDescriptor, domainSignalDescriptor);
            }
        }
        else if (packetType == PacketType::Data)
        {
            auto dataPacket = packet.asPtr<IDataPacket>();
            processDataPacketInput(dataPacket);
        }

        packet = conn.dequeue();
    }
}
}

END_NAMESPACE_REF_FB_MODULE
