#include <ref_fb_module/power_fb_impl.h>
#include <opendaq/function_block_ptr.h>
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

BEGIN_NAMESPACE_REF_FB_MODULE
    namespace Power
{

PowerFbImpl::PowerFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initComponentStatus();
    createInputPorts();
    createSignals();
    initProperties();
}

void PowerFbImpl::initProperties()
{
    const auto voltageScaleProp = FloatProperty("VoltageScale", 1.0);
    objPtr.addProperty(voltageScaleProp);
    objPtr.getOnPropertyValueWrite("VoltageScale") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(false); };

    const auto voltageOffsetProp = FloatProperty("VoltageOffset", 0.0);
    objPtr.addProperty(voltageOffsetProp);
    objPtr.getOnPropertyValueWrite("VoltageOffset") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(false); };

    const auto currentScaleProp = FloatProperty("CurrentScale", 1.0);
    objPtr.addProperty(currentScaleProp);
    objPtr.getOnPropertyValueWrite("CurrentScale") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(false); };

    const auto currentOffsetProp = FloatProperty("CurrentOffset", 0.0);
    objPtr.addProperty(currentOffsetProp);
    objPtr.getOnPropertyValueWrite("CurrentOffset") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(false); };

    const auto customHighValueProp = FloatProperty("CustomHighValue", 10.0, EvalValue("$UseCustomOutputRange"));
    objPtr.addProperty(customHighValueProp);
    objPtr.getOnPropertyValueWrite("CustomHighValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto customLowValueProp = FloatProperty("CustomLowValue", -10.0, EvalValue("$UseCustomOutputRange"));
    objPtr.addProperty(customLowValueProp);
    objPtr.getOnPropertyValueWrite("CustomLowValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto useCustomOutputRangeProp = BoolProperty("UseCustomOutputRange", False);
    objPtr.addProperty(useCustomOutputRangeProp);
    objPtr.getOnPropertyValueWrite("UseCustomOutputRange") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    readProperties();
}

void PowerFbImpl::propertyChanged(bool configure)
{
    readProperties();
    if (configure)
        this->configure(false);
}

void PowerFbImpl::readProperties()
{
    voltageScale = objPtr.getPropertyValue("VoltageScale");
    voltageOffset = objPtr.getPropertyValue("VoltageOffset");
    currentScale = objPtr.getPropertyValue("CurrentScale");
    currentOffset = objPtr.getPropertyValue("CurrentOffset");
    useCustomOutputRange = objPtr.getPropertyValue("UseCustomOutputRange");
    powerHighValue = objPtr.getPropertyValue("CustomHighValue");
    powerLowValue = objPtr.getPropertyValue("CustomLowValue");
}

FunctionBlockTypePtr PowerFbImpl::CreateType()
{
    return FunctionBlockType("RefFBModulePower", "Power", "Calculates power");
}

void PowerFbImpl::onPacketReceived(const InputPortPtr& port)
{
    processPackets();
}

void PowerFbImpl::processSignalDescriptorChanged(const DataDescriptorPtr& voltageDataDescriptor,
                                                 const DataDescriptorPtr& voltageDomainDataDescriptor,
                                                 const DataDescriptorPtr& currentDataDescriptor,
                                                 const DataDescriptorPtr& currentDomainDataDescriptor)
{
    if (voltageDataDescriptor.assigned())
        this->voltageDataDescriptor = voltageDataDescriptor;
    if (voltageDomainDataDescriptor.assigned())
        this->voltageDomainDataDescriptor = voltageDomainDataDescriptor;
    if (currentDataDescriptor.assigned())
        this->currentDataDescriptor = currentDataDescriptor;
    if (currentDomainDataDescriptor.assigned())
        this->currentDomainDataDescriptor = currentDomainDataDescriptor;

    configure(true);
}

void PowerFbImpl::processPackets()
{
    auto lock = this->getAcquisitionLock();

    PacketPtr voltagePacket;
    PacketPtr currentPacket;
    const auto voltageConnection = voltageInputPort.getConnection();
    const auto currentConnection = currentInputPort.getConnection();

    do
    {
        if (voltageConnection.assigned())
        {
            voltagePacket = voltageConnection.dequeue();
            if (voltagePacket.assigned())
            {
                if (voltagePacket.supportsInterface<IEventPacket>())
                {
                    auto voltageEventPacket = voltagePacket.asPtr<IEventPacket>();

                    if (voltageEventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
                    {
                        // TODO handle Null-descriptor params ('Null' sample type descriptors)
                        DataDescriptorPtr valueDataDescriptor = voltageEventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                        DataDescriptorPtr domainDataDescriptor =
                            voltageEventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
                        processSignalDescriptorChanged(valueDataDescriptor, domainDataDescriptor, nullptr, nullptr);
                    }
                }

                if (voltagePacket.supportsInterface<IDataPacket>())
                {
                    voltageQueue.push_front(voltagePacket);
                    processDataPackets();
                }

                voltagePacket = voltageConnection.peek();
            }
        }

        if (currentConnection.assigned())
        {
            currentPacket = currentConnection.dequeue();
            if (currentPacket.assigned())
            {
                if (currentPacket.supportsInterface<IEventPacket>())
                {
                    auto currentEventPacket = currentPacket.asPtr<IEventPacket>();

                    if (currentEventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
                    {
                        // TODO handle Null-descriptor params ('Null' sample type descriptors)
                        DataDescriptorPtr valueSignalDescriptor =
                            currentEventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                        DataDescriptorPtr domainSignalDescriptor =
                            currentEventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
                        processSignalDescriptorChanged(nullptr, nullptr, valueSignalDescriptor, domainSignalDescriptor);
                    }
                }
                else if (currentPacket.supportsInterface<IDataPacket>())
                {
                    currentQueue.push_front(currentPacket);
                    processDataPackets();
                }

                currentPacket = currentConnection.peek();
            }
        }

    } while (currentPacket.assigned() || voltagePacket.assigned());
}

void PowerFbImpl::checkPacketQueues()
{
    while (voltageQueue.size() > 100)
    {
        voltageQueue.pop_back();
        setComponentStatusWithMessage(ComponentStatus::Warning, "Data lost, voltage packets skipped");
        LOG_W("Data lost, voltage packets skipped")
        synced = false;
    }
    while (currentQueue.size() > 100)
    {
        currentQueue.pop_back();
        setComponentStatusWithMessage(ComponentStatus::Warning, "Data lost, current packets skipped");
        LOG_W("Data lost, voltage packets skipped")
        synced = false;
    }
}

void PowerFbImpl::processDataPackets()
{
    checkPacketQueues();

    while (!synced && !voltageQueue.empty() && !currentQueue.empty())
    {
        auto voltagePacket = voltageQueue.back();
        auto voltageDomainPacket = voltagePacket.getDomainPacket();
        Int voltagePacketStart = voltageDomainPacket.getOffset();
        Int voltagePacketEnd = voltageDomainPacket.getOffset() + (voltageDomainPacket.getSampleCount() - 1) * delta + start;

        auto currentPacket = currentQueue.back();
        auto currentDomainPacket = currentPacket.getDomainPacket();
        Int currentPacketStart = currentDomainPacket.getOffset();
        Int currentPacketEnd = currentDomainPacket.getOffset() + (currentDomainPacket.getSampleCount() - 1) * delta + start;

        if (voltagePacketEnd < currentPacketStart)
            voltageQueue.pop_back();
        else if (currentPacketEnd < voltagePacketStart)
            currentQueue.pop_back();
        else
        {
            synced = true;
            if (voltagePacketStart >= currentPacketStart)
            {
                voltagePos = 0;
                currentPos = (voltagePacketStart - currentPacketStart) / delta;
                curDomainValue = currentPacketStart;
            }
            else
            {
                currentPos = 0;
                voltagePos = (currentPacketStart - voltagePacketStart) / delta;
                curDomainValue = voltagePacketStart;
            }
        }
    }

    if (!synced)
        return;

   while (!voltageQueue.empty() && !currentQueue.empty())
        processPacket();
}

void PowerFbImpl::processPacket()
{
   switch (voltageSampleType)
   {
        case SampleType::Float32:
            switch (currentSampleType)
            {
                case SampleType::Float32:
                    processPacketTemplated<SampleType::Float32, SampleType::Float32>();
                    break;
                case SampleType::Float64:
                    processPacketTemplated<SampleType::Float32, SampleType::Float64>();
                    break;
                default:
                    break;
            }
            break;
        case SampleType::Float64:
            switch (currentSampleType)
            {
                case SampleType::Float32:
                    processPacketTemplated<SampleType::Float64, SampleType::Float32>();
                    break;
                case SampleType::Float64:
                    processPacketTemplated<SampleType::Float64, SampleType::Float64>();
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
   }
}

template<SampleType VoltageSampleType, SampleType CurrentSampleType>
void PowerFbImpl::processPacketTemplated()
{
   using VoltageType = typename SampleTypeToType<VoltageSampleType>::Type;
   using CurrentType = typename SampleTypeToType<CurrentSampleType>::Type;

   auto voltagePacket = voltageQueue.back();
   auto voltageData = static_cast<VoltageType*>(voltagePacket.getData()) + voltagePos;

   auto currentPacket = currentQueue.back();
   auto currentData = static_cast<CurrentType*>(currentPacket.getData()) + currentPos;

   auto voltageSize = voltagePacket.getSampleCount();
   auto currentSize = currentPacket.getSampleCount();

   auto voltageSamplesToProcess = voltageSize - voltagePos;
   auto currentSamplesToProcess = currentSize - currentPos;
   auto newSamples = std::min(voltageSamplesToProcess, currentSamplesToProcess);

   const auto powerDomainPacket = DataPacket(powerDomainDataDescriptor, newSamples, curDomainValue);
   const auto powerPacket = DataPacketWithDomain(powerDomainPacket, powerDataDescriptor, newSamples);

   auto powerData = static_cast<Float*>(powerPacket.getData());

   for (size_t i = 0; i < newSamples; i++)
        *powerData++ = (voltageScale * *voltageData++ + voltageOffset) * (currentScale * *currentData++ + currentOffset);

   powerSignal.sendPacket(powerPacket);
   powerDomainSignal.sendPacket(powerDomainPacket);

   curDomainValue += newSamples * delta;
   voltagePos += newSamples;
   currentPos += newSamples;

   if (voltagePos == voltageSize)
   {
        voltagePos = 0;
        voltageQueue.pop_back();
   }

   if (currentPos == currentSize)
   {
        currentPos = 0;
        currentQueue.pop_back();
   }
   setComponentStatus(ComponentStatus::Ok);
}

RangePtr PowerFbImpl::getValueRange(DataDescriptorPtr voltageDataDescriptor, DataDescriptorPtr currentDataDescriptor)
{
    Float voltageHigh = voltageDataDescriptor.getValueRange().getHighValue();
    Float voltageLow = voltageDataDescriptor.getValueRange().getLowValue();
    Float currentHigh = currentDataDescriptor.getValueRange().getHighValue();
    Float currentLow = currentDataDescriptor.getValueRange().getLowValue();

    Float val1 = voltageHigh * currentHigh;
    Float val2 = voltageHigh * currentLow;
    Float val3 = voltageLow * currentHigh;
    Float val4 = voltageLow * currentLow;

    Float powerHigh = std::max({val1, val2, val3, val4});
    Float powerLow = std::min({val1, val2, val3, val4});

    return Range(powerLow, powerHigh);
}

void PowerFbImpl::configure(bool resync)
{
    if (resync)
    {
        synced = false;
        voltageQueue.clear();
        currentQueue.clear();
    }

    if (!voltageDataDescriptor.assigned() || !voltageDomainDataDescriptor.assigned() || !currentDataDescriptor.assigned() ||
        !currentDomainDataDescriptor.assigned())
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, "Incomplete signal descriptors");
        LOG_D("Incomplete signal descriptors")
        return;
    }

    try
    {
        if (voltageDataDescriptor.getDimensions().getCount() > 0)
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Arrays not supported");
            throw std::runtime_error("Arrays not supported");
        }

        if (currentDataDescriptor.getDimensions().getCount() > 0)
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Invalid sample typ");
            throw std::runtime_error("Invalid sample typ");
        }

        voltageSampleType = voltageDataDescriptor.getSampleType();
        if (voltageSampleType != SampleType::Float64 && voltageSampleType != SampleType::Float32)
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Invalid sample type");
            throw std::runtime_error("Invalid sample type");
        }

        currentSampleType = currentDataDescriptor.getSampleType();
        if (currentSampleType != SampleType::Float64 && currentSampleType != SampleType::Float32)
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Invalid sample type");
            throw std::runtime_error("Invalid sample type");
        }

        auto powerDataDescriptorBuilder =
            DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("W", -1, "watt", "power"));

        RangePtr powerRange;
        if (useCustomOutputRange)
            powerRange = Range(powerLowValue, powerHighValue);
        else
            powerRange = getValueRange(voltageDataDescriptor, currentDataDescriptor);

        powerDataDescriptor = powerDataDescriptorBuilder.setValueRange(powerRange).setName("Power").build();
        powerSignal.setDescriptor(powerDataDescriptor);

        if (voltageDomainDataDescriptor.getOrigin() != currentDomainDataDescriptor.getOrigin())
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Domain mismatch");
            throw std::runtime_error("Domain mismatch");
        }

        if (voltageDomainDataDescriptor.getTickResolution() != currentDomainDataDescriptor.getTickResolution())
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Domain tick resolution mismatch");
            throw std::runtime_error("Domain tick resolution mismatch");
        }

        if (voltageDomainDataDescriptor.getSampleType() != SampleType::Int64 &&
            voltageDomainDataDescriptor.getSampleType() != SampleType::UInt64)
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Invalid domain sample type");
            throw std::runtime_error("Invalid domain sample type");
        }

        if (currentDomainDataDescriptor.getSampleType() != SampleType::Int64 &&
            currentDomainDataDescriptor.getSampleType() != SampleType::UInt64)
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Invalid domain sample type");
            throw std::runtime_error("Invalid domain sample type");
        }

        if (voltageDomainDataDescriptor.getSampleType() != currentDomainDataDescriptor.getSampleType())
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Domain sample type mismatch");
            throw std::runtime_error("Domain sample type mismatch");
        }
            

        if (currentDomainDataDescriptor.getSampleType() != SampleType::Int64 &&
            currentDomainDataDescriptor.getSampleType() != SampleType::UInt64)
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Invalid domain sample type");
            throw std::runtime_error("Invalid domain sample type");
        }

        if (voltageDomainDataDescriptor.getUnit() != currentDomainDataDescriptor.getUnit())
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Domain unit mismatch");
            throw std::runtime_error("Domain unit mismatch");
        }

        const auto voltageDomainRule = voltageDomainDataDescriptor.getRule();
        if (!voltageDomainRule.assigned() || voltageDomainRule.getType() != DataRuleType::Linear)
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Linear rule not used");
            throw std::runtime_error("Linear rule not used");
        }

        const auto currentDomainRule = currentDomainDataDescriptor.getRule();
        if (!currentDomainRule.assigned() || currentDomainRule.getType() != DataRuleType::Linear)
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Linear rule not used");
            throw std::runtime_error("Linear rule not used");
        }

        const auto voltageDomainRuleParams = voltageDomainRule.getParameters();
        const auto currentDomainRuleParams = currentDomainRule.getParameters();
        if (voltageDomainRuleParams != currentDomainRuleParams)
        {
            setComponentStatusWithMessage(ComponentStatus::Error, "Domain rule mismatch");
            throw std::runtime_error("Domain rule mismatch");
        }

        powerDomainDataDescriptor = DataDescriptorBuilderCopy(voltageDomainDataDescriptor).setName("Power domain").build();

        powerDomainSignal.setDescriptor(powerDomainDataDescriptor);

        start = voltageDomainRuleParams.get("start");
        delta = voltageDomainRuleParams.get("delta");
    }
    catch (const std::exception& e)
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, "Failed to set descriptor for power signal");
        LOG_W("Failed to set descriptor for power signal: {}", e.what())
        powerSignal.setDescriptor(nullptr);
    }
    setComponentStatus(ComponentStatus::Ok);
}

void PowerFbImpl::createInputPorts()
{
    voltageInputPort = createAndAddInputPort("voltage", PacketReadyNotification::Scheduler);
    currentInputPort = createAndAddInputPort("current", PacketReadyNotification::Scheduler);
}

void PowerFbImpl::createSignals()
{
    powerSignal = createAndAddSignal("power");
    powerSignal.setName("Power");
    powerDomainSignal = createAndAddSignal("power_domain", nullptr, false);
    powerDomainSignal.setName("PowerDomain");
    powerSignal.setDomainSignal(powerDomainSignal);
}

void PowerFbImpl::onConnected(const InputPortPtr& inputPort)
{
    auto lock = this->getRecursiveConfigLock();
    LOG_T("Connected to port {}", inputPort.getLocalId())
}

void PowerFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    auto lock = this->getRecursiveConfigLock();
    LOG_T("Disconnected from port {}", inputPort.getLocalId())
}

}

END_NAMESPACE_REF_FB_MODULE
