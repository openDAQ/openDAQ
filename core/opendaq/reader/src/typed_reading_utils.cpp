#include <opendaq/typed_reading_utils.h>

#include <coretypes/exceptions.h>
#include <opendaq/reader_utils.h>
#include <opendaq/sample_type.h>
#include <opendaq/sample_type_traits.h>

BEGIN_NAMESPACE_OPENDAQ

namespace
{

template <typename T>
struct TypeTag
{
    using Type = T;
};

template <typename Visitor>
decltype(auto) visitSampleType(SampleType sampleType, Visitor&& visitor)
{
    switch (sampleType)
    {
        case SampleType::Float32:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::Float32>::Type>{});
        case SampleType::Float64:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::Float64>::Type>{});
        case SampleType::UInt8:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::UInt8>::Type>{});
        case SampleType::Int8:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::Int8>::Type>{});
        case SampleType::UInt16:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::UInt16>::Type>{});
        case SampleType::Int16:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::Int16>::Type>{});
        case SampleType::UInt32:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::UInt32>::Type>{});
        case SampleType::Int32:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::Int32>::Type>{});
        case SampleType::UInt64:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::UInt64>::Type>{});
        case SampleType::Int64:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::Int64>::Type>{});
        case SampleType::RangeInt64:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::RangeInt64>::Type>{});
        case SampleType::ComplexFloat32:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::ComplexFloat32>::Type>{});
        case SampleType::ComplexFloat64:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::ComplexFloat64>::Type>{});
        case SampleType::Struct:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::Struct>::Type>{});
        case SampleType::Undefined:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::Struct>::Type>{});
        case SampleType::Binary:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::Struct>::Type>{});
        case SampleType::String:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::Struct>::Type>{});
        case SampleType::Null:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::Struct>::Type>{});
        case SampleType::_count:
            return std::forward<Visitor>(visitor)(TypeTag<SampleTypeToType<SampleType::Struct>::Type>{});
    }
    DAQ_THROW_EXCEPTION(NotSupportedException, "The requested sample-type is unsupported or invalid.");
}

std::string_view format_as(SampleType sampleType);

bool validateInputType(SampleType sampleType, bool isDomain)
{
    switch (sampleType)
    {
        case SampleType::Float32:
        case SampleType::Float64:
        case SampleType::UInt8:
        case SampleType::Int8:
        case SampleType::UInt16:
        case SampleType::Int16:
        case SampleType::UInt32:
        case SampleType::Int32:
        case SampleType::UInt64:
        case SampleType::Int64:
            return true;
        case SampleType::RangeInt64:
            return true;  // TODO: Not sure what kind of support we actually have
        case SampleType::ComplexFloat32:
        case SampleType::ComplexFloat64:
        case SampleType::Struct:
        case SampleType::Undefined:
        case SampleType::Binary:
        case SampleType::String:
            if (isDomain)
                DAQ_THROW_EXCEPTION(NotSupportedException, "Using the SampleType {} as a domain is not supported", format_as(sampleType));
            return true;
        case SampleType::Null:
            DAQ_THROW_EXCEPTION(NotSupportedException, "Null input sample type encountered.");
        case SampleType::_count:
        default:
            DAQ_THROW_EXCEPTION(InvalidStateException, "Unexpected input sample type encountered.");
    }
}

bool validateOutputType(SampleType sampleType, bool isDomain)
{
    switch (sampleType)
    {
        case SampleType::Float32:
        case SampleType::Float64:
        case SampleType::UInt8:
        case SampleType::Int8:
        case SampleType::UInt16:
        case SampleType::Int16:
        case SampleType::UInt32:
        case SampleType::Int32:
        case SampleType::UInt64:
        case SampleType::Int64:
        case SampleType::RangeInt64:
            return true;
        case SampleType::ComplexFloat32:
        case SampleType::ComplexFloat64:
        case SampleType::Struct:
        case SampleType::Undefined:
            return !isDomain;
        case SampleType::Binary:
        case SampleType::String:
        case SampleType::Null:
        case SampleType::_count:
        default:
            DAQ_THROW_EXCEPTION(NotSupportedException, "The requested output sample type is unsupported or invalid.");
    }
}

bool validateInputOutputType(SampleType inputType, SampleType outputType, bool isDomain)
{
    // TODO: Implement combination checking (float -> int for domains etc.)
    return true;
}

template <typename Visitor>
decltype(auto) visitTwoSampleTypes(SampleType inputType, SampleType outputType, bool isDomain, Visitor&& visitor)
{
    if (!validateInputType(inputType, isDomain))
        DAQ_THROW_EXCEPTION(NotSupportedException, "Input sample type not supported.");

    if (!validateOutputType(outputType, isDomain))
        DAQ_THROW_EXCEPTION(NotSupportedException, "Output sample type not supported.");

    if (!validateInputOutputType(inputType, outputType, isDomain))
        DAQ_THROW_EXCEPTION(NotSupportedException, "Output sample type not supported.");

    return visitSampleType(inputType,
                           [&](auto inputTag) -> decltype(auto)
                           {
                               return visitSampleType(outputType,
                                                      [&](auto outputTag) -> decltype(auto)
                                                      { return std::forward<Visitor>(visitor)(inputTag, outputTag); });
                           });
}

std::string_view format_as(SampleType sampleType)
{
    switch (sampleType)
    {
        case SampleType::Float32:
            return "Float32";
        case SampleType::Float64:
            return "Float64";
        case SampleType::UInt8:
            return "UInt8";
        case SampleType::Int8:
            return "Int8";
        case SampleType::UInt16:
            return "UInt16";
        case SampleType::Int16:
            return "Int16";
        case SampleType::UInt32:
            return "UInt32";
        case SampleType::Int32:
            return "Int32";
        case SampleType::UInt64:
            return "UInt64";
        case SampleType::Int64:
            return "Int64";
        case SampleType::RangeInt64:
            return "RangeInt64";
        case SampleType::ComplexFloat32:
            return "ComplexFloat32";
        case SampleType::ComplexFloat64:
            return "ComplexFloat64";
        case SampleType::Struct:
            return "Struct";
        case SampleType::Undefined:
            return "Undefined";
        case SampleType::Binary:
            return "Binary";
        case SampleType::String:
            return "String";
        case SampleType::Null:
            return "Null";
        case SampleType::_count:
            return "Count";
    }
    return "Unknown";
}

}

namespace detail
{

template <typename InputT, typename OutputT>
bool isSampleTypeConvertible(bool isDomain)
{
    if constexpr (std::is_same_v<OutputT, void*>)
    {
        return !isDomain;
    }
    else
    {
        return std::is_convertible_v<InputT, OutputT>;
    }
}

template <typename InputT, typename OutputT>
ErrCode readData(const ReadLayout& readLayout,
                 void* inputBuffer,
                 SizeT offset,
                 void** outputBuffer,
                 SizeT toRead,
                 const FunctionPtr& transformFunction = nullptr)
{
    OPENDAQ_PARAM_NOT_NULL(inputBuffer);
    OPENDAQ_PARAM_NOT_NULL(outputBuffer);
    OPENDAQ_PARAM_NOT_NULL(readLayout.descriptor.getObject());

    const auto& rawSampleSize = readLayout.rawSampleSize;
    const auto& valuesPerSample = readLayout.valuesPerSample;
    const auto& dataDescriptor = readLayout.descriptor;

    if constexpr (std::is_same_v<OutputT, void*>)
    {
        if (transformFunction.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "Transform function for void reader not supported.");

        const auto dataStart = static_cast<void*>(static_cast<uint8_t*>(inputBuffer) + offset * rawSampleSize);
        const auto toReadInBytes = rawSampleSize * toRead;
        const auto dataOut = *outputBuffer;
        std::memcpy(dataOut, dataStart, toReadInBytes);
        *outputBuffer = static_cast<void*>(static_cast<uint8_t*>(dataOut) + toReadInBytes);
        return OPENDAQ_SUCCESS;
    }
    else if constexpr (std::is_convertible_v<InputT, OutputT>)
    {
        auto* dataStart = static_cast<InputT*>(inputBuffer) + (offset * valuesPerSample);
        auto* dataOut = static_cast<OutputT*>(*outputBuffer);

        if (transformFunction.assigned())
        {
            transformFunction.call((Int) dataStart, (Int) dataOut, toRead, dataDescriptor);

            *outputBuffer = dataOut + (valuesPerSample * toRead);
            return OPENDAQ_SUCCESS;
        }

        // If the type of samples is the same, then just copy
        if constexpr (std::is_same_v<OutputT, InputT>)
        {
            // Returns the pointer to the value after the last copied one
            *outputBuffer = std::copy_n(dataStart, valuesPerSample * toRead, dataOut);  // C4244 - possible data loss due to conversion
        }
        else
        {
            for (std::size_t i = 0; i < toRead * valuesPerSample; ++i)
            {
                dataOut[i] = static_cast<OutputT>(dataStart[i]);  // C4244 - possible data loss due to conversion
            }

            // Set the pointer to the value after the last copied one
            *outputBuffer = &dataOut[toRead];
        }

        return OPENDAQ_SUCCESS;
    }
    else
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED,
                                   "Implicit conversion from packet data-type to the read data-type is not supported.");
    }
}

template <typename InputT, typename OutputT>
void getDeltaStart(const daq::DictPtr<daq::IString, daq::IBaseObject>& params, OutputT& delta, OutputT& start)
{
    if constexpr (std::is_same_v<OutputT, void*>)
    {
        DAQ_THROW_EXCEPTION(NotSupportedException, "Void reader should not be used for domain.");
    }
    if constexpr (!std::is_convertible_v<InputT, OutputT>)
    {
        DAQ_THROW_EXCEPTION(NotSupportedException, "Implicit conversion from packet data-type to the read data-type is not supported.");
    }
    else
    {
        delta = static_cast<OutputT>(params.get("delta"));
        start = static_cast<OutputT>(params.get("start"));
    }
}

template <typename InputT, typename OutputT>
std::unique_ptr<DomainValue> readDomainValueLinear(const DataPacketPtr& domainPacket, SizeT index, const DomainInfo& domainInfo)
{
    if constexpr (std::is_same_v<void*, OutputT> || !std::is_integral_v<OutputT>)
    {
        DAQ_THROW_EXCEPTION(NotSupportedException,
                            "ReadDomainValueLinear not supported for the selected output type (void / non-integral).");
        return {};
    }
    else
    {
        const DataRulePtr dataRule = domainPacket.getDataDescriptor().getRule();
        NumberPtr packetOffset = domainPacket.getOffset();
        const auto parameters = dataRule.getParameters();
        OutputT delta, start;
        getDeltaStart<InputT, OutputT>(parameters, delta, start);

        int64_t rdOffset = 0;
        auto refDomainInfo = domainPacket.getDataDescriptor().getReferenceDomainInfo();
        if (refDomainInfo.assigned())
        {
            const IntPtr referenceDomainOffset = refDomainInfo.getReferenceDomainOffset();
            if (referenceDomainOffset.assigned())
            {
                rdOffset = referenceDomainOffset;
            }
        }

        OutputT timestamp =
            start + static_cast<OutputT>(rdOffset) + static_cast<OutputT>(packetOffset.getIntValue()) + delta * static_cast<OutputT>(index);
        return std::make_unique<DomainValueImpl<OutputT>>(domainInfo, timestamp);
    }
}

template <typename InputT, typename OutputT>
std::unique_ptr<DomainValue> readDomainValue(const ReadLayout& readLayout,
                                             const DataPacketPtr& domainPacket,
                                             SizeT index,
                                             const DomainInfo& domainInfo)
{
    if constexpr (std::is_same_v<void*, OutputT>)
    {
        DAQ_THROW_EXCEPTION(NotSupportedException, "ReadDomainValueLinear not supported for the void output type.");
        return {};
    }
    else
    {
        auto descriptor = domainPacket.getDataDescriptor();
        if (!descriptor.assigned())
            DAQ_THROW_EXCEPTION(InvalidStateException, "Packet should have descriptor assigned.");

        OutputT timestamp{};
        void* data = &timestamp;
        readData<InputT, OutputT>(readLayout, domainPacket.getData(), index, &data, 1, nullptr);
        return std::make_unique<DomainValueImpl<OutputT>>(domainInfo, timestamp);
    }
}

template <typename InputT, typename OutputT>
SizeT findDomainValueLinear(const DataPacketPtr& domainPacket,
                            const DomainValue* target,
                            [[maybe_unused]] std::chrono::system_clock::rep* absoluteTimestamp)
{
    if constexpr (!std::is_same_v<void*, OutputT> && std::is_integral_v<OutputT> && std::is_convertible_v<InputT, OutputT>)
    {
        const SizeT sampleCount = domainPacket.getSampleCount();
        if (sampleCount == 0)
        {
            return static_cast<SizeT>(-1);
        }

        const DataRulePtr& dataRule = domainPacket.getDataDescriptor().getRule();
        const auto parameters = dataRule.getParameters();

        int64_t rdOffset = 0;
        {  // Extract reference domain offset
            auto refDomainInfo = domainPacket.getDataDescriptor().getReferenceDomainInfo();
            if (refDomainInfo.assigned())
            {
                const IntPtr referenceDomainOffset = refDomainInfo.getReferenceDomainOffset();
                if (referenceDomainOffset.assigned())
                {
                    rdOffset = referenceDomainOffset;
                }
            }
        }

        OutputT ruleDelta, ruleStart;
        getDeltaStart<InputT, OutputT>(parameters, ruleDelta, ruleStart);
        NumberPtr packetOffset = domainPacket.getOffset();

        // Total packet offset in signal resolution ticks.
        const OutputT startTick = ruleStart + static_cast<OutputT>(rdOffset) + static_cast<OutputT>(packetOffset.getIntValue());
        const OutputT previousEndTick = startTick - ruleDelta;

        // Tick of the last sample in signal resolution ticks
        const SizeT packetSize = domainPacket.getSampleCount();
        OutputT endTick = startTick + ruleDelta * static_cast<OutputT>(packetSize - 1);

        const DomainValueImpl<OutputT>* typedTarget = dynamic_cast<const DomainValueImpl<OutputT>*>(target);
        OutputT targetValue = typedTarget->getValue();

        /*
        The function returns the index k of the first tick where the tick_k >= target
        (in max resolution ticks since min epoch). The k-th sample is the answer for any target inside
        interval (tick_{k-1}, tick_k]. The n-th packet then covers ticks (endTick_{n-1}, endTick].
        Using the linear rule, the last sample in the previous packet was at endTick_{n-1} = startTick_n - ruleDelta.

        Example: Packet with ticks [12, 15, 18] will return index 0 for targets [10, 11, 12] and -1 for <=9 and >=19.
        */
        if (targetValue <= previousEndTick || targetValue > endTick)
        {
            // Target is outside this packet
            return static_cast<SizeT>(-1);
        }

        // Ticks from the end of the previous packet's end.
        SizeT ticksToTarget = targetValue - previousEndTick;
        // Minus one due to calculation from the previous packet end tick.
        SizeT index = static_cast<SizeT>((ticksToTarget + ruleDelta - 1) / ruleDelta) - 1;

        assert(index < packetSize && "Index out of bounds");

        if (absoluteTimestamp)
        {
            if constexpr (IsTemplateOf<OutputT, daq::RangeType>::value || IsTemplateOf<OutputT, daq::Complex_Number>::value)
            {
                DAQ_THROW_EXCEPTION(NotSupportedException);
            }
            else
            {
                // Tick corresponding to index in signal's resolution ticks.
                OutputT tick = startTick + static_cast<OutputT>(index) * ruleDelta;
                auto readValueSysTime = reader::toSysTime(tick, target->getDomain().epoch, target->getDomain().resolution);
                *absoluteTimestamp = readValueSysTime.time_since_epoch().count();
            }
        }
        return index;
    }
    else
    {
        DAQ_THROW_EXCEPTION(NotSupportedException, "Implicit conversion from packet data-type to the read data-type is not supported.");
    }
}

template <typename InputT, typename OutputT>
SizeT findDomainValue(const ReadLayout& readLayout,
                      const DataPacketPtr& domainPacket,
                      const DomainValue* target,
                      [[maybe_unused]] std::chrono::system_clock::rep* absoluteTimestamp)
{
    void* inputBuffer = domainPacket.getData();
    SizeT size = domainPacket.getSampleCount();

    if (!inputBuffer)
        DAQ_THROW_EXCEPTION(ArgumentNullException, "Packet with null data buffer");

    if constexpr (std::is_convertible_v<InputT, OutputT> && !std::is_same_v<void*, OutputT> && !std::is_same_v<void*, InputT>)
    {
        InputT* domainBuffer = static_cast<InputT*>(inputBuffer);

        const DomainValueImpl<OutputT>* typedTarget = dynamic_cast<const DomainValueImpl<OutputT>*>(target);
        OutputT targetValue = typedTarget->getValue();

        for (std::size_t i = 0; i < size * readLayout.valuesPerSample; ++i)
        {
            OutputT value = static_cast<OutputT>(domainBuffer[i]);  // C4244 - possible data loss due to conversion

            bool greaterEqual = false;
            if constexpr (IsTemplateOf<OutputT, daq::RangeType>::value)
            {
                if (value.start >= targetValue.start)
                {
                    if (absoluteTimestamp)
                    {
                        auto readValueSysTime = reader::toSysTime(value.start, target->getDomain().epoch, target->getDomain().resolution);
                        *absoluteTimestamp = readValueSysTime.time_since_epoch().count();
                    }
                    greaterEqual = true;
                }
            }
            else if constexpr (!IsTemplateOf<OutputT, daq::Complex_Number>::value)
            {
                if (value >= targetValue)
                {
                    if (absoluteTimestamp)
                    {
                        auto readValueSysTime = reader::toSysTime(value, target->getDomain().epoch, target->getDomain().resolution);
                        *absoluteTimestamp = readValueSysTime.time_since_epoch().count();
                    }
                    greaterEqual = true;
                }
            }
            else
            {
                DAQ_THROW_EXCEPTION(NotSupportedException);
            }

            if (greaterEqual)
            {
                return i / readLayout.valuesPerSample;
            }
        }

        return static_cast<SizeT>(-1);
    }
    else
    {
        DAQ_THROW_EXCEPTION(NotSupportedException, "Implicit conversion from packet data-type to the read data-type is not supported.");
    }
}

}

ReadLayout TypedReadingUtils::createReadLayout(const DataDescriptorPtr& descriptor)
{
    if (!descriptor.assigned())
        DAQ_THROW_EXCEPTION(ArgumentNullException, "Descriptor must be assigned!");

    const SizeT rawSampleSize = descriptor.getRawSampleSize();
    SizeT valuesPerSample = 1;
    auto dimensions = descriptor.getDimensions();
    if (dimensions.assigned() && dimensions.getCount() == 1)
    {
        valuesPerSample = dimensions[0].getSize();
    }

    return {descriptor, rawSampleSize, valuesPerSample};
}

bool TypedReadingUtils::isSampleTypeConvertible(SampleType in, SampleType out, bool isDomain)
{
    // TODO: Detais about limiting allowed types (not throwing unless necessary)
    switch (in)
    {
        case SampleType::Struct:
        case SampleType::Invalid:
        case SampleType::Null:
        case SampleType::_count:
            return false;
        default:
            break;
    }

    if (isDomain)
    {
        switch (in)
        {
            case SampleType::Float32:
                return in == SampleType::Float32 || in == SampleType::Float64;
            case SampleType::Float64:
                return in == SampleType::Float32 || in == SampleType::Float64;
            case SampleType::ComplexFloat32:
            case SampleType::ComplexFloat64:
            case SampleType::Binary:
                return false;
            default:
                break;
        }
    }

    return visitTwoSampleTypes(in,
                               out,
                               isDomain,
                               [&](auto inputTag, auto outputTag) -> bool
                               {
                                   using InputT = typename decltype(inputTag)::Type;
                                   using OutputT = typename decltype(outputTag)::Type;
                                   return detail::isSampleTypeConvertible<InputT, OutputT>(isDomain);
                               });
}

std::unique_ptr<DomainValue> TypedReadingUtils::readDomainValue(SampleType in,
                                                                SampleType out,
                                                                const ReadLayout& readLayout,
                                                                const DataPacketPtr& domainPacket,
                                                                SizeT index,
                                                                const DomainInfo& domainInfo)
{
    const DataRulePtr dataRule = domainPacket.getDataDescriptor().getRule();
    if (dataRule.getType() == DataRuleType::Linear)
    {
        return visitTwoSampleTypes(in,
                                   out,
                                   true,
                                   [&](auto inputTag, auto outputTag) -> std::unique_ptr<DomainValue>
                                   {
                                       using InputT = typename decltype(inputTag)::Type;
                                       using OutputT = typename decltype(outputTag)::Type;
                                       return detail::readDomainValueLinear<InputT, OutputT>(domainPacket, index, domainInfo);
                                   });
    }
    else
    {
        return visitTwoSampleTypes(in,
                                   out,
                                   true,
                                   [&](auto inputTag, auto outputTag) -> std::unique_ptr<DomainValue>
                                   {
                                       using InputT = typename decltype(inputTag)::Type;
                                       using OutputT = typename decltype(outputTag)::Type;
                                       return detail::readDomainValue<InputT, OutputT>(readLayout, domainPacket, index, domainInfo);
                                   });
    }
}

SizeT TypedReadingUtils::findDomainValue(SampleType in,
                                         SampleType out,
                                         const ReadLayout& readLayout,
                                         const DataPacketPtr& domainPacket,
                                         const DomainValue* target,
                                         std::chrono::system_clock::rep* firstSampleAbsoluteTime)
{
    const DataRulePtr dataRule = domainPacket.getDataDescriptor().getRule();
    if (dataRule.getType() == DataRuleType::Linear)
    {
        return visitTwoSampleTypes(in,
                                   out,
                                   true,
                                   [&](auto inputTag, auto outputTag) -> SizeT
                                   {
                                       using InputT = typename decltype(inputTag)::Type;
                                       using OutputT = typename decltype(outputTag)::Type;
                                       return detail::findDomainValueLinear<InputT, OutputT>(domainPacket, target, firstSampleAbsoluteTime);
                                   });
    }
    else
    {
        return visitTwoSampleTypes(in,
                                   out,
                                   true,
                                   [&](auto inputTag, auto outputTag) -> SizeT
                                   {
                                       using InputT = typename decltype(inputTag)::Type;
                                       using OutputT = typename decltype(outputTag)::Type;
                                       return detail::findDomainValue<InputT, OutputT>(
                                           readLayout, domainPacket, target, firstSampleAbsoluteTime);
                                   });
    }
}

ErrCode TypedReadingUtils::readData(SampleType in,
                                    SampleType out,
                                    bool isDomain,
                                    const ReadLayout& readLayout,
                                    void* inputBuffer,
                                    SizeT offset,
                                    void** outputBuffer,
                                    SizeT count,
                                    const FunctionPtr transform)
{
    return visitTwoSampleTypes(in,
                               out,
                               isDomain,
                               [&](auto inputTag, auto outputTag) -> ErrCode
                               {
                                   using InputT = typename decltype(inputTag)::Type;
                                   using OutputT = typename decltype(outputTag)::Type;
                                   return detail::readData<InputT, OutputT>(
                                       readLayout, inputBuffer, offset, outputBuffer, count, transform);
                               });
}

END_NAMESPACE_OPENDAQ