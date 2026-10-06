#include <ref_fb_module/sum_reader_fb_impl.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/signal_factory.h>

#include <cmath>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace SumReader
{

SumReaderFbImpl::SumReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& /*config*/)
    : ReaderFbBase(CreateType(), ctx, parent, localId)
{
    setComponentStatusWithMessage(ComponentStatus::Warning, "No signals connected!");

    sumSignal = createAndAddSignal("Sum");
    sumSignal.setName("Sum");
    sumDomainSignal = createAndAddSignal("SumDomain", nullptr, false);
    sumDomainSignal.setName("SumDomain");
    sumSignal.setDomainSignal(sumDomainSignal);

    params.setInputs(List<IComponent>());
    params.setValueReadType(SampleType::Float64);
    params.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);  // an input that cannot align is set aside, the sum goes on
    createReader();
    addFreePort("SumPort");
}

FunctionBlockTypePtr SumReaderFbImpl::CreateType()
{
    return FunctionBlockType("RefFBModuleSumReader", "Sum with reader", "Calculates equal-rate signal sum using multi reader");
}

// The unit every input has to carry: that of any contributing input, as of the last status, other than the one
// being judged
UnitPtr SumReaderFbImpl::referenceUnit(const ComponentPtr& judged) const
{
    const auto judgedId = judged.getGlobalId().toStdString();
    for (const auto& [id, descriptor] : cached)
    {
        if (id != judgedId && descriptor.assigned())
            return descriptor.getUnit();
    }
    return nullptr;
}

bool SumReaderFbImpl::accepts(const ComponentPtr& input, const DataDescriptorPtr& descriptor)
{
    if (!descriptor.assigned())
        return false;
    const int sampleType = static_cast<int>(descriptor.getSampleType());
    if (sampleType == 0 || sampleType > static_cast<int>(SampleType::Int64))
        return false;  // scalar numeric types only
    if (const auto dimensions = descriptor.getDimensions(); dimensions.assigned() && dimensions.getCount() > 0)
        return false;
    const auto reference = referenceUnit(input);
    return !reference.assigned() || reference == descriptor.getUnit();
}

bool SumReaderFbImpl::onChanges(const MultiReader2StatusPtr& status)
{
    for (const MultiReader2InputStatusPtr in : status.getInputs())
    {
        const InputPortConfigPtr port = in.getInput().asPtr<IInputPortConfig>(true);
        if (in.getError() == MultiReader2InputError::Disconnected)
        {
            // A port lost its signal: the block offers one free port only, so this one goes
            LOG_D("Sum Reader FB: Input port {} disconnected, removing it", port.getLocalId())
            cached.erase(port.getGlobalId().toStdString());
            removeInputPort(port);
            params.setInputs(readerPorts());
            reader.configure(params);
            return true;
        }
    }

    return ReaderFbBase::onChanges(status);
}

void SumReaderFbImpl::processAndSend(SizeT count, SizeT packetOffset)
{
    if (!outputDomain.assigned() || !sumSignal.getDescriptor().assigned())
        return;

    const auto sumDomainPacket = DataPacket(sumDomainSignal.getDescriptor(), count, packetOffset);
    const auto sumValuePacket = DataPacketWithDomain(sumDomainPacket, sumSignal.getDescriptor(), count);
    const auto sumValueData = static_cast<double*>(sumValuePacket.getRawData());
    std::fill_n(sumValueData, count, 0.0);

    for (SizeT slot = 0; slot < buffers.size(); slot++)
    {
        if (buffers[slot] == nullptr)
            continue;
        const auto data = static_cast<const double*>(buffers[slot]);
        for (SizeT i = 0; i < count; i++)
            sumValueData[i] += data[i];
    }

    sumDomainSignal.sendPacket(sumDomainPacket);
    sumSignal.sendPacket(sumValuePacket);
}

void SumReaderFbImpl::rebuildOutputDescriptor()
{
    UnitPtr unit;
    double lowValue = 0;
    double highValue = 0;
    bool anyInput = false;
    for (const auto& [id, descriptor] : cached)
    {
        if (!descriptor.assigned())
            continue;
        anyInput = true;
        if (!unit.assigned())
            unit = descriptor.getUnit();
        if (const auto range = descriptor.getValueRange(); range.assigned())
        {
            lowValue += range.getLowValue().getFloatValue();
            highValue += range.getHighValue().getFloatValue();
        }
    }

    if (!anyInput || !outputDomain.assigned())
    {
        setComponentStatusWithMessage(ComponentStatus::Warning, "No signals connected!");
        return;
    }

    const RangePtr range = std::fabs(lowValue - highValue) > 1e-9 ? Range(lowValue, highValue) : Range(-10, 10);
    sumSignal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(unit).setValueRange(range).build());
    sumDomainSignal.setDescriptor(outputDomain);
}

}

END_NAMESPACE_REF_FB_MODULE
