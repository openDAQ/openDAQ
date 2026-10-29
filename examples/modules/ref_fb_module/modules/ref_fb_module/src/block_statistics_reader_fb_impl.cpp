#include <ref_fb_module/block_statistics_reader_fb_impl.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/component_type_private.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>

#include <cmath>
#include <limits>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace BlockStatisticsReader
{

namespace
{

const char* statisticNames[] = {"Mean", "RMS", "AC RMS", "Peak-to-peak", "Crest factor"};

Int floorMod(Int value, Int divisor)
{
    const Int r = value % divisor;
    return r < 0 ? r + divisor : r;
}

Int floorDiv(Int value, Int divisor)
{
    return (value - floorMod(value, divisor)) / divisor;
}

}

void BlockStatisticsReaderFbImpl::Accumulator::add(const double* x, SizeT count)
{
    for (SizeT i = 0; i < count; ++i)
    {
        const double v = x[i];
        if (n++ == 0)
            min = max = v;
        min = std::min(min, v);
        max = std::max(max, v);
        const double d = v - mean;
        mean += d / static_cast<double>(n);
        m2 += d * (v - mean);
        sumSq += v * v;
    }
}

double BlockStatisticsReaderFbImpl::Accumulator::value(Statistic statistic) const
{
    const double rms = std::sqrt(sumSq / static_cast<double>(n));
    switch (statistic)
    {
        case Statistic::Mean:
            return mean;
        case Statistic::Rms:
            return rms;
        case Statistic::AcRms:
            return std::sqrt(m2 / static_cast<double>(n));
        case Statistic::PeakToPeak:
            return max - min;
        case Statistic::CrestFactor:
            return rms > 0 ? std::max(std::fabs(min), std::fabs(max)) / rms : std::numeric_limits<double>::quiet_NaN();
    }
    return std::numeric_limits<double>::quiet_NaN();
}

BlockStatisticsReaderFbImpl::BlockStatisticsReaderFbImpl(const ModuleInfoPtr& moduleInfo,
                                                         const ContextPtr& ctx,
                                                         const ComponentPtr& parent,
                                                         const StringPtr& localId)
    : ReaderFbBase(CreateType(moduleInfo), ctx, parent, localId)
{
    domainSignal = createAndAddSignal("StatisticsDomain", nullptr, false);
    domainSignal.setName("StatisticsDomain");

    const auto add = [this](const PropertyPtr& property)
    {
        objPtr.addProperty(property);
        objPtr.getOnPropertyValueWrite(property.getName()) += [this](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { propertyChanged(); };
    };
    add(IntPropertyBuilder("BlockSize", 100)
            .setMinValue(1)
            .setMaxValue(10000000)
            .setDescription("Samples per block. Blocks start on the samples whose index from the domain origin is a multiple of it")
            .build());
    add(SelectionProperty("Statistic", List<IString>(statisticNames[0], statisticNames[1], statisticNames[2], statisticNames[3], statisticNames[4]), 1));
    blockSize = objPtr.getPropertyValue("BlockSize");
    statistic = static_cast<Statistic>(static_cast<Int>(objPtr.getPropertyValue("Statistic")));

    setInputs(List<IComponent>());  // nothing to read yet
    params.setValueReadType(SampleType::Float64);
    params.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);  // the other inputs go on while one is in error
    createReader();
    addFreePort("Input");
}

FunctionBlockTypePtr BlockStatisticsReaderFbImpl::CreateType(const ModuleInfoPtr& moduleInfo)
{
    auto fbType = FunctionBlockType("RefFBModuleBlockStatisticsReader",
                                    "Block statistics with reader",
                                    "Calculates a statistic per block of every connected equal-rate signal using multi reader");
    checkErrorInfo(fbType.asPtr<IComponentTypePrivate>(true)->setModuleInfo(moduleInfo));
    return fbType;
}

void BlockStatisticsReaderFbImpl::propertyChanged()
{
    auto lock = getRecursiveConfigLock2();
    blockSize = objPtr.getPropertyValue("BlockSize");
    statistic = static_cast<Statistic>(static_cast<Int>(objPtr.getPropertyValue("Statistic")));
    filled = 0;
    rebuildOutputDescriptor();
}

bool BlockStatisticsReaderFbImpl::accepts(const ComponentPtr&, const DataDescriptorPtr& descriptor, const ListPtr<IDataDescriptor>&)
{
    return scalarNumeric(descriptor);
}

// Drops the partial block on a resynchronization, a new domain or a contributing input's new descriptor
void BlockStatisticsReaderFbImpl::onChanges(const MultiReader2StatusPtr& status)
{
    bool restart = status.getStatus() == ComponentStatus::Error || status.getResynchronized() || status.getDomainDescriptorChanged();
    for (const MultiReader2InputStatusPtr in : status.getInputs())
    {
        const auto id = in.getInput().getGlobalId().toStdString();
        restart = restart || (in.getDescriptorChanged() && cached.count(id));  // a rejoining input only yields NaN
        if (in.getError() != MultiReader2InputError::None || in.getDescriptorChanged())
            accumulators[id].spoiled = true;
    }
    ReaderFbBase::onChanges(status);
    if (restart)
        filled = 0;

    MultiReader2StatusPtr next;
    for (const auto& port : removeDisconnectedPorts(status, next))
    {
        const auto id = port.getGlobalId().toStdString();
        if (const auto it = outputs.find(id); it != outputs.end())
        {
            removeSignal(it->second);
            outputs.erase(it);
        }
        accumulators.erase(id);
    }
    if (next.assigned())
        onChanges(next);
}

SignalConfigPtr BlockStatisticsReaderFbImpl::outputFor(const ComponentPtr& input)
{
    const auto id = input.getGlobalId().toStdString();
    if (const auto it = outputs.find(id); it != outputs.end())
        return it->second;

    // Input_3 publishes Statistic_3
    const std::string portId = input.getLocalId();
    const auto signal = createAndAddSignal("Statistic" + portId.substr(portId.find('_')));
    signal.setDomainSignal(domainSignal);
    outputs[id] = signal;
    return signal;
}

DataDescriptorPtr BlockStatisticsReaderFbImpl::statisticDescriptor(const DataDescriptorPtr& input) const
{
    auto builder = DataDescriptorBuilder().setSampleType(SampleType::Float64).setName(statisticNames[static_cast<Int>(statistic)]);
    if (statistic == Statistic::CrestFactor)
        return builder.setValueRange(Range(1, std::sqrt(static_cast<double>(blockSize)))).build();  // peak over RMS of N samples is at most sqrt(N)

    builder.setUnit(input.getUnit());
    const auto range = input.getValueRange();
    if (!range.assigned() || input.getPostScaling().assigned())
        return builder.build();
    const double low = range.getLowValue();
    const double high = range.getHighValue();
    switch (statistic)
    {
        case Statistic::Mean:
            builder.setValueRange(Range(low, high));
            break;
        case Statistic::Rms:
            builder.setValueRange(Range(0, std::max(std::fabs(low), std::fabs(high))));
            break;
        case Statistic::AcRms:
            builder.setValueRange(Range(0, (high - low) / 2));
            break;
        default:
            builder.setValueRange(Range(0, high - low));
    }
    return builder.build();
}

// The output domain is the main input's with BlockSize times its delta; origin, start, resolution and reference domain
// stay, so a value's tick is its block's first sample's
void BlockStatisticsReaderFbImpl::rebuildOutputDescriptor()
{
    if (outputDomain.assigned())
    {
        const auto rule = outputDomain.getRule().getParameters();
        domainDelta = rule.get("delta");
        domainStart = rule.get("start");
        const auto descriptor = DataDescriptorBuilderCopy(outputDomain)
                                    .setName("StatisticsDomain")
                                    .setRule(LinearDataRule(domainDelta * blockSize, domainStart))
                                    .build();
        if (domainSignal.getDescriptor() != descriptor)
            domainSignal.setDescriptor(descriptor);
    }
    for (const auto& input : params.getInputs())
    {
        const auto it = cached.find(input.getGlobalId().toStdString());
        if (it == cached.end())
            continue;
        const auto signal = outputFor(input);
        const auto descriptor = statisticDescriptor(it->second);
        if (signal.getDescriptor() != descriptor)
            signal.setDescriptor(descriptor);
    }
}

void BlockStatisticsReaderFbImpl::processAndSend(SizeT count, SizeT packetOffset)
{
    if (!outputDomain.assigned())
        return;
    const Int delta = domainDelta;
    Int start = domainStart;
    if (const auto reference = outputDomain.getReferenceDomainInfo();
        reference.assigned() && reference.getReferenceDomainOffset().assigned())
        start += static_cast<Int>(reference.getReferenceDomainOffset());  // blocks align on the reference domain
    const auto inputs = params.getInputs();
    const auto size = static_cast<SizeT>(blockSize);
    std::vector<Accumulator*> slots;  // looked up once per read, not once per block
    for (const auto& input : inputs)
        slots.push_back(&accumulators[input.getGlobalId().toStdString()]);

    std::vector<std::vector<double>> values(inputs.getCount());
    SizeT blocks = 0;
    SizeT firstOffset = 0;
    for (SizeT j = 0; j < count;)
    {
        if (filled == 0)
        {
            // Between blocks: skip to the next sample whose index from the origin is a multiple of the block size
            const Int index = floorDiv(static_cast<Int>(packetOffset + j * delta) + start, delta);
            const auto skip = static_cast<SizeT>(floorMod(-index, blockSize));
            if (skip >= count - j)
                break;
            j += skip;
            blockOffset = packetOffset + j * delta;
            for (auto& [id, accumulator] : accumulators)
                accumulator = Accumulator();
        }

        const SizeT len = std::min(count - j, size - filled);
        for (SizeT i = 0; i < inputs.getCount(); ++i)
        {
            if (buffers[i])
                slots[i]->add(static_cast<const double*>(buffers[i]) + j, len);
            else
                slots[i]->spoiled = true;
        }
        filled += len;
        j += len;
        if (filled < size)
            break;

        if (blocks++ == 0)
            firstOffset = blockOffset;
        for (SizeT i = 0; i < inputs.getCount(); ++i)
            values[i].push_back(slots[i]->spoiled || slots[i]->n != size ? std::numeric_limits<double>::quiet_NaN()
                                                                         : slots[i]->value(statistic));
        filled = 0;
    }
    if (blocks == 0)
        return;

    const auto domainPacket = DataPacket(domainSignal.getDescriptor(), blocks, static_cast<Int>(firstOffset));
    domainSignal.sendPacket(domainPacket);
    for (SizeT i = 0; i < inputs.getCount(); ++i)
    {
        const auto it = outputs.find(inputs[i].getGlobalId().toStdString());
        if (it == outputs.end() || !it->second.getDescriptor().assigned())
            continue;
        const auto valuePacket = DataPacketWithDomain(domainPacket, it->second.getDescriptor(), blocks);
        std::copy(values[i].begin(), values[i].end(), static_cast<double*>(valuePacket.getRawData()));
        it->second.sendPacket(valuePacket);
    }
}

}

END_NAMESPACE_REF_FB_MODULE
