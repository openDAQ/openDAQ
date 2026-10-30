#include <coreobjects/eval_value_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/component_type_private.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/signal_factory.h>
#include <ref_fb_module/power_reader_fb_impl.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace PowerReader
{

PowerReaderFbImpl::PowerReaderFbImpl(const ModuleInfoPtr& moduleInfo,
                                     const ContextPtr& ctx,
                                     const ComponentPtr& parent,
                                     const StringPtr& localId)
    : ReaderFbBase(CreateType(moduleInfo), ctx, parent, localId)
{
    powerSignal = createAndAddSignal("Power");
    powerSignal.setName("Power");
    powerDomainSignal = createAndAddSignal("PowerDomain", nullptr, false);
    powerDomainSignal.setName("PowerDomain");
    powerSignal.setDomainSignal(powerDomainSignal);
    initProperties();

    voltageInputPort = createAndAddInputPort("Voltage", PacketReadyNotification::Scheduler, nullptr, true);
    currentInputPort = createAndAddInputPort("Current", PacketReadyNotification::Scheduler, nullptr, true);
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
    const auto add = [this](const PropertyPtr& property, bool rebuild)
    {
        objPtr.addProperty(property);
        objPtr.getOnPropertyValueWrite(property.getName()) +=
            [this, rebuild](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { propertyChanged(rebuild); };
    };
    add(FloatProperty("VoltageScale", 1.0), false);
    add(FloatProperty("VoltageOffset", 0.0), false);
    add(FloatProperty("CurrentScale", 1.0), false);
    add(FloatProperty("CurrentOffset", 0.0), false);
    add(FloatProperty("CustomHighValue", 10.0, EvalValue("$UseCustomOutputRange")), true);
    add(FloatProperty("CustomLowValue", -10.0, EvalValue("$UseCustomOutputRange")), true);
    add(BoolProperty("UseCustomOutputRange", False), true);
    readProperties();
}

void PowerReaderFbImpl::propertyChanged(bool rebuild)
{
    auto lock = getRecursiveConfigLock2();
    readProperties();
    if (rebuild)
        rebuildOutputDescriptor();
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

// Scalar numbers on both ports; volts on the voltage port
bool PowerReaderFbImpl::accepts(const ComponentPtr& input, const DataDescriptorPtr& descriptor)
{
    if (!scalarNumeric(descriptor))
        return false;
    if (input.getGlobalId() != voltageInputPort.getGlobalId())
        return true;
    const auto unit = descriptor.getUnit();
    return !unit.assigned() || unit.getSymbol() == "V";
}

RangePtr PowerReaderFbImpl::valueRange(const DataDescriptorPtr& voltage, const DataDescriptorPtr& current) const
{
    if (useCustomOutputRange)
        return Range(powerLowValue, powerHighValue);
    const auto voltageRange = voltage.getValueRange();
    const auto currentRange = current.getValueRange();
    if (!voltageRange.assigned() || !currentRange.assigned())
        return nullptr;
    const Float vh = voltageRange.getHighValue(), vl = voltageRange.getLowValue();
    const Float ch = currentRange.getHighValue(), cl = currentRange.getLowValue();
    return Range(std::min({vh * ch, vh * cl, vl * ch, vl * cl}), std::max({vh * ch, vh * cl, vl * ch, vl * cl}));
}

void PowerReaderFbImpl::rebuildOutputDescriptor()
{
    // Slot 0 is the voltage, slot 1 the current
    if (descriptors.size() != 2 || !descriptors[0].assigned() || !descriptors[1].assigned())
        return;
    powerSignal.setDescriptor(DataDescriptorBuilder()
                                  .setSampleType(SampleType::Float64)
                                  .setUnit(Unit("W", -1, "watt", "power"))
                                  .setValueRange(valueRange(descriptors[0], descriptors[1]))
                                  .build());
    if (outputDomain.assigned() && powerDomainSignal.getDescriptor() != outputDomain)
        powerDomainSignal.setDescriptor(outputDomain);
}

void PowerReaderFbImpl::processAndSend(SizeT count, SizeT packetOffset)
{
    const auto* voltage = static_cast<const double*>(buffers[0]);
    const auto* current = static_cast<const double*>(buffers[1]);
    const auto domainPacket = DataPacket(powerDomainSignal.getDescriptor(), count, packetOffset);
    const auto valuePacket = DataPacketWithDomain(domainPacket, powerSignal.getDescriptor(), count);
    auto* power = static_cast<double*>(valuePacket.getRawData());
    for (SizeT i = 0; i < count; ++i)
        power[i] = (voltageScale * voltage[i] + voltageOffset) * current[i];
    powerDomainSignal.sendPacket(domainPacket);
    powerSignal.sendPacket(valuePacket);
}

}

END_NAMESPACE_REF_FB_MODULE
