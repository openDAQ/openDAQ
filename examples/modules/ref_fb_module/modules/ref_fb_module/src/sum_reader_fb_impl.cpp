#include <opendaq/data_descriptor_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/signal_factory.h>
#include <ref_fb_module/sum_reader_fb_impl.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace SumReader
{

SumReaderFbImpl::SumReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr&)
    : ReaderFbBase(CreateType(), ctx, parent, localId)
{
    sumSignal = createAndAddSignal("Sum");
    sumSignal.setName("Sum");
    sumDomainSignal = createAndAddSignal("SumDomain", nullptr, false);
    sumDomainSignal.setName("SumDomain");
    sumSignal.setDomainSignal(sumDomainSignal);

    setInputs(List<IComponent>());  // nothing to read yet
    params.setValueReadType(SampleType::Float64);
    params.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);  // a rejected or unalignable input is set aside
    createReader();
    addFreePort("SumPort");
}

FunctionBlockTypePtr SumReaderFbImpl::CreateType()
{
    return FunctionBlockType("RefFBModuleSumReader", "Sum with reader", "Calculates equal-rate signal sum using multi reader");
}

// The reference is the unit of the first input in slot order whose value descriptor is scalar numeric, this one
// included; a reference without a unit accepts every unit
bool SumReaderFbImpl::accepts(const ComponentPtr&, const DataDescriptorPtr& descriptor, const ListPtr<IDataDescriptor>& values)
{
    if (!scalarNumeric(descriptor))
        return false;
    for (const DataDescriptorPtr other : values)
        if (scalarNumeric(other))
            return !other.getUnit().assigned() || descriptor.getUnit() == other.getUnit();
    return true;
}

// After every status: the default caching and reporting, then remove disconnected ports. The configure returns a
// status that comes back here; the second pass finds nothing to do
void SumReaderFbImpl::onChanges(const MultiReader2StatusPtr& status)
{
    ReaderFbBase::onChanges(status);
    MultiReader2StatusPtr next;
    removeDisconnectedPorts(status, next);
    if (next.assigned())
        onChanges(next);
}

void SumReaderFbImpl::rebuildOutputDescriptor()
{
    if (cached.empty())
        return;  // no input yet: the output keeps the descriptor it has, none before the first input
    UnitPtr unit;
    double low = 0;
    double high = 0;
    for (const auto& input : params.getInputs())
    {
        const auto it = cached.find(input.getGlobalId().toStdString());
        if (it == cached.end())
            continue;
        if (!unit.assigned())
            unit = it->second.getUnit();
        if (const auto range = it->second.getValueRange(); range.assigned())
        {
            low += range.getLowValue().getFloatValue();
            high += range.getHighValue().getFloatValue();
        }
    }
    const auto descriptor = DataDescriptorBuilder()
                                .setSampleType(SampleType::Float64)
                                .setUnit(unit)
                                .setValueRange(std::fabs(low - high) > 1e-9 ? Range(low, high) : Range(-10, 10))
                                .build();
    if (sumSignal.getDescriptor() != descriptor)
        sumSignal.setDescriptor(descriptor);
    if (outputDomain.assigned() && sumDomainSignal.getDescriptor() != outputDomain)
        sumDomainSignal.setDescriptor(outputDomain);
}

void SumReaderFbImpl::processAndSend(SizeT count, SizeT packetOffset)
{
    const auto domainPacket = DataPacket(sumDomainSignal.getDescriptor(), count, packetOffset);
    const auto valuePacket = DataPacketWithDomain(domainPacket, sumSignal.getDescriptor(), count);
    auto* sum = static_cast<double*>(valuePacket.getRawData());
    std::fill_n(sum, count, 0.0);
    for (const void* buffer : buffers)
    {
        if (!buffer)
            continue;
        const auto* values = static_cast<const double*>(buffer);
        for (SizeT i = 0; i < count; ++i)
            sum[i] += values[i];
    }
    sumDomainSignal.sendPacket(domainPacket);
    sumSignal.sendPacket(valuePacket);
}

}

END_NAMESPACE_REF_FB_MODULE
