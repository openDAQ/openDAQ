// White-box tests of the MultiReader2 units that hold no lock and call no user code: the tick arithmetic onto the main
// input's grid, and one input's queue. Their sources are compiled into this test, as they are not exported from the library.

#include <gtest/gtest.h>
#include <opendaq/multi_reader2_domain.h>
#include <opendaq/multi_reader2_queue.h>
#include <opendaq/multi_reader2_factory.h>
#include <opendaq/opendaq.h>

#include <limits>
#include <vector>

using namespace daq;
using namespace daq::multi_reader2;
using QueueEvent = daq::multi_reader2::Event;

namespace
{
DataDescriptorPtr domainDescriptor(Int resNum,
                                   Int resDen,
                                   Int delta,
                                   Int start = 0,
                                   const std::string& origin = "1970-01-01T00:00:00Z",
                                   SampleType type = SampleType::Int64,
                                   const std::string& unit = "s")
{
    return DataDescriptorBuilder()
        .setSampleType(type)
        .setUnit(Unit(unit))
        .setTickResolution(Ratio(resNum, resDen))
        .setRule(LinearDataRule(delta, start))
        .setOrigin(origin)
        .build();
}

DataPacketPtr valuePacket(SizeT count)
{
    static const auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    return DataPacket(descriptor, count);
}

struct Taken
{
    SizeT first;
    SizeT count;
};

std::vector<Taken> takeAll(InputQueue& queue, SizeT count)
{
    std::vector<Taken> taken;
    queue.take(count, [&](const DataPacketPtr&, SizeT first, SizeT n) { taken.push_back({first, n}); });
    return taken;
}
}

// ---------------------------------------------------------------- domain

TEST(MultiReader2Domain, ParsesARegularDomain)
{
    const auto ticks = parseDomain(domainDescriptor(1, 1000000, 1000, 5, "1970-01-01T00:00:10Z"));
    ASSERT_TRUE(ticks.has_value());
    EXPECT_EQ(ticks->rate, 1000);
    EXPECT_EQ(ticks->delta, 1000);
    EXPECT_EQ(ticks->start, 5);
    EXPECT_EQ(ticks->originSeconds, 10);
    EXPECT_EQ(ticks->ticksIn(1), 1000000);
}

TEST(MultiReader2Domain, AcceptsAYearOrAYearAndMonthAsOrigin)
{
    EXPECT_EQ(parseDomain(domainDescriptor(1, 1000, 1, 0, "1970"))->originSeconds, 0);
    EXPECT_EQ(parseDomain(domainDescriptor(1, 1000, 1, 0, "1970-02"))->originSeconds, 31 * 24 * 3600);
}

TEST(MultiReader2Domain, RejectsEveryBrokenConstraint)
{
    EXPECT_FALSE(parseDomain(nullptr).has_value());
    EXPECT_FALSE(parseDomain(domainDescriptor(1, 1000, 1, 0, "1970-01-01T00:00:00Z", SampleType::Float64)).has_value());
    EXPECT_FALSE(parseDomain(domainDescriptor(1, 1000, 1, 0, "1970-01-01T00:00:00Z", SampleType::Int64, "ms")).has_value());
    EXPECT_FALSE(parseDomain(domainDescriptor(1, 1000, 3)).has_value());  // 333.3 samples per second
    EXPECT_FALSE(parseDomain(domainDescriptor(1, 1000, 1, 0, "1970-01-01T00:00:00.5Z")).has_value());
}

TEST(MultiReader2Domain, RelatesEqualRatesWithDifferentResolutions)
{
    // 1 kHz both: the input counts in ms, the main input in us with delta 1000; the input's origin is 1 s later
    const auto main = *parseDomain(domainDescriptor(1, 1000000, 1000));
    const auto input = *parseDomain(domainDescriptor(1, 1000, 1, 0, "1970-01-01T00:00:01Z"));
    const auto map = relate(input, main);
    ASSERT_TRUE(map.has_value());
    EXPECT_EQ(map->step, 1000);
    EXPECT_EQ(map->offset, 1000000);
    EXPECT_EQ(map->firstTick(3), 1003000);
}

TEST(MultiReader2Domain, FindsTicksBetweenTheMainTicks)
{
    // The input counts in ms, the main input in 2 ms with delta 1: input tick 3 lies between two main ticks
    const auto main = *parseDomain(domainDescriptor(1, 500, 1));
    const auto input = *parseDomain(domainDescriptor(1, 1000, 2));
    const auto map = relate(input, main);
    ASSERT_TRUE(map.has_value());
    EXPECT_EQ(map->firstTick(4), 2);
    EXPECT_FALSE(map->firstTick(3).has_value());
}

TEST(MultiReader2Domain, RefusesWhatCannotBeReadTogether)
{
    const auto main = *parseDomain(domainDescriptor(1, 1000, 1));
    EXPECT_FALSE(relate(*parseDomain(domainDescriptor(1, 1000, 2)), main).has_value());  // 500 Hz against 1 kHz

    // An origin difference that does not fit in Int64 main ticks. Short of that, whole-second origins at integer rates
    // are always a whole number of main ticks apart
    const auto fine = *parseDomain(domainDescriptor(1, 1000000000, 1000000));
    EXPECT_FALSE(relate(*parseDomain(domainDescriptor(1, 1000000000, 1000000, 0, "2500-01-01T00:00:00Z")), fine).has_value());
}

TEST(MultiReader2Domain, CheckedArithmeticSaturatesInsteadOfWrapping)
{
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    constexpr auto min = std::numeric_limits<std::int64_t>::min();
    EXPECT_FALSE(checkedAdd(max, 1).has_value());
    EXPECT_FALSE(checkedSub(min, 1).has_value());
    EXPECT_FALSE(checkedMul(max / 2 + 1, 2).has_value());
    EXPECT_EQ(checkedMul(-3, 4), -12);
    EXPECT_TRUE(exceeds(max, min, 0));
    EXPECT_FALSE(exceeds(min, max, 0));
    EXPECT_FALSE(exceeds(10, 8, 2));
    EXPECT_TRUE(exceeds(11, 8, 2));

    const TickMap map{1, 1, max, 0, 1};
    EXPECT_FALSE(map.firstTick(1).has_value());
}

// A domain without an origin is relative: it is read only together with other domains without one, on a shared time base
TEST(MultiReader2Domain, DomainWithoutOriginIsRelativeAndRelatesOnlyToRelativeDomains)
{
    const auto relative = parseDomain(domainDescriptor(1, 1000, 1, 0, ""));
    const auto absolute = parseDomain(domainDescriptor(1, 1000, 1, 0));
    ASSERT_TRUE(relative.has_value() && absolute.has_value());
    EXPECT_TRUE(relative->relative);
    EXPECT_FALSE(absolute->relative);
    const auto map = relate(*relative, *relative);
    ASSERT_TRUE(map.has_value());
    EXPECT_EQ(map->offset, 0);
    EXPECT_FALSE(relate(*relative, *absolute).has_value());
    EXPECT_FALSE(relate(*absolute, *relative).has_value());
}

// A failed constraint is named, so the status message can say which one
TEST(MultiReader2Domain, NamesTheFailedConstraint)
{
    const char* fault = nullptr;
    EXPECT_FALSE(parseDomain(domainDescriptor(1, 1000, 1, 0, "1970-01-01T00:00:00Z", SampleType::Float64), &fault).has_value());
    EXPECT_STREQ(fault, "ticks not of an integer type");
    EXPECT_FALSE(parseDomain(domainDescriptor(1, 1000, 1, 0, "1970-01-01T00:00:00Z", SampleType::Int64, "ms"), &fault).has_value());
    EXPECT_STREQ(fault, "unit not seconds");
    EXPECT_FALSE(parseDomain(domainDescriptor(1, 1000, 1, 0, "1970-01-01T00:00:00.5Z"), &fault).has_value());
    EXPECT_STREQ(fault, "origin not an ISO 8601 time on a full second");
    const auto relative = parseDomain(domainDescriptor(1, 1000, 1, 0, ""));
    const auto absolute = parseDomain(domainDescriptor(1, 1000, 1, 0));
    const auto faster = parseDomain(domainDescriptor(1, 2000, 1, 0));
    ASSERT_TRUE(relative.has_value() && absolute.has_value() && faster.has_value());
    EXPECT_FALSE(relate(*absolute, *relative, &fault).has_value());
    EXPECT_STREQ(fault, "origin while the main input has none");
    EXPECT_FALSE(relate(*relative, *absolute, &fault).has_value());
    EXPECT_STREQ(fault, "no origin while the main input has one");
    EXPECT_FALSE(relate(*faster, *absolute, &fault).has_value());
    EXPECT_STREQ(fault, "sampling rate differs from the main input's");
}

TEST(MultiReader2Domain, ReadsTheMinutesAndTheSignOfAnOriginOffset)
{
    EXPECT_EQ(parseOrigin("1970-01-01T01:30:00+01:30"), 0);
    EXPECT_EQ(parseOrigin("1970-01-01T00:00:00-05:30"), 5 * 3600 + 30 * 60);
    EXPECT_EQ(parseOrigin("1970-01-01T01:30:00+0130"), 0);
    EXPECT_EQ(parseOrigin("1970-01-01T02:00:00+02"), 0);
    EXPECT_EQ(parseOrigin("1970-01-01T00:00:10Z"), 10);
    EXPECT_EQ(parseOrigin("1970-01-01T00:00:10"), 10);
    EXPECT_EQ(parseOrigin("1970-01-02"), 24 * 3600);
    EXPECT_FALSE(parseOrigin("1970-01-01T00:00:00.5Z").has_value());
    EXPECT_FALSE(parseOrigin("1970-01-01T00:00:00+1:30").has_value());
    EXPECT_FALSE(parseOrigin("1970-01-01T00:00:00Z junk").has_value());
    EXPECT_FALSE(parseOrigin("").has_value());
}

TEST(MultiReader2Domain, RejectsAPeriodThatOverflows)
{
    // Resolution numerator 2^62 and delta 4: the period in seconds does not fit in Int64
    EXPECT_FALSE(parseDomain(domainDescriptor(Int(1) << 62, 1, 4)).has_value());
}

TEST(MultiReader2Domain, LimitsSaturateInsteadOfWrapping)
{
    // 2^62 ticks per second: 3 s of data does not fit in Int64
    const auto ticks = parseDomain(domainDescriptor(1, Int(1) << 62, 1));
    ASSERT_TRUE(ticks.has_value());
    EXPECT_EQ(ticks->samplesIn(3), std::numeric_limits<std::int64_t>::max());
    EXPECT_EQ(ticks->ticksIn(2), std::numeric_limits<std::int64_t>::max());
    EXPECT_EQ(parseDomain(domainDescriptor(1, 1000, 1))->samplesIn(3), 3000);
}

// ---------------------------------------------------------------- queue

TEST(MultiReader2Queue, CountsTheFrontRunUpToTheFirstEvent)
{
    InputQueue queue;
    queue.pushData(valuePacket(10), 10, 0, 80);
    queue.pushData(valuePacket(5), 5, 10, 40);
    queue.pushEvent({QueueEvent::Kind::Gap});
    queue.pushData(valuePacket(7), 7, 100, 56);

    EXPECT_EQ(queue.frontCount(), 15u);
    EXPECT_EQ(queue.sampleCount(), 22u);
    EXPECT_EQ(queue.byteCount(), 176u);
    EXPECT_EQ(queue.eventCount(), 1u);
    EXPECT_EQ(queue.frontTick(), 0);
    EXPECT_EQ(queue.frontLastTick(), 14);
}

TEST(MultiReader2Queue, TakesAcrossPacketsAndMovesTheTicks)
{
    InputQueue queue;
    queue.setStep(10);
    queue.pushData(valuePacket(4), 4, 1000, 32);
    queue.pushData(valuePacket(4), 4, 1040, 32);

    const auto taken = takeAll(queue, 6);
    ASSERT_EQ(taken.size(), 2u);
    EXPECT_EQ(taken[0].first, 0u);
    EXPECT_EQ(taken[0].count, 4u);
    EXPECT_EQ(taken[1].first, 0u);
    EXPECT_EQ(taken[1].count, 2u);
    EXPECT_EQ(queue.frontCount(), 2u);
    EXPECT_EQ(queue.frontTick(), 1060);
    EXPECT_EQ(queue.byteCount(), 32u);  // a packet's bytes leave with its last sample
}

TEST(MultiReader2Queue, PoppingAnEventCountsTheNextRun)
{
    InputQueue queue;
    queue.pushEvent({QueueEvent::Kind::Gap});
    queue.pushData(valuePacket(3), 3, 0, 0);
    queue.pushData(valuePacket(3), 3, 3, 0);
    queue.pushEvent({QueueEvent::Kind::Disconnected});
    EXPECT_EQ(queue.frontCount(), 0u);
    EXPECT_FALSE(queue.frontTick().has_value());

    EXPECT_EQ(queue.popEvent().kind, QueueEvent::Kind::Gap);
    EXPECT_EQ(queue.frontCount(), 6u);
    queue.skip(6);
    EXPECT_TRUE(queue.frontIsEvent());
}

TEST(MultiReader2Queue, DiscardsTheSamplesBeforeATick)
{
    InputQueue queue;
    queue.setStep(2);
    queue.pushData(valuePacket(5), 5, 0, 0);    // ticks 0 to 8
    queue.pushData(valuePacket(5), 5, 10, 0);   // ticks 10 to 18

    queue.discardBefore(13);  // into the second packet: up to tick 14
    EXPECT_EQ(queue.frontTick(), 14);
    EXPECT_EQ(queue.frontCount(), 3u);

    queue.discardBefore(100);
    EXPECT_FALSE(queue.hasData());
}

TEST(MultiReader2Queue, DroppingDataKeepsTheEvents)
{
    InputQueue queue;
    queue.pushData(valuePacket(3), 3, 0, 24);
    queue.pushEvent({QueueEvent::Kind::Gap});
    queue.pushData(valuePacket(3), 3, 10, 24);

    queue.dropData();
    EXPECT_EQ(queue.eventCount(), 1u);
    EXPECT_EQ(queue.sampleCount(), 0u);
    EXPECT_EQ(queue.byteCount(), 0u);
    EXPECT_TRUE(queue.frontIsEvent());
}

TEST(MultiReader2Queue, KeepsATotalOverQueuesAndHandsBackWhatItDrops)
{
    SizeT total = 0;
    InputQueue a;
    InputQueue b;
    a.total = &total;
    b.total = &total;
    a.pushData(valuePacket(3), 3, 0, 24);
    b.pushData(valuePacket(5), 5, 0, 40);
    a.pushEvent({QueueEvent::Kind::Gap});
    a.pushData(valuePacket(2), 2, 10, 16);
    EXPECT_EQ(total, 80u);

    a.skip(3);
    EXPECT_EQ(total, 56u);
    const auto dropped = a.dropData();
    EXPECT_EQ(dropped.size(), 1u);
    EXPECT_EQ(total, 40u);
    b.clear();
    EXPECT_EQ(total, 0u);
}

TEST(MultiReader2Queue, AFailureMarkerGoesInFrontOfTheData)
{
    InputQueue queue;
    queue.pushData(valuePacket(3), 3, 0, 0);
    queue.pushFrontEvent({QueueEvent::Kind::SyncFailed, nullptr, nullptr, SyncCause::Ahead});
    EXPECT_TRUE(queue.frontIsEvent());
    EXPECT_EQ(queue.frontCount(), 0u);
    EXPECT_EQ(queue.frontEvent().cause, SyncCause::Ahead);
}

TEST(MultiReader2Queue, CancelsOnlyADisconnectAtTheTail)
{
    InputQueue queue;
    queue.pushEvent({QueueEvent::Kind::Disconnected});
    queue.pushData(valuePacket(3), 3, 0, 0);
    EXPECT_FALSE(queue.cancelTrailingDisconnect());

    queue.pushEvent({QueueEvent::Kind::Disconnected});
    EXPECT_TRUE(queue.cancelTrailingDisconnect());
    EXPECT_EQ(queue.eventCount(), 1u);
}

TEST(MultiReader2Queue, RingKeepsTheOrderAcrossWrapGrowthAndShrink)
{
    Ring<int> ring;
    for (int i = 0; i < 6; ++i)
        ring.pushBack(i);
    ring.popFront();
    ring.popFront();
    for (int i = 6; i < 12; ++i)
        ring.pushBack(i);  // wraps around the array, then grows it
    ring.pushFront(1);
    ASSERT_EQ(ring.size(), 11u);
    for (SizeT i = 0; i < ring.size(); ++i)
        EXPECT_EQ(ring[i], static_cast<int>(i + 1));

    for (int i = 12; i < 300; ++i)
        ring.pushBack(i);
    while (ring.size() > 3)
        ring.popFront();  // shrinks on the way
    EXPECT_EQ(ring.front(), 297);
    EXPECT_EQ(ring.back(), 299);
    ring.popBack();
    EXPECT_EQ(ring.back(), 298);
}

// ---------------------------------------------------------------- facade

TEST(MultiReader2Robustness, ConfigureWithANonComponentInTheInputListFailsWithoutThrowing)
{
    const auto reader = MultiReader2(MultiReader2Params());
    const auto params = MultiReader2Params();
    auto inputs = List<IBaseObject>();
    inputs.pushBack(Integer(1));
    ASSERT_EQ(params->setInputs(inputs), OPENDAQ_SUCCESS);

    IMultiReader2Status* status = nullptr;
    ErrCode errCode = OPENDAQ_SUCCESS;
    EXPECT_NO_THROW(errCode = reader->configure(params, &status));
    EXPECT_TRUE(OPENDAQ_FAILED(errCode));
    EXPECT_EQ(status, nullptr);
    daqClearErrorInfo();
}
