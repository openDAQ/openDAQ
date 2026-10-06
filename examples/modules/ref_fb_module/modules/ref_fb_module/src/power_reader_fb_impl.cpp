#include <ref_fb_module/power_reader_fb_impl.h>
#include <coreobjects/eval_value_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/component_type_private.h>
#include <opendaq/custom_log.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/signal_factory.h>

#include <algorithm>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace PowerReader
{

PowerReaderFbImpl::PowerReaderFbImpl(const ModuleInfoPtr& moduleInfo, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : ReaderFbBase(CreateType(moduleInfo), ctx, parent, localId)
{
    voltageInputPort = createAndAddInputPort("Voltage", PacketReadyNotification::Scheduler, nullptr, true);
    currentInputPort = createAndAddInputPort("Current", PacketReadyNotification::Scheduler, nullptr, true);
    setComponentStatusWithMessage(ComponentStatus::Warning, fmt::format("Port {} is not connected!", voltageInputPort.getLocalId()));

    powerSignal = createAndAddSignal("Power");
    powerSignal.setName("Power");
    powerDomainSignal = createAndAddSignal("PowerDomain", nullptr, false);
    powerDomainSignal.setName("PowerDomain");
    powerSignal.setDomainSignal(powerDomainSignal);

    initProperties();

    params.setInputs(List<IComponent>(voltageInputPort, currentInputPort));
    params.setMainInput(voltageInputPort);  // pinned: its error stops the block
    params.setValueReadType(SampleType::Float64);
    createReader();
}

FunctionBlockTypePtr PowerReaderFbImpl::CreateType(const ModuleInfoPtr& moduleInfo)
{
    auto fbType = FunctionBlockType("RefFBModulePowerReader", "Power with reader", "Calculates power using multi reader");
    checkErrorInfo(fbType.asPtr<IComponentTypePrivate>(true)->setModuleInfo(moduleInfo));
    return fbType;
}

void PowerReaderFbImpl::initProperties()
{
    // Property callbacks run under the object lock already held by the writer
    const auto onValueChange = [this](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { readProperties(); };
    const auto onRangeChange = [this](PropertyObjectPtr&, PropertyValueEventArgsPtr&)
    {
        readProperties();
        rebuildOutputDescriptor();
    };

    objPtr.addProperty(FloatProperty("VoltageScale", 1.0));
    objPtr.getOnPropertyValueWrite("VoltageScale") += onValueChange;
    objPtr.addProperty(FloatProperty("VoltageOffset", 0.0));
    objPtr.getOnPropertyValueWrite("VoltageOffset") += onValueChange;
    objPtr.addProperty(FloatProperty("CurrentScale", 1.0));
    objPtr.getOnPropertyValueWrite("CurrentScale") += onValueChange;
    objPtr.addProperty(FloatProperty("CurrentOffset", 0.0));
    objPtr.getOnPropertyValueWrite("CurrentOffset") += onValueChange;
    objPtr.addProperty(FloatProperty("CustomHighValue", 10.0, EvalValue("$UseCustomOutputRange")));
    objPtr.getOnPropertyValueWrite("CustomHighValue") += onRangeChange;
    objPtr.addProperty(FloatProperty("CustomLowValue", -10.0, EvalValue("$UseCustomOutputRange")));
    objPtr.getOnPropertyValueWrite("CustomLowValue") += onRangeChange;
    objPtr.addProperty(BoolProperty("UseCustomOutputRange", False));
    objPtr.getOnPropertyValueWrite("UseCustomOutputRange") += onRangeChange;

    readProperties();
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

RangePtr PowerReaderFbImpl::getValueRange(const DataDescriptorPtr& voltageDescriptor, const DataDescriptorPtr& currentDescriptor) const
{
    const auto voltageRange = voltageDescriptor.getValueRange();
    const auto currentRange = currentDescriptor.getValueRange();
    if (!voltageRange.assigned() || !currentRange.assigned())
        return powerRange;

    const Float voltageHigh = voltageRange.getHighValue();
    const Float voltageLow = voltageRange.getLowValue();
    const Float currentHigh = currentRange.getHighValue();
    const Float currentLow = currentRange.getLowValue();
    const Float corners[]{voltageHigh * currentHigh, voltageHigh * currentLow, voltageLow * currentHigh, voltageLow * currentLow};
    return Range(*std::min_element(std::begin(corners), std::end(corners)), *std::max_element(std::begin(corners), std::end(corners)));
}

// Volts on the voltage port. The port is the pinned main, so a rejected unit makes the reader invalid and the data
// is dropped by the reader, never read and discarded here
bool PowerReaderFbImpl::accepts(const ComponentPtr& input, const DataDescriptorPtr& descriptor)
{
    if (input.getGlobalId() != voltageInputPort.getGlobalId())
        return true;
    const auto unit = descriptor.getUnit();
    return !unit.assigned() || unit.getSymbol() == "V";
}

void PowerReaderFbImpl::processAndSend(SizeT count, SizeT packetOffset)
{
    if (!outputValid)
        return;  // Invalidate and a judgement only on the main input: data means both inputs contributed

    const auto powerDomainPacket = DataPacket(powerDomainSignal.getDescriptor(), count, packetOffset);
    const auto powerValuePacket = DataPacketWithDomain(powerDomainPacket, powerSignal.getDescriptor(), count);
    const auto powerValueData = static_cast<double*>(powerValuePacket.getRawData());
    const auto voltageData = static_cast<const double*>(buffers[0]);
    const auto currentData = static_cast<const double*>(buffers[1]);
    for (SizeT i = 0; i < count; i++)
        powerValueData[i] = (voltageScale * voltageData[i] + voltageOffset) * (currentScale * currentData[i] + currentOffset);

    powerDomainSignal.sendPacket(powerDomainPacket);
    powerSignal.sendPacket(powerValuePacket);
}

void PowerReaderFbImpl::rebuildOutputDescriptor()
{
    const auto voltageIt = cached.find(voltageInputPort.getGlobalId().toStdString());
    const auto currentIt = cached.find(currentInputPort.getGlobalId().toStdString());
    if (voltageIt == cached.end() || currentIt == cached.end() || !voltageIt->second.assigned() || !currentIt->second.assigned() || !outputDomain.assigned())
        return;

    try
    {
        const auto& voltageDescriptor = voltageIt->second;
        const auto& currentDescriptor = currentIt->second;
        powerRange = useCustomOutputRange ? Range(powerLowValue, powerHighValue) : getValueRange(voltageDescriptor, currentDescriptor);
        powerSignal.setDescriptor(DataDescriptorBuilder()
                                      .setSampleType(SampleType::Float64)
                                      .setUnit(Unit("W", -1, "watt", "power"))
                                      .setValueRange(powerRange)
                                      .build());
        powerDomainSignal.setDescriptor(outputDomain);
        outputValid = true;
        outputError.clear();
    }
    catch (const std::exception& e)
    {
        outputValid = false;
        outputError = fmt::format("Failed to set descriptor for power signal: {}", e.what());
    }
}

}

END_NAMESPACE_REF_FB_MODULE
