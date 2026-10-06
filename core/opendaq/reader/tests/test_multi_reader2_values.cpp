/*
 * MultiReader2 value rules: which value descriptors convert to which read types, the conversion itself for every
 * numeric pair, dimensioned and post-scaled values, and the Undefined read type.
 */
#include "test_multi_reader2_common.h"

#include <opendaq/scaling_factory.h>

using namespace daq;
using namespace testing;

namespace multi_reader2_values_test
{
struct Conversion
{
    SampleType source;
    SampleType read;
};

inline std::ostream& operator<<(std::ostream& os, const Conversion& c)
{
    return os << static_cast<int>(c.source) << "to" << static_cast<int>(c.read);
}

inline const char* typeName(SampleType type)
{
    switch (type)
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
        case SampleType::Binary:
            return "Binary";
        case SampleType::String:
            return "String";
        default:
            return "Other";
    }
}
}

using multi_reader2_values_test::Conversion;

class MultiReader2ValuesTest : public MultiReader2Test
{
public:
    // Writes the values 0 .. count-1 into a packet of the signal's own sample type and sends it at `offset`
    static void sendTyped(const ReadSignal2& read, Int offset, Int count)
    {
        switch (read.signal.getDescriptor().getSampleType())
        {
            case SampleType::Float32:
                return read.sendAt<float>(offset, count);
            case SampleType::Float64:
                return read.sendAt<double>(offset, count);
            case SampleType::UInt8:
                return read.sendAt<uint8_t>(offset, count);
            case SampleType::Int8:
                return read.sendAt<int8_t>(offset, count);
            case SampleType::UInt16:
                return read.sendAt<uint16_t>(offset, count);
            case SampleType::Int16:
                return read.sendAt<int16_t>(offset, count);
            case SampleType::UInt32:
                return read.sendAt<uint32_t>(offset, count);
            case SampleType::Int32:
                return read.sendAt<int32_t>(offset, count);
            case SampleType::UInt64:
                return read.sendAt<uint64_t>(offset, count);
            case SampleType::Int64:
                return read.sendAt<int64_t>(offset, count);
            default:
                FAIL() << "not a numeric type";
        }
    }

    // Reads `count` samples of the read type from one buffer as doubles
    static std::vector<double> asDoubles(const void* buffer, SampleType type, SizeT count)
    {
        std::vector<double> out(count);
        for (SizeT i = 0; i < count; i++)
        {
            switch (type)
            {
                case SampleType::Float32:
                    out[i] = static_cast<const float*>(buffer)[i];
                    break;
                case SampleType::Float64:
                    out[i] = static_cast<const double*>(buffer)[i];
                    break;
                case SampleType::UInt8:
                    out[i] = static_cast<const uint8_t*>(buffer)[i];
                    break;
                case SampleType::Int8:
                    out[i] = static_cast<const int8_t*>(buffer)[i];
                    break;
                case SampleType::UInt16:
                    out[i] = static_cast<const uint16_t*>(buffer)[i];
                    break;
                case SampleType::Int16:
                    out[i] = static_cast<const int16_t*>(buffer)[i];
                    break;
                case SampleType::UInt32:
                    out[i] = static_cast<const uint32_t*>(buffer)[i];
                    break;
                case SampleType::Int32:
                    out[i] = static_cast<const int32_t*>(buffer)[i];
                    break;
                case SampleType::UInt64:
                    out[i] = static_cast<double>(static_cast<const uint64_t*>(buffer)[i]);
                    break;
                case SampleType::Int64:
                    out[i] = static_cast<double>(static_cast<const int64_t*>(buffer)[i]);
                    break;
                default:
                    break;
            }
        }
        return out;
    }

    DataDescriptorPtr valueDescriptor(SampleType type)
    {
        return DataDescriptorBuilder().setSampleType(type).build();
    }
};

class MultiReader2ConversionTest : public MultiReader2ValuesTest, public WithParamInterface<Conversion>
{
};

// Two inputs of the source type read as the read type: the values 0 .. 7 survive every pairing
TEST_P(MultiReader2ConversionTest, ConvertsValues)
{
    const Conversion c = GetParam();
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 8, domain, c.source);
    addSignal(0, 8, domain, c.source);
    auto reader = createReaderProbed(params(signalsToList(), c.read));
    sendTyped(readSignals[0], 0, 8);
    sendTyped(readSignals[1], 0, 8);
    scheduler.waitAll();
    ASSERT_EQ(reader.getAvailableCount(), 8u);

    std::vector<uint8_t> a(9 * 8, 0xAA), b(9 * 8, 0xAA);
    void* buffers[2]{a.data(), b.data()};
    SizeT count = 8;
    SizeT offset = 0;
    auto status = reader.read(buffers, &count, offset);
    ASSERT_EQ(count, 8u);
    ASSERT_THAT(asDoubles(a.data(), c.read, 8), ElementsAre(0, 1, 2, 3, 4, 5, 6, 7));
    ASSERT_THAT(asDoubles(b.data(), c.read, 8), ElementsAre(0, 1, 2, 3, 4, 5, 6, 7));
    // Nothing is written past the samples read
    ASSERT_EQ(a[8 * getSampleSize(c.read)], 0xAA);
}

static std::vector<Conversion> conversionMatrix()
{
    const SampleType sources[]{SampleType::Float32,
                               SampleType::Float64,
                               SampleType::UInt8,
                               SampleType::Int8,
                               SampleType::UInt16,
                               SampleType::Int16,
                               SampleType::UInt32,
                               SampleType::Int32,
                               SampleType::UInt64,
                               SampleType::Int64};
    const SampleType reads[]{SampleType::Float64, SampleType::Float32, SampleType::Int32, SampleType::UInt8};
    std::vector<Conversion> out;
    for (const auto source : sources)
        for (const auto read : reads)
            out.push_back({source, read});
    return out;
}

INSTANTIATE_TEST_SUITE_P(Matrix,
                         MultiReader2ConversionTest,
                         ValuesIn(conversionMatrix()),
                         [](const TestParamInfo<Conversion>& info)
                         {
                             return std::string(multi_reader2_values_test::typeName(info.param.source)) + "To" +
                                    multi_reader2_values_test::typeName(info.param.read);
                         });

class MultiReader2NonNumericTest : public MultiReader2ValuesTest, public WithParamInterface<SampleType>
{
};

// A non-numeric value type is an error for a numeric read type and accepted by Undefined
TEST_P(MultiReader2NonNumericTest, InvalidForNumericReadAcceptedByUndefined)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 4, domain, SampleType::Float64);
    auto odd = Signal(context, nullptr, "odd");
    odd.setDescriptor(valueDescriptor(GetParam()));
    odd.setDomainSignal(domain);
    readSignals.emplace_back(odd, 0, 4);

    auto p = params(signalsToList(), SampleType::Float64);
    auto reader = createReader(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::ValueDescriptorInvalid);

    p.setValueReadType(SampleType::Undefined);
    reader.configure(p);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::None);
    ASSERT_EQ(input(status, 1).getDescriptor().getSampleType(), GetParam());

    // Rejected by the owner under Exclude, it is set aside like any erroring input
    p.setValueReadType(SampleType::Int16);
    p.setErrorPolicy(MultiReader2ErrorPolicy::Exclude);
    p.setAcceptsDescriptor(rejecting({odd}));
    reader.configure(p);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::ValueDescriptorInvalid);
}

INSTANTIATE_TEST_SUITE_P(
    Types,
    MultiReader2NonNumericTest,
    Values(SampleType::RangeInt64, SampleType::ComplexFloat32, SampleType::ComplexFloat64, SampleType::Binary, SampleType::String),
    [](const TestParamInfo<SampleType>& info) { return std::string(multi_reader2_values_test::typeName(info.param)); });

// ---------------------------------------------------------------- descriptors

TEST_F(MultiReader2ValuesTest, SignalWithoutADescriptorIsValueInvalidUntilItGetsOne)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    auto bare = Signal(context, nullptr, "bare");
    bare.setDomainSignal(domain);
    readSignals.emplace_back(bare, 0, 10);
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::ValueDescriptorInvalid);

    bare.setDescriptor(setupDescriptor(SampleType::Float64));
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::None);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2ValuesTest, UndefinedRejectsASignalWithoutADescriptor)
{
    readSignals.reserve(1);
    auto domain = createDomainSignal();
    auto bare = Signal(context, nullptr, "bare");
    bare.setDomainSignal(domain);
    readSignals.emplace_back(bare, 0, 10);
    auto reader = createReader(params(signalsToList(), SampleType::Undefined));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::ValueDescriptorInvalid);
}

TEST_F(MultiReader2ValuesTest, MixedSourceTypesReadAsOneType)
{
    readSignals.reserve(4);
    auto domain = createDomainSignal();
    addSignal(0, 5, domain, SampleType::UInt8);
    addSignal(0, 5, domain, SampleType::Int64);
    addSignal(0, 5, domain, SampleType::Float32);
    addSignal(0, 5, domain, SampleType::Int16);
    auto reader = createReaderProbed(params(signalsToList(), SampleType::Float64));
    for (const auto& read : readSignals)
        sendTyped(read, 0, 5);
    scheduler.waitAll();
    std::array<std::array<double, 5>, 4> values{};
    SizeT count = 5;
    read(reader, values, count);
    ASSERT_EQ(count, 5u);
    for (SizeT i = 0; i < 4; i++)
        ASSERT_THAT(values[i], ElementsAre(0, 1, 2, 3, 4)) << i;
}

TEST_F(MultiReader2ValuesTest, UndefinedKeepsEveryInputsOwnType)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    addSignal(0, 4, domain, SampleType::UInt8);
    addSignal(0, 4, domain, SampleType::Int64);
    addSignal(0, 4, domain, SampleType::Float32);
    auto reader = createReaderProbed(params(signalsToList(), SampleType::Undefined));
    for (const auto& read : readSignals)
        sendTyped(read, 0, 4);
    scheduler.waitAll();
    std::array<uint8_t, 4> a{};
    std::array<int64_t, 4> b{};
    std::array<float, 4> c{};
    void* buffers[3]{a.data(), b.data(), c.data()};
    SizeT count = 4;
    SizeT offset = 0;
    reader.read(buffers, &count, offset);
    ASSERT_EQ(count, 4u);
    ASSERT_THAT(a, ElementsAre(0, 1, 2, 3));
    ASSERT_THAT(b, ElementsAre(0, 1, 2, 3));
    ASSERT_THAT(c, ElementsAre(0.0f, 1.0f, 2.0f, 3.0f));
}

TEST_F(MultiReader2ValuesTest, UndefinedCopiesComplexValuesRaw)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 3, domain, SampleType::Float64);
    auto complexSignal = Signal(context, nullptr, "complex");
    complexSignal.setDescriptor(valueDescriptor(SampleType::ComplexFloat64));
    complexSignal.setDomainSignal(domain);
    readSignals.emplace_back(complexSignal, 0, 3);
    auto reader = createReaderProbed(params(signalsToList(), SampleType::Undefined));
    readSignals[0].createAndSendPacket(0);
    {
        auto domainPacket = DataPacket(domain.getDescriptor(), 3, 0);
        auto packet = DataPacketWithDomain(domainPacket, complexSignal.getDescriptor(), 3);
        auto data = static_cast<double*>(packet.getRawData());
        for (int i = 0; i < 6; i++)
            data[i] = i * 0.5;
        complexSignal.sendPacket(packet);
    }
    scheduler.waitAll();
    std::array<double, 3> a{};
    std::array<double, 6> b{};
    void* buffers[2]{a.data(), b.data()};
    SizeT count = 3;
    SizeT offset = 0;
    reader.read(buffers, &count, offset);
    ASSERT_EQ(count, 3u);
    ASSERT_THAT(b, ElementsAre(0, 0.5, 1, 1.5, 2, 2.5));
}

TEST_F(MultiReader2ValuesTest, DimensionedValuesConvertEveryElement)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    auto vec = Signal(context, nullptr, "vec");
    vec.setDescriptor(DataDescriptorBuilder()
                          .setSampleType(SampleType::Int16)
                          .setDimensions(List<IDimension>(Dimension(LinearDimensionRule(1, 0, 3))))
                          .build());
    vec.setDomainSignal(domain);
    readSignals.emplace_back(vec, 0, 2);
    addSignal(0, 2, domain);
    auto reader = createReaderProbed(params(signalsToList(), SampleType::Int32));
    {
        auto domainPacket = DataPacket(domain.getDescriptor(), 2, 0);
        auto packet = DataPacketWithDomain(domainPacket, vec.getDescriptor(), 2);
        auto data = static_cast<int16_t*>(packet.getRawData());
        for (int i = 0; i < 6; i++)
            data[i] = static_cast<int16_t>(-i);
        vec.sendPacket(packet);
    }
    readSignals[1].createAndSendPacket(0);
    scheduler.waitAll();
    std::array<int32_t, 6> a{};
    std::array<int32_t, 2> b{};
    void* buffers[2]{a.data(), b.data()};
    SizeT count = 2;
    SizeT offset = 0;
    reader.read(buffers, &count, offset);
    ASSERT_EQ(count, 2u);
    ASSERT_THAT(a, ElementsAre(0, -1, -2, -3, -4, -5));
    ASSERT_THAT(b, ElementsAre(0, 1));
}

TEST_F(MultiReader2ValuesTest, DimensionedValuesCopyRawUnderUndefined)
{
    readSignals.reserve(1);
    auto domain = createDomainSignal();
    auto vec = Signal(context, nullptr, "vec");
    vec.setDescriptor(DataDescriptorBuilder()
                          .setSampleType(SampleType::Int16)
                          .setDimensions(List<IDimension>(Dimension(LinearDimensionRule(1, 0, 3))))
                          .build());
    vec.setDomainSignal(domain);
    readSignals.emplace_back(vec, 0, 2);
    auto reader = createReaderProbed(params(signalsToList(), SampleType::Undefined));
    {
        auto domainPacket = DataPacket(domain.getDescriptor(), 2, 0);
        auto packet = DataPacketWithDomain(domainPacket, vec.getDescriptor(), 2);
        auto data = static_cast<int16_t*>(packet.getRawData());
        for (int i = 0; i < 6; i++)
            data[i] = static_cast<int16_t>(10 * i);
        vec.sendPacket(packet);
    }
    scheduler.waitAll();
    std::array<int16_t, 6> a{};
    void* buffers[1]{a.data()};
    SizeT count = 2;
    SizeT offset = 0;
    reader.read(buffers, &count, offset);
    ASSERT_EQ(count, 2u);
    ASSERT_THAT(a, ElementsAre(0, 10, 20, 30, 40, 50));
}

TEST_F(MultiReader2ValuesTest, PostScaledValuesAreReadScaled)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    auto scaled = Signal(context, nullptr, "scaled");
    scaled.setDescriptor(DataDescriptorBuilder()
                             .setSampleType(SampleType::Float64)
                             .setPostScaling(LinearScaling(2, 1, SampleType::Int32, ScaledSampleType::Float64))
                             .build());
    scaled.setDomainSignal(domain);
    readSignals.emplace_back(scaled, 0, 4);
    addSignal(0, 4, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    {
        auto domainPacket = DataPacket(domain.getDescriptor(), 4, 0);
        auto packet = DataPacketWithDomain(domainPacket, scaled.getDescriptor(), 4);
        auto raw = static_cast<int32_t*>(packet.getRawData());
        for (int i = 0; i < 4; i++)
            raw[i] = i;
        scaled.sendPacket(packet);
    }
    readSignals[1].createAndSendPacket(0);
    scheduler.waitAll();
    std::array<std::array<double, 4>, 2> values{};
    SizeT count = 4;
    read(reader, values, count);
    ASSERT_EQ(count, 4u);
    ASSERT_THAT(values[0], ElementsAre(1, 3, 5, 7));
}

TEST_F(MultiReader2ValuesTest, PostScaledValuesReadAsIntegersTruncate)
{
    readSignals.reserve(1);
    auto domain = createDomainSignal();
    auto scaled = Signal(context, nullptr, "scaled");
    scaled.setDescriptor(DataDescriptorBuilder()
                             .setSampleType(SampleType::Float64)
                             .setPostScaling(LinearScaling(0.5, 0, SampleType::Int32, ScaledSampleType::Float64))
                             .build());
    scaled.setDomainSignal(domain);
    readSignals.emplace_back(scaled, 0, 4);
    auto reader = createReaderProbed(params(signalsToList(), SampleType::Int32));
    {
        auto domainPacket = DataPacket(domain.getDescriptor(), 4, 0);
        auto packet = DataPacketWithDomain(domainPacket, scaled.getDescriptor(), 4);
        auto raw = static_cast<int32_t*>(packet.getRawData());
        for (int i = 0; i < 4; i++)
            raw[i] = i;
        scaled.sendPacket(packet);
    }
    scheduler.waitAll();
    std::array<int32_t, 4> a{};
    void* buffers[1]{a.data()};
    SizeT count = 4;
    SizeT offset = 0;
    reader.read(buffers, &count, offset);
    ASSERT_THAT(a, ElementsAre(0, 0, 1, 1));
}

TEST_F(MultiReader2ValuesTest, ValueTypeChangeMidStreamConvertsTheNewPackets)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 5, domain, SampleType::Float64);
    addSignal(0, 5, domain, SampleType::Float64);
    auto reader = createReaderProbed(params(signalsToList(), SampleType::Float64));
    sendPackets(0);
    readSignals[1].signal.setDescriptor(setupDescriptor(SampleType::Int16));
    readSignals[0].createAndSendPacket(1);
    readSignals[1].createAndSendPacket<int16_t>(1);
    scheduler.waitAll();

    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    auto status = read(reader, values, count);
    ASSERT_EQ(count, 5u);
    ASSERT_TRUE(input(status, 1).getDescriptorChanged());
    ASSERT_EQ(input(status, 1).getDescriptor().getSampleType(), SampleType::Int16);
    count = 10;
    status = read(reader, values, count);
    ASSERT_EQ(count, 5u);
    ASSERT_THAT(std::vector<double>(values[1].begin(), values[1].begin() + 5), ElementsAre(5, 6, 7, 8, 9));
}

TEST_F(MultiReader2ValuesTest, ValueTypeChangeToInvalidAndBackUnderUndefined)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 5, domain, SampleType::Float64);
    addSignal(0, 5, domain, SampleType::Float64);
    auto reader = createReaderProbed(params(signalsToList(), SampleType::Undefined));
    readSignals[1].signal.setDescriptor(valueDescriptor(SampleType::ComplexFloat32));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());  // Undefined takes anything with a type
    ASSERT_EQ(input(status, 1).getDescriptor().getSampleType(), SampleType::ComplexFloat32);
    readSignals[1].signal.setDescriptor(valueDescriptor(SampleType::Float64));
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(input(status, 1).getDescriptorChanged());
}

TEST_F(MultiReader2ValuesTest, ValueDescriptorChangeOnTheMainIsNotADomainChange)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 5, domain);
    addSignal(0, 5, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    readSignals[0].signal.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setUnit(Unit("V")).build());
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(input(status, 0).getDescriptorChanged());
    ASSERT_FALSE(status.getDomainDescriptorChanged());
    ASSERT_FALSE(status.getResynchronized());
    ASSERT_FALSE(input(status, 1).getDescriptorChanged());
}

TEST_F(MultiReader2ValuesTest, EqualValueDescriptorIsNotAChange)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal();
    addSignal(0, 5, domain);
    addSignal(0, 5, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    readSignals[0].signal.setDescriptor(setupDescriptor(SampleType::Float64));  // equal to the one it has
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getHasChanges());
    ASSERT_FALSE(input(status, 0).getDescriptorChanged());
}

TEST_F(MultiReader2ValuesTest, LargeMagnitudesSurviveFloat64)
{
    readSignals.reserve(1);
    auto domain = createDomainSignal();
    addSignal(0, 3, domain, SampleType::Int64);
    auto reader = createReaderProbed(params(signalsToList(), SampleType::Float64));
    {
        auto domainPacket = DataPacket(domain.getDescriptor(), 3, 0);
        auto packet = DataPacketWithDomain(domainPacket, readSignals[0].signal.getDescriptor(), 3);
        auto raw = static_cast<int64_t*>(packet.getRawData());
        raw[0] = -1;
        raw[1] = 1LL << 40;
        raw[2] = -(1LL << 52);
        readSignals[0].signal.sendPacket(packet);
    }
    scheduler.waitAll();
    std::array<double, 3> a{};
    void* buffers[1]{a.data()};
    SizeT count = 3;
    SizeT offset = 0;
    reader.read(buffers, &count, offset);
    ASSERT_THAT(a, ElementsAre(-1.0, 1099511627776.0, -4503599627370496.0));
}

TEST_F(MultiReader2ValuesTest, NegativeValuesIntoUnsignedWrap)
{
    readSignals.reserve(1);
    auto domain = createDomainSignal();
    addSignal(0, 2, domain, SampleType::Int32);
    auto reader = createReaderProbed(params(signalsToList(), SampleType::UInt8));
    {
        auto domainPacket = DataPacket(domain.getDescriptor(), 2, 0);
        auto packet = DataPacketWithDomain(domainPacket, readSignals[0].signal.getDescriptor(), 2);
        auto raw = static_cast<int32_t*>(packet.getRawData());
        raw[0] = -1;
        raw[1] = 256;
        readSignals[0].signal.sendPacket(packet);
    }
    scheduler.waitAll();
    std::array<uint8_t, 2> a{};
    void* buffers[1]{a.data()};
    SizeT count = 2;
    SizeT offset = 0;
    reader.read(buffers, &count, offset);
    ASSERT_THAT(a, ElementsAre(255, 0));
}
