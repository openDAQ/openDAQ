#include <ref_fb_module/power_reader_fb_impl.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/data_descriptor_ptr.h>

#include <opendaq/event_packet_ptr.h>
#include <opendaq/signal_factory.h>

#include <opendaq/event_packet_params.h>

#include <coreobjects/unit_factory.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/sample_type_traits.h>
#include <coreobjects/eval_value_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/reader_config_ptr.h>
#include <opendaq/component_type_private.h>

BEGIN_NAMESPACE_REF_FB_MODULE
namespace PowerReader
{

PowerReaderFbImpl::PowerReaderFbImpl(const ModuleInfoPtr& moduleInfo,
                                     const ContextPtr& ctx,
                                     const ComponentPtr& parent,
                                     const StringPtr& localId)
    : FunctionBlock(CreateType(moduleInfo), ctx, parent, localId)
{
    initComponentStatus();
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

    const auto tickOffsetToleranceUsProp = IntProperty("TickOffsetToleranceUs", 0.0);
    objPtr.addProperty(tickOffsetToleranceUsProp);
    objPtr.getOnPropertyValueWrite("TickOffsetToleranceUs") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); createReader(); };

    readProperties();
}

void PowerReaderFbImpl::propertyChanged(bool configure)
{
    auto lock = getRecursiveConfigLock();
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
    tickOffsetToleranceUs = std::chrono::milliseconds(objPtr.getPropertyValue("TickOffsetToleranceUs"));
}

FunctionBlockTypePtr PowerReaderFbImpl::CreateType(const ModuleInfoPtr& moduleInfo)
{
    auto fbType = FunctionBlockType("RefFBModulePowerReader", "Power with reader", "Calculates power using multi reader");
    checkErrorInfo(fbType.asPtr<IComponentTypePrivate>(true)->setModuleInfo(moduleInfo));
    return fbType;
}

bool PowerReaderFbImpl::descriptorNotNull(const DataDescriptorPtr& descriptor)
{
    return descriptor.assigned() && descriptor != NullDataDescriptor();
}

void PowerReaderFbImpl::getDataDescriptors(const EventPacketPtr& eventPacket, DataDescriptorPtr& valueDesc, DataDescriptorPtr& domainDesc)
{
    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        valueDesc = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        domainDesc = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
    }
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
    auto lock = this->getAcquisitionLock();

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

            bool domainChanged = false;
            if (eventPackets.hasKey(voltageInputPort.getGlobalId()))
            {
                getDataDescriptors(eventPackets.get(voltageInputPort.getGlobalId()), voltageDescriptor, domainDescriptor);
                domainChanged = descriptorNotNull(domainDescriptor);
            }


            if (eventPackets.hasKey(currentInputPort.getGlobalId()))
            {
                getDataDescriptors(eventPackets.get(currentInputPort.getGlobalId()), currentDescriptor, domainDescriptor);
                domainChanged |= descriptorNotNull(domainDescriptor);
            }
                
            getDomainDescriptor(status.getMainDescriptor(), domainDescriptor);

            if (voltageDescriptor.assigned() || currentDescriptor.assigned() || domainChanged)
                configure(domainDescriptor, voltageDescriptor, currentDescriptor);
        }

        if (!status.getValid())
        {
            reader = MultiReaderFromExisting(reader, SampleType::Float64, SampleType::Int64);
        }
    }
}

void PowerReaderFbImpl::checkPortConnections() const
{
    for (const auto& port : reader.asPtr<IReaderConfig>().getInputPorts())
    {
        if (!port.getConnection().assigned())
        {
            setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Port {} is not connected!", port.getLocalId()));
            return;
        }
    }
    
    setComponentStatus(ComponentStatus::Ok);
}

void PowerReaderFbImpl::onConnected(const InputPortPtr& inputPort)
{
    LOG_D("Power Reader FB: Input port {} connected", inputPort.getLocalId())
    checkPortConnections();
}

void PowerReaderFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    LOG_D("Power Reader FB: Input port {} disconnected", inputPort.getLocalId())
    checkPortConnections();
}

RangePtr PowerReaderFbImpl::getValueRange(const DataDescriptorPtr& voltageDataDescriptor, const DataDescriptorPtr& currentDataDescriptor)
{
    const auto voltageRange = voltageDataDescriptor.getValueRange();
    const auto currentRange = currentDataDescriptor.getValueRange();
    if (!voltageRange.assigned() || !currentRange.assigned())
        return powerRange;

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

        if (this->domainDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("Input domain descriptor is not set");
        }
        if (this->voltageDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("Input voltage descriptor is not set");
        }
            
        if (this->currentDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("Input current descriptor is not set");
        }

        if (this->voltageDescriptor.assigned() && this->voltageDescriptor.getUnit().assigned() &&
            this->voltageDescriptor.getUnit().getSymbol() != "V")
        {
            throw std::runtime_error("Invalid voltage signal unit");
        }

        const auto powerDataDescriptorBuilder =
            DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("W", -1, "watt", "power"));

        if (useCustomOutputRange)
            powerRange = Range(powerLowValue, powerHighValue);
        else
            powerRange = getValueRange(this->voltageDescriptor, this->currentDescriptor);

        powerDataDescriptor = powerDataDescriptorBuilder.setValueRange(powerRange).build();
        powerDomainDataDescriptor = this->domainDescriptor;

        reader.setActive(True);

        powerSignal.setDescriptor(powerDataDescriptor);
        powerDomainSignal.setDescriptor(powerDomainDataDescriptor);

        setComponentStatus(ComponentStatus::Ok);
        reader.setActive(True);
    }
    catch (const std::exception& e)
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Failed to set descriptor for power signal: {}", e.what()));
        reader.setActive(False);
    }
}

void PowerReaderFbImpl::createInputPorts()
{
    voltageInputPort = createAndAddInputPort("Voltage", PacketReadyNotification::Scheduler, nullptr, true);
    currentInputPort = createAndAddInputPort("Current", PacketReadyNotification::Scheduler, nullptr, true);
    
    setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Port {} is not connected!", voltageInputPort.getLocalId()));
}

void PowerReaderFbImpl::createReader()
{
    auto tolerance = SimplifiedRatio(tickOffsetToleranceUs.count(), 1'000'000);
    tolerance = tolerance.simplify();

    reader.release();

    reader = MultiReaderBuilder()
        .addInputPort(voltageInputPort)
        .addInputPort(currentInputPort)
        .setDomainReadType(SampleType::Int64)
        .setValueReadType(SampleType::Float64)
        .setTickOffsetTolerance(tolerance)
        .setAllowDifferentSamplingRates(false)
        .setInputPortNotificationMethod(PacketReadyNotification::Unspecified)
        .build();

    reader.setExternalListener(this->objPtr);
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
    powerSignal = createAndAddSignal("Power");
    powerSignal.setName("Power");
    powerDomainSignal = createAndAddSignal("PowerDomain", nullptr, false);
    powerDomainSignal.setName("PowerDomain");
    powerSignal.setDomainSignal(powerDomainSignal);
}
}

END_NAMESPACE_REF_FB_MODULE
