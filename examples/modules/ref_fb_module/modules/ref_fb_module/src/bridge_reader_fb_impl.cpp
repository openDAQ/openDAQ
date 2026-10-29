#include <ref_fb_module/bridge_reader_fb_impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/component_type_private.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>

#include <cmath>
#include <limits>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace BridgeReader
{

namespace
{

// Volts per unit of a voltage signal; 0 for any other unit. A signal without a unit is read as volts
double voltsPerUnit(const DataDescriptorPtr& descriptor)
{
    const auto unit = descriptor.getUnit();
    if (!unit.assigned() || !unit.getSymbol().assigned() || unit.getSymbol().getLength() == 0)
        return 1;
    const std::string symbol = unit.getSymbol();
    if (symbol == "V")
        return 1;
    if (symbol == "mV")
        return 1e-3;
    if (symbol == "uV" || symbol == "\xC2\xB5V")
        return 1e-6;
    return 0;
}

}

BridgeReaderFbImpl::BridgeReaderFbImpl(const ModuleInfoPtr& moduleInfo, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : ReaderFbBase(CreateType(moduleInfo), ctx, parent, localId)
{
    outputSignal = createAndAddSignal("Strain");
    outputSignal.setName("Strain");
    outputDomainSignal = createAndAddSignal("StrainDomain", nullptr, false);
    outputDomainSignal.setName("StrainDomain");
    outputSignal.setDomainSignal(outputDomainSignal);

    const auto add = [this](const PropertyPtr& property)
    {
        objPtr.addProperty(property);
        objPtr.getOnPropertyValueWrite(property.getName()) += [this](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { propertyChanged(); };
    };
    add(SelectionProperty("BridgeType", List<IString>("Quarter", "Half bending", "Full bending"), 0));
    add(FloatPropertyBuilder("GaugeFactor", 2.0).setMinValue(0.01).setMaxValue(1000.0).build());
    add(FloatPropertyBuilder("ZeroOffset", 0.0)
            .setUnit(Unit("mV/V"))
            .setDescription("The bridge ratio at zero strain, subtracted before the conversion")
            .build());
    add(SelectionProperty("OutputQuantity", List<IString>("Strain", "Ratio"), 0));
    add(FloatPropertyBuilder("MinExcitation", 0.5)
            .setUnit(Unit("V"))
            .setMinValue(0.0)
            .setDescription("Samples whose excitation is lower in magnitude yield NaN")
            .build());

    signalPort = createAndAddInputPort("Signal", PacketReadyNotification::Scheduler, nullptr, true);
    excitationPort = createAndAddInputPort("Excitation", PacketReadyNotification::Scheduler, nullptr, true);
    params.setInputs(List<IComponent>(signalPort, excitationPort));
    params.setMainInput(signalPort);  // pinned: the measurement defines the time axis
    params.setValueReadType(SampleType::Float64);
    propertyChanged();
    createReader();
}

FunctionBlockTypePtr BridgeReaderFbImpl::CreateType(const ModuleInfoPtr& moduleInfo)
{
    auto fbType = FunctionBlockType("RefFBModuleBridgeReader",
                                    "Bridge strain with reader",
                                    "Calculates ratiometric strain from a bridge output and its excitation using multi reader");
    checkErrorInfo(fbType.asPtr<IComponentTypePrivate>(true)->setModuleInfo(moduleInfo));
    return fbType;
}

void BridgeReaderFbImpl::propertyChanged()
{
    auto lock = getRecursiveConfigLock2();
    bridgeType = static_cast<BridgeType>(static_cast<Int>(objPtr.getPropertyValue("BridgeType")));
    quantity = static_cast<OutputQuantity>(static_cast<Int>(objPtr.getPropertyValue("OutputQuantity")));
    gaugeFactor = objPtr.getPropertyValue("GaugeFactor");
    zeroOffset = static_cast<double>(objPtr.getPropertyValue("ZeroOffset")) * 1e-3;
    minExcitation = objPtr.getPropertyValue("MinExcitation");
    if (!cached.empty())
        rebuildOutputDescriptor();
}

bool BridgeReaderFbImpl::accepts(const ComponentPtr&, const DataDescriptorPtr& descriptor, const ListPtr<IDataDescriptor>&)
{
    return scalarNumeric(descriptor) && voltsPerUnit(descriptor) > 0;
}

// The strain of a Wheatstone bridge from its ratio change, for the bridge configurations of the type
double BridgeReaderFbImpl::strain(double ratio) const
{
    switch (bridgeType)
    {
        case BridgeType::Quarter:
            return -4 * ratio / (gaugeFactor * (1 + 2 * ratio));
        case BridgeType::HalfBending:
            return -2 * ratio / gaugeFactor;
        case BridgeType::FullBending:
            return -ratio / gaugeFactor;
    }
    return std::numeric_limits<double>::quiet_NaN();
}

// The range follows from the bridge output's range over the lowest excitation the excitation's range allows, never
// below MinExcitation. Without a bridge output range, or across the quarter bridge's pole, there is none
void BridgeReaderFbImpl::rebuildOutputDescriptor()
{
    const auto signal = cached.find(signalPort.getGlobalId().toStdString());
    const auto excitation = cached.find(excitationPort.getGlobalId().toStdString());
    if (signal == cached.end() || excitation == cached.end())
        return;

    auto builder = DataDescriptorBuilder().setSampleType(SampleType::Float64);
    if (quantity == OutputQuantity::Strain)
        builder.setName("Strain").setUnit(Unit("\xC2\xB5m/m", -1, "micrometre per metre", "strain"));
    else
        builder.setName("Bridge ratio").setUnit(Unit("mV/V", -1, "millivolt per volt", "ratio"));

    if (const auto signalRange = signal->second.getValueRange(); signalRange.assigned())
    {
        double lowestExcitation = minExcitation;
        if (const auto range = excitation->second.getValueRange(); range.assigned())
        {
            const double low = range.getLowValue();
            const double high = range.getHighValue();
            if (low * high > 0)
                lowestExcitation = std::max(minExcitation, std::min(std::fabs(low), std::fabs(high)) * voltsPerUnit(excitation->second));
        }
        const double peak = std::max(std::fabs(static_cast<double>(signalRange.getLowValue())), std::fabs(static_cast<double>(signalRange.getHighValue())));
        const double bound = lowestExcitation > 0 ? peak * voltsPerUnit(signal->second) / lowestExcitation : 0;
        const double low = -bound - zeroOffset;
        const double high = bound - zeroOffset;
        if (quantity == OutputQuantity::Ratio && bound > 0)
            builder.setValueRange(Range(low * 1e3, high * 1e3));
        else if (bound > 0 && (bridgeType != BridgeType::Quarter || 1 + 2 * low > 0))
            builder.setValueRange(Range(std::min(strain(low), strain(high)) * 1e6, std::max(strain(low), strain(high)) * 1e6));
    }

    const auto descriptor = builder.build();
    if (outputSignal.getDescriptor() != descriptor)
        outputSignal.setDescriptor(descriptor);
    if (outputDomain.assigned() && outputDomainSignal.getDescriptor() != outputDomain)
        outputDomainSignal.setDescriptor(outputDomain);
}

void BridgeReaderFbImpl::processAndSend(SizeT count, SizeT packetOffset)
{
    const auto* signal = static_cast<const double*>(buffers[0]);
    const auto* excitation = static_cast<const double*>(buffers[1]);
    const double signalVolts = voltsPerUnit(cached[signalPort.getGlobalId().toStdString()]);
    const double excitationVolts = voltsPerUnit(cached[excitationPort.getGlobalId().toStdString()]);

    const auto domainPacket = DataPacket(outputDomainSignal.getDescriptor(), count, packetOffset);
    const auto valuePacket = DataPacketWithDomain(domainPacket, outputSignal.getDescriptor(), count);
    auto* out = static_cast<double*>(valuePacket.getRawData());
    for (SizeT i = 0; i < count; ++i)
    {
        const double ex = excitation[i] * excitationVolts;
        if (!(std::fabs(ex) >= minExcitation) || ex == 0)
        {
            out[i] = std::numeric_limits<double>::quiet_NaN();
            continue;
        }
        const double ratio = signal[i] * signalVolts / ex - zeroOffset;
        out[i] = quantity == OutputQuantity::Strain ? strain(ratio) * 1e6 : ratio * 1e3;
    }
    outputDomainSignal.sendPacket(domainPacket);
    outputSignal.sendPacket(valuePacket);
}

}

END_NAMESPACE_REF_FB_MODULE
