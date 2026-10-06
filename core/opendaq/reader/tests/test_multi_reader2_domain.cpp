/*
 * MultiReader2 domain rules: which domain descriptors a single input may carry, and how two inputs on different
 * clocks, resolutions and origins relate to the main input.
 */
#include "test_multi_reader2_common.h"

using namespace daq;
using namespace testing;

namespace multi_reader2_domain_test
{
enum class RuleKind
{
    Linear,
    Explicit,
    Constant
};

struct DomainCase
{
    const char* name;
    SampleType type;
    const char* unit;  // nullptr: no unit
    RuleKind rule;
    double delta;
    double start;
    std::int64_t resNum;  // 0: no resolution
    std::int64_t resDen;
    const char* origin;  // empty: no origin
    bool dimensions;
    bool valid;
};

constexpr const char* ORIGIN = "2022-09-27T00:02:03+00:00";

inline std::ostream& operator<<(std::ostream& os, const DomainCase& c)
{
    return os << c.name;
}
}

using multi_reader2_domain_test::DomainCase;
using multi_reader2_domain_test::ORIGIN;
using multi_reader2_domain_test::RuleKind;

class MultiReader2DomainTest : public MultiReader2Test
{
public:
    static DataDescriptorPtr build(const DomainCase& c)
    {
        auto builder = DataDescriptorBuilder().setSampleType(c.type);
        if (c.unit != nullptr)
            builder.setUnit(Unit(c.unit, -1, "seconds", "time"));
        switch (c.rule)
        {
            case RuleKind::Linear:
                if (c.delta == static_cast<double>(static_cast<Int>(c.delta)) && c.start == static_cast<double>(static_cast<Int>(c.start)))
                    builder.setRule(LinearDataRule(static_cast<Int>(c.delta), static_cast<Int>(c.start)));
                else
                    builder.setRule(LinearDataRule(c.delta, c.start));
                break;
            case RuleKind::Explicit:
                builder.setRule(ExplicitDataRule());
                break;
            case RuleKind::Constant:
                builder.setRule(ConstantDataRule());
                break;
        }
        if (c.resNum != 0)
            builder.setTickResolution(Ratio(c.resNum, c.resDen));
        if (c.origin != nullptr && c.origin[0] != 0)
            builder.setOrigin(c.origin);
        if (c.dimensions)
            builder.setDimensions(List<IDimension>(Dimension(LinearDimensionRule(1, 0, 2))));
        return builder.build();
    }

    // A value signal on a domain signal that carries the descriptor given
    ReadSignal2& addSignalOn(const DataDescriptorPtr& domainDescriptor, Int packetOffset = 0, Int packetSize = 10)
    {
        auto domain = Signal(context, nullptr, fmt::format("dom{}", readSignals.size()));
        domain.setDescriptor(domainDescriptor);
        return addSignal(packetOffset, packetSize, domain);
    }

    template <SizeT Inputs, SizeT Samples>
    MultiReader2StatusPtr readWithDomain(const MultiReader2Ptr& reader,
                                         std::array<std::int64_t, Samples>& ticks,
                                         std::array<std::array<double, Samples>, Inputs>& values,
                                         SizeT& count)
    {
        void* buffers[Inputs + 1];
        buffers[0] = ticks.data();
        for (SizeT i = 0; i < Inputs; i++)
            buffers[i + 1] = values[i].data();
        return reader.readWithDomain(buffers, &count);
    }
};

class MultiReader2DomainCaseTest : public MultiReader2DomainTest, public WithParamInterface<DomainCase>
{
};

// A single input: validity depends on the descriptor alone
TEST_P(MultiReader2DomainCaseTest, SingleInputAcceptance)
{
    const DomainCase& c = GetParam();
    readSignals.reserve(1);
    addSignalOn(build(c));
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_EQ(status.getValid(), c.valid);
    ASSERT_EQ(input(status, 0).getError(), c.valid ? MultiReader2InputError::None : MultiReader2InputError::DomainDescriptorInvalid);
    ASSERT_EQ(status.getDomainDescriptor().assigned(), c.valid);
    ASSERT_EQ(input(status, 0).getDescriptor().assigned(), c.valid);
    ASSERT_EQ(reader.getMainInput(), c.valid ? readSignals[0].signal.getGlobalId() : String(""));
}

class MultiReader2InvalidDomainCaseTest : public MultiReader2DomainTest, public WithParamInterface<DomainCase>
{
};

// A second, standard 1 kHz input next to it: an invalid descriptor is still only that input's error
TEST_P(MultiReader2InvalidDomainCaseTest, InvalidDescriptorDoesNotMarkTheNeighbour)
{
    const DomainCase& c = GetParam();
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignalOn(build(c));
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);
    ASSERT_EQ(reader.getMainInput(), "");

    // Setting the bad input unused makes the reader whole again
    auto p = params(signalsToList());
    p.setInputUsed(readSignals[1].signal, false);
    reader.configure(p);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(reader.getMainInput(), readSignals[0].signal.getGlobalId());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);
    ASSERT_FALSE(input(status, 1).getUsed());
}

static std::vector<DomainCase> domainCases()
{
    return {
        // sample types
        DomainCase{"Int64", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, false, true},
        DomainCase{"UInt64", SampleType::UInt64, "s", RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, false, true},
        DomainCase{"Int32", SampleType::Int32, "s", RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, false, true},
        DomainCase{"UInt32", SampleType::UInt32, "s", RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, false, true},
        DomainCase{"Int16", SampleType::Int16, "s", RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, false, true},
        DomainCase{"UInt16", SampleType::UInt16, "s", RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, false, true},
        DomainCase{"Int8", SampleType::Int8, "s", RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, false, true},
        DomainCase{"UInt8", SampleType::UInt8, "s", RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, false, true},
        DomainCase{"Float64IsNotATick", SampleType::Float64, "s", RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, false, false},
        DomainCase{"Float32IsNotATick", SampleType::Float32, "s", RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, false, false},
        // units
        DomainCase{"NoUnit", SampleType::Int64, nullptr, RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, false, false},
        DomainCase{"MillisecondUnit", SampleType::Int64, "ms", RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, false, false},
        // rules
        DomainCase{"ExplicitRule", SampleType::Int64, "s", RuleKind::Explicit, 0, 0, 1, 1000, ORIGIN, false, false},
        DomainCase{"ConstantRule", SampleType::Int64, "s", RuleKind::Constant, 0, 0, 1, 1000, ORIGIN, false, false},
        DomainCase{"ZeroDelta", SampleType::Int64, "s", RuleKind::Linear, 0, 0, 1, 1000, ORIGIN, false, false},
        DomainCase{"FractionalDelta", SampleType::Int64, "s", RuleKind::Linear, 0.5, 0, 1, 1000, ORIGIN, false, false},
        DomainCase{"NegativeDelta", SampleType::Int64, "s", RuleKind::Linear, -1, 0, 1, 1000, ORIGIN, false, false},
        DomainCase{"FractionalStart", SampleType::Int64, "s", RuleKind::Linear, 1, 0.5, 1, 1000, ORIGIN, false, false},
        DomainCase{"StartSeven", SampleType::Int64, "s", RuleKind::Linear, 1, 7, 1, 1000, ORIGIN, false, true},
        DomainCase{"LargeStart", SampleType::Int64, "s", RuleKind::Linear, 1, 1000000000000, 1, 1000, ORIGIN, false, true},
        DomainCase{"DeltaThousandIsOneHertz", SampleType::Int64, "s", RuleKind::Linear, 1000, 0, 1, 1000, ORIGIN, false, true},
        // resolutions
        DomainCase{"Millisecond", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, false, true},
        DomainCase{"Microsecond", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000000, ORIGIN, false, true},
        DomainCase{"Nanosecond", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000000000, ORIGIN, false, true},
        DomainCase{"FifteenMegahertz", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 15000000, ORIGIN, false, true},
        DomainCase{"TwoPerThousandIsFiveHundredHertz", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 2, 1000, ORIGIN, false, true},
        DomainCase{"OneThirdIsThreeHertz", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 3, ORIGIN, false, true},
        DomainCase{"TenthOfASecond", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 10, ORIGIN, false, true},
        DomainCase{"NoResolutionIsOneHertz", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 0, 0, ORIGIN, false, true},
        DomainCase{"ThreePerThousandIsNotAWholeRate", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 3, 1000, ORIGIN, false, false},
        DomainCase{"DeltaThreeIsNotAWholeRate", SampleType::Int64, "s", RuleKind::Linear, 3, 0, 1, 1000, ORIGIN, false, false},
        DomainCase{"DeltaSevenIsNotAWholeRate", SampleType::Int64, "s", RuleKind::Linear, 7, 0, 1, 1000, ORIGIN, false, false},
        DomainCase{"ThousandSecondTicksAreBelowOneHertz", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1000, 1, ORIGIN, false, false},
        DomainCase{"TenKilohertzTickDeltaTen", SampleType::Int64, "s", RuleKind::Linear, 10, 0, 1, 10000, ORIGIN, false, true},
        // origins
        DomainCase{"OriginUtcOffset", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, "2022-09-27T00:02:03+00:00", false, true},
        DomainCase{"OriginZulu", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, "2022-09-27T00:02:03Z", false, true},
        DomainCase{"OriginPositiveZone", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, "2022-09-27T02:02:03+02:00", false, true},
        DomainCase{"OriginNoZone", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, "2022-09-27T00:02:03", false, true},
        DomainCase{"OriginDateOnly", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, "2022-09-27", false, true},
        DomainCase{"OriginYearMonth", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, "2022-09", false, true},
        DomainCase{"OriginYear", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, "2022", false, true},
        DomainCase{
            "OriginZeroFraction", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, "2022-09-27T00:02:03.000+00:00", false, true},
        DomainCase{"OriginUnixEpoch", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, "1970-01-01T00:00:00+00:00", false, true},
        DomainCase{
            "OriginHalfSecond", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, "2022-09-27T00:02:03.5+00:00", false, false},
        DomainCase{
            "OriginMilliseconds", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, "2022-09-27T00:02:03.123+00:00", false, false},
        DomainCase{"OriginGarbage", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, "yesterday", false, false},
        DomainCase{"OriginNone", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, "", false, true},
        // shape
        DomainCase{"DimensionedDomain", SampleType::Int64, "s", RuleKind::Linear, 1, 0, 1, 1000, ORIGIN, true, false}};
}

static std::vector<DomainCase> invalidDomainCases()
{
    std::vector<DomainCase> out;
    for (const auto& c : domainCases())
        if (!c.valid)
            out.push_back(c);
    return out;
}

INSTANTIATE_TEST_SUITE_P(Descriptors,
                         MultiReader2DomainCaseTest,
                         ValuesIn(domainCases()),
                         [](const TestParamInfo<DomainCase>& info) { return std::string(info.param.name); });

INSTANTIATE_TEST_SUITE_P(Descriptors,
                         MultiReader2InvalidDomainCaseTest,
                         ValuesIn(invalidDomainCases()),
                         [](const TestParamInfo<DomainCase>& info) { return std::string(info.param.name); });

// ---------------------------------------------------------------- two inputs

TEST_F(MultiReader2DomainTest, RateMismatchIsTheNonMainInputsError)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 0)));
    addSignal(0, 10, createDomainSignal("", Ratio(1, 10000), LinearDataRule(5, 0)));  // 2 kHz
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);
}

TEST_F(MultiReader2DomainTest, PinnedMainDecidesWhichSideOfARateMismatchErrors)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 0)));
    addSignal(0, 10, createDomainSignal("", Ratio(1, 10000), LinearDataRule(5, 0)));
    auto p = params(signalsToList());
    p.setMainInput(readSignals[1].signal);
    auto reader = createReader(p);
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::DomainDescriptorInvalid);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::None);
}

TEST_F(MultiReader2DomainTest, HalfRateThroughTheResolutionIsInvalid)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 0)));
    addSignal(0, 10, createDomainSignal("", Ratio(2, 1000), LinearDataRule(1, 0)));  // 500 Hz
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);
}

TEST_F(MultiReader2DomainTest, OriginAgainstNoOriginIsInvalid)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal(ORIGIN));
    addSignal(0, 10, createDomainSignal());
    readSignals[1].signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(
        DataDescriptorBuilderCopy(readSignals[1].domainDescriptor()).setOrigin("").build());
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);
}

TEST_F(MultiReader2DomainTest, TwoInputsWithoutOriginsAlignOnTicks)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(100, 10, createDomainSignal());
    for (auto& read : readSignals)
        read.signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(
            DataDescriptorBuilderCopy(read.domainDescriptor()).setOrigin("").build());
    auto reader = createReaderProbed(params(signalsToList()));
    for (Int i = 0; i < 15; i++)
        sendPackets(i);
    std::array<std::int64_t, 5> ticks{};
    std::array<std::array<double, 5>, 2> values{};
    SizeT count = 5;
    readWithDomain(reader, ticks, values, count);
    ASSERT_EQ(count, 5u);
    ASSERT_THAT(ticks, ElementsAre(100, 101, 102, 103, 104));
    ASSERT_THAT(values[0], ElementsAreArray(values[1]));
}

TEST_F(MultiReader2DomainTest, EquivalentOriginsInDifferentZonesAlignWithoutOffset)
{
    readSignals.reserve(3);
    addSignal(0, 10, createDomainSignal("2022-09-27T00:02:03+00:00"));
    addSignal(0, 10, createDomainSignal("2022-09-27T02:02:03+02:00"));
    addSignal(0, 10, createDomainSignal("2022-09-27T00:02:03Z"));
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
    std::array<std::int64_t, 3> ticks{};
    std::array<std::array<double, 3>, 3> values{};
    SizeT count = 3;
    readWithDomain(reader, ticks, values, count);
    ASSERT_THAT(ticks, ElementsAre(0, 1, 2));
    ASSERT_THAT(values[1], ElementsAre(0, 1, 2));
    ASSERT_THAT(values[2], ElementsAre(0, 1, 2));
}

TEST_F(MultiReader2DomainTest, DateOnlyOriginIsMidnight)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal("2022-09-27"));
    addSignal(0, 10, createDomainSignal("2022-09-27T00:00:01+00:00"));
    auto reader = createReaderProbed(params(signalsToList()));
    for (Int i = 0; i < 101; i++)
        sendPackets(i);
    // Input 1 tick 0 is one second after midnight: main tick 1000
    std::array<std::int64_t, 3> ticks{};
    std::array<std::array<double, 3>, 2> values{};
    SizeT count = 3;
    readWithDomain(reader, ticks, values, count);
    ASSERT_THAT(ticks, ElementsAre(1000, 1001, 1002));
    ASSERT_THAT(values[0], ElementsAre(1000, 1001, 1002));
    ASSERT_THAT(values[1], ElementsAre(0, 1, 2));
}

TEST_F(MultiReader2DomainTest, OriginsAMinuteApartAlign)
{
    readSignals.reserve(2);
    addSignal(0, 1000, createDomainSignal("2022-09-27T00:02:03+00:00"));
    addSignal(0, 1000, createDomainSignal("2022-09-27T00:03:03+00:00"));
    auto reader = createReaderProbed(params(signalsToList()));
    // The first input starts a minute early: beyond the default distance, so the guard is widened for the test
    manager(reader).setSyncLimits(std::chrono::seconds(60), std::chrono::seconds(120));
    for (Int i = 0; i < 61; i++)
        sendPackets(i);
    std::array<std::int64_t, 2> ticks{};
    std::array<std::array<double, 2>, 2> values{};
    SizeT count = 2;
    readWithDomain(reader, ticks, values, count);
    ASSERT_THAT(ticks, ElementsAre(60000, 60001));
    ASSERT_THAT(values[1], ElementsAre(0, 1));
}

TEST_F(MultiReader2DomainTest, MicrosecondInputScalesOntoAMillisecondMain)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 0)));
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000000), LinearDataRule(1000, 0)));
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    std::array<std::int64_t, 3> ticks{};
    std::array<std::array<double, 3>, 2> values{};
    SizeT count = 3;
    readWithDomain(reader, ticks, values, count);
    ASSERT_THAT(ticks, ElementsAre(0, 1, 2));
    ASSERT_THAT(values[1], ElementsAre(0, 1000, 2000));
}

TEST_F(MultiReader2DomainTest, MillisecondInputScalesOntoAMicrosecondMain)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000000), LinearDataRule(1000, 0)));
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 0)));
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    sendPackets(1);
    std::array<std::int64_t, 3> ticks{};
    std::array<std::array<double, 3>, 2> values{};
    SizeT count = 3;
    readWithDomain(reader, ticks, values, count);
    ASSERT_THAT(ticks, ElementsAre(0, 1000, 2000));
    ASSERT_THAT(values[1], ElementsAre(0, 1, 2));
    SizeT offset = 0;
    count = 3;
    void* buffers[2]{values[0].data(), values[1].data()};
    reader.read(buffers, &count, offset);
    ASSERT_EQ(offset, 3000u);
}

TEST_F(MultiReader2DomainTest, ThreeResolutionsAlign)
{
    readSignals.reserve(3);
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 0)));
    addSignal(0, 10, createDomainSignal("", Ratio(1, 10000), LinearDataRule(10, 0)));
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000000), LinearDataRule(1000, 0)));
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    sendPackets(1);
    ASSERT_EQ(reader.getAvailableCount(), 20u);
    std::array<std::int64_t, 20> ticks{};
    std::array<std::array<double, 20>, 3> values{};
    SizeT count = 20;
    readWithDomain(reader, ticks, values, count);
    ASSERT_EQ(count, 20u);
    for (SizeT k = 0; k < 20; k++)
    {
        ASSERT_EQ(ticks[k], static_cast<std::int64_t>(k));
        ASSERT_EQ(values[0][k], static_cast<double>(k));
        ASSERT_EQ(values[1][k], static_cast<double>(10 * k));
        ASSERT_EQ(values[2][k], static_cast<double>(1000 * k));
    }
}

TEST_F(MultiReader2DomainTest, OffsetOriginAndResolutionCombine)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal("2022-09-27T00:02:03+00:00", Ratio(1, 1000), LinearDataRule(1, 0)));
    addSignal(0, 10, createDomainSignal("2022-09-27T00:02:04+00:00", Ratio(1, 10000), LinearDataRule(10, 0)));
    auto reader = createReaderProbed(params(signalsToList()));
    for (Int i = 0; i < 101; i++)
        sendPackets(i);
    std::array<std::int64_t, 2> ticks{};
    std::array<std::array<double, 2>, 2> values{};
    SizeT count = 2;
    readWithDomain(reader, ticks, values, count);
    ASSERT_THAT(ticks, ElementsAre(1000, 1001));
    ASSERT_THAT(values[1], ElementsAre(0, 10));
}

TEST_F(MultiReader2DomainTest, RuleStartsShiftTheTicksNotTheValues)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 0)));
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 7)));
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    // Input 1 packet offset 0 is tick 7; the common start is tick 7
    std::array<std::int64_t, 3> ticks{};
    std::array<std::array<double, 3>, 2> values{};
    SizeT count = 3;
    readWithDomain(reader, ticks, values, count);
    ASSERT_THAT(ticks, ElementsAre(7, 8, 9));
    ASSERT_THAT(values[0], ElementsAre(7, 8, 9));
    ASSERT_THAT(values[1], ElementsAre(0, 1, 2));
}

TEST_F(MultiReader2DomainTest, OffLatticeDataIsADomainErrorUntilTheDescriptorChanges)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal("", Ratio(1, 1000), LinearDataRule(1, 0)));
    addSignal(5, 10, createDomainSignal("", Ratio(1, 10000), LinearDataRule(10, 0)));  // tick 5 is half a main tick
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);

    // A new domain descriptor clears the error; on-grid data then aligns
    readSignals[1].signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(
        createDomainDescriptor("", Ratio(1, 10000), LinearDataRule(10, 10)));
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::None);
    readSignals[0].sendAt(0, 10);
    readSignals[1].sendAt(0, 10);  // tick 10 .. 100
    scheduler.waitAll();
    std::array<std::int64_t, 3> ticks{};
    std::array<std::array<double, 3>, 2> values{};
    SizeT count = 3;
    readWithDomain(reader, ticks, values, count);
    ASSERT_EQ(count, 3u);
    ASSERT_THAT(ticks, ElementsAre(1, 2, 3));
}

TEST_F(MultiReader2DomainTest, DomainSignalWithoutADescriptorIsInvalidUntilItGetsOne)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    auto bareDomain = Signal(context, nullptr, "bare");
    addSignal(0, 10, bareDomain);
    auto reader = createReader(params(signalsToList()));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);

    bareDomain.setDescriptor(createDomainDescriptor());
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::None);
    sendPackets(0);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2DomainTest, SharedDomainSignalChangeTouchesEveryInput)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal();
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    addSignal(0, 10, domain);
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    std::array<std::array<double, 10>, 3> values{};
    SizeT count = 10;
    read(reader, values, count);

    domain.setDescriptor(createDomainDescriptor("2022-09-27T00:02:04+00:00"));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getDomainDescriptorChanged());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_EQ(status.getDomainDescriptor().getOrigin(), "2022-09-27T00:02:04+00:00");
    for (SizeT i = 0; i < 3; i++)
        ASSERT_EQ(input(status, i).getError(), MultiReader2InputError::None);
    sendPackets(1);
    count = 10;
    SizeT offset = 0;
    read(reader, values, count, &offset);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(offset, 10u);
}

TEST_F(MultiReader2DomainTest, NonMainRateChangeMidStreamInvalidatesAndTheOldRateRecovers)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    read(reader, values, count);

    readSignals[1].signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(
        createDomainDescriptor("", Ratio(1, 1000), LinearDataRule(2, 0)));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);

    readSignals[1].signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(createDomainDescriptor());
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getResynchronized());
    sendPackets(1);
    ASSERT_EQ(reader.getAvailableCount(), 10u);
}

TEST_F(MultiReader2DomainTest, MainRateChangeMidStreamMakesTheOthersInvalid)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    read(reader, values, count);

    // The main is always valid against itself; the other no longer fits
    readSignals[0].signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(
        createDomainDescriptor("", Ratio(1, 1000), LinearDataRule(2, 0)));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_FALSE(status.getValid());
    ASSERT_EQ(input(status, 0).getError(), MultiReader2InputError::None);
    ASSERT_EQ(input(status, 1).getError(), MultiReader2InputError::DomainDescriptorInvalid);
}

TEST_F(MultiReader2DomainTest, ResolutionChangeAtTheSameRateKeepsStreaming)
{
    readSignals.reserve(2);
    addSignal(0, 10, createDomainSignal());
    addSignal(0, 10, createDomainSignal());
    auto reader = createReaderProbed(params(signalsToList()));
    sendPackets(0);
    std::array<std::array<double, 10>, 2> values{};
    SizeT count = 10;
    read(reader, values, count);

    readSignals[1].signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(
        createDomainDescriptor("", Ratio(1, 10000), LinearDataRule(10, 0)));
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_FALSE(status.getDomainDescriptorChanged());

    readSignals[0].sendAt(10, 10);
    readSignals[1].sendAt(100, 10);
    scheduler.waitAll();
    SizeT offset = 0;
    count = 10;
    read(reader, values, count, &offset);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(offset, 10u);
    ASSERT_EQ(values[1][0], 100);
}
