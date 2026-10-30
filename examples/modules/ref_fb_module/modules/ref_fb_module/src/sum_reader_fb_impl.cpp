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

    params.setValueReadType(SampleType::Float64);
    params.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);  // a rejected or unalignable input is set aside
    createReader();
    addFreePort("SumPort");
}

FunctionBlockTypePtr SumReaderFbImpl::CreateType()
{
    return FunctionBlockType("RefFBModuleSumReader", "Sum with reader", "Calculates equal-rate signal sum using multi reader");
}

bool SumReaderFbImpl::accepts(const ComponentPtr&, const DataDescriptorPtr& descriptor)
{
    return scalarNumeric(descriptor) && (!referenceUnit.assigned() || descriptor.getUnit() == referenceUnit);
}

// The main input sets the unit; when it moves, every input is judged again. With no main input, as when no input is
// healthy, every unit is accepted until one is
bool SumReaderFbImpl::onChanges(const MultiReader2StatusPtr& status)
{
    const auto main = status.getMainInput();
    const UnitPtr unit = main.assigned() ? status.getInputStatus(main).getDescriptor().getUnit() : nullptr;
    return std::exchange(referenceUnit, unit) != unit;
}

void SumReaderFbImpl::rebuildOutputDescriptor()
{
    double low = 0;
    double high = 0;
    for (const auto& input : params.getInputs())
    {
        const auto it = cached.find(input.getGlobalId().toStdString());
        if (it == cached.end())
            continue;
        if (const auto range = it->second.getValueRange(); range.assigned())
        {
            low += range.getLowValue().getFloatValue();
            high += range.getHighValue().getFloatValue();
        }
    }
    const auto descriptor = DataDescriptorBuilder()
                                .setSampleType(SampleType::Float64)
                                .setUnit(referenceUnit)
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
