#include <ref_fb_module/power_reader_fb_impl.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/data_descriptor_ptr.h>

#include <opendaq/event_packet_ptr.h>
#include <opendaq/signal_factory.h>

#include <opendaq/custom_log.h>
#include <opendaq/event_packet_params.h>

#include <coreobjects/unit_factory.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/sample_type_traits.h>
#include <coreobjects/eval_value_factory.h>
#include <opendaq/reader_factory.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace PowerReader
{

PowerReaderFbImpl::PowerReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    createInputPorts();
    createSignals();
    initProperties();
    createReader();
}

void PowerReaderFbImpl::initProperties()
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

void PowerReaderFbImpl::propertyChanged(bool configure)
{
    std::scoped_lock lock(sync);
    readProperties();
    if (configure)
        this->configure(nullptr, nullptr, nullptr);
}

void PowerReaderFbImpl::readProperties()
{
    voltageScale = objPtr.getPropertyValue("VoltageScale");
    voltageOffset = objPtr.getPropertyValue("VoltageOffset");
    currentScale = objPtr.getPropertyValue("CurrentScale");
    currentOffset = objPtr.getPropertyValue("CurrentOffset");
    useCustomOutputRange = objPtr.getPropertyValue("UseCustomOutputRange");
    powerHighValue = objPtr.getPropertyValue("CustomHighValue");
    powerLowValue = objPtr.getPropertyValue("CustomLowValue");
}

FunctionBlockTypePtr PowerReaderFbImpl::CreateType()
{
    return FunctionBlockType("RefFBModulePowerReader", "Power with reader", "Calculates power using multi reader");
}

bool PowerReaderFbImpl::getDataDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc)
{
    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        valueDesc = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        return true;
    }
    return false;
}

bool PowerReaderFbImpl::getDomainDescriptor(const EventPacketPtr& eventPacket, DataDescriptorPtr& domainDesc)
{
    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        domainDesc = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        return true;
    }
    return false;
}

void PowerReaderFbImpl::onDataReceived()
{
    std::scoped_lock lock(sync);

    SizeT cnt = reader.getAvailableCount();
    const auto voltageData = std::make_unique<double[]>(cnt);
    const auto currentData = std::make_unique<double[]>(cnt);
    std::array<double*, 2> data{voltageData.get(), currentData.get()};

    const MultiReaderStatusPtr status = reader.read(data.data(), &cnt);

    if (cnt > 0)
    {
        const auto powerDomainPacket = DataPacket(powerDomainSignal.getDescriptor(), cnt, status.getOffset());
        const auto powerValuePacket = DataPacketWithDomain(powerDomainPacket, powerSignal.getDescriptor(), cnt);
        double* powerValueData = static_cast<double*>(powerValuePacket.getRawData());

        for (size_t i = 0; i < cnt; i++)
            *powerValueData++ = (voltageScale * voltageData[i] + voltageOffset) * currentData[i];

        powerDomainSignal.sendPacket(powerDomainPacket);
        powerSignal.sendPacket(powerValuePacket);
    }

    if (status.getReadStatus() == ReadStatus::Event)
    {
        const auto eventPackets = status.getEventPackets();
        if (eventPackets.getCount() > 0)
        {
            DataDescriptorPtr domainDescriptor;
            DataDescriptorPtr voltageDescriptor;
            DataDescriptorPtr currentDescriptor;

            if (eventPackets.hasKey(voltageInputPort.getGlobalId()))
                getDataDescriptor(eventPackets.get(voltageInputPort.getGlobalId()), voltageDescriptor);

            if (eventPackets.hasKey(currentInputPort.getGlobalId()))
                getDataDescriptor(eventPackets.get(currentInputPort.getGlobalId()), currentDescriptor);

            getDomainDescriptor(status.getMainDescriptor(), domainDescriptor);

            if (voltageDescriptor.assigned() || currentDescriptor.assigned())
                configure(domainDescriptor, voltageDescriptor, currentDescriptor);
        }
    }
}

RangePtr PowerReaderFbImpl::getValueRange(DataDescriptorPtr voltageDataDescriptor, DataDescriptorPtr currentDataDescriptor)
{
    const auto voltageRange = voltageDataDescriptor.getValueRange();
    const auto currentRange = currentDataDescriptor.getValueRange();
    if (!voltageRange.assigned() || !currentRange.assigned())
        return nullptr;

    const Float voltageHigh = voltageRange.getHighValue();
    const Float voltageLow = voltageRange.getLowValue();
    const Float currentHigh = currentRange.getHighValue();
    const Float currentLow = currentRange.getLowValue();

    Float val1 = voltageHigh * currentHigh;
    Float val2 = voltageHigh * currentLow;
    Float val3 = voltageLow * currentHigh;
    Float val4 = voltageLow * currentLow;

    const Float powerHigh = std::max({val1, val2, val3, val4});
    const Float powerLow = std::min({val1, val2, val3, val4});

    return Range(powerLow, powerHigh);
}

void PowerReaderFbImpl::configure(const DataDescriptorPtr& domainDescriptor, const DataDescriptorPtr& voltageDescriptor, const DataDescriptorPtr& currentDescriptor)
{
    try
    {
        if (domainDescriptor.assigned())
            this->domainDescriptor = domainDescriptor;
        if (voltageDescriptor.assigned())
            this->voltageDescriptor = voltageDescriptor;
        if (currentDescriptor.assigned())
            this->currentDescriptor = currentDescriptor;

        const auto powerDataDescriptorBuilder =
            DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("W", -1, "watt", "power"));

        RangePtr powerRange;
        if (useCustomOutputRange)
            powerRange = Range(powerLowValue, powerHighValue);
        else
            powerRange = getValueRange(this->voltageDescriptor, this->currentDescriptor);

        powerDataDescriptor = powerDataDescriptorBuilder.setValueRange(powerRange).setName("Power").build();
        powerSignal.setDescriptor(powerDataDescriptor);

        powerDomainDataDescriptor = DataDescriptorBuilderCopy(this->domainDescriptor).setName("Power domain").build();

        powerDomainSignal.setDescriptor(powerDomainDataDescriptor);
    }
    catch (const std::exception& e)
    {
        LOG_W("Failed to set descriptor for power signal: {}", e.what())
        powerSignal.setDescriptor(nullptr);
    }
}

void PowerReaderFbImpl::createInputPorts()
{
    voltageInputPort = createAndAddInputPort("voltage", PacketReadyNotification::Scheduler, nullptr, true);
    currentInputPort = createAndAddInputPort("current", PacketReadyNotification::Scheduler, nullptr, true);
}

void PowerReaderFbImpl::createReader()
{
    reader = MultiReaderBuilder()
        .addInputPort(voltageInputPort)
        .addInputPort(currentInputPort)
        .setDomainReadType(SampleType::Int64)
        .setValueReadType(SampleType::Float64)
        .build();

    auto thisWeakRef = this->template getWeakRefInternal<IFunctionBlock>();
    reader.setOnDataAvailable([this, thisWeakRef = std::move(thisWeakRef)]
        {
            const auto thisFb = thisWeakRef.getRef();
            if (thisFb.assigned())
                this->onDataReceived();
        });
}

void PowerReaderFbImpl::createSignals()
{
    powerSignal = createAndAddSignal("power");
    powerSignal.setName("Power");
    powerDomainSignal = createAndAddSignal("power_domain", nullptr, false);
    powerDomainSignal.setName("PowerDomain");
    powerSignal.setDomainSignal(powerDomainSignal);
}

}

END_NAMESPACE_REF_FB_MODULE
