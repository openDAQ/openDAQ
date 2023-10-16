#include <date/date.h>
#include <gtest/gtest.h>

#include <opendaq/time_reader.h>

#include <chrono>
#include <sstream>
#include <string>

using DateTest = testing::Test;

struct DateTime
{
    using Clock = std::chrono::system_clock;

    using ClockTime = date::time_of_day<Clock::duration>;
    using ClockDate = date::year_month_day;

    explicit DateTime(Clock::time_point timePoint)
    {
        auto days = std::chrono::time_point_cast<date::days>(timePoint);
        time = date::make_time(timePoint - days);
        date = days;
    }

    [[nodiscard]]
    bool equalsTime(ClockTime other) const
    {
        return other.to_duration() == time.to_duration();
    }

    ClockTime time{};
    ClockDate date{};
};


TEST_F(DateTest, ParseUtc)
{
    using namespace date;
    using namespace std::chrono;

    using ClockTime = time_of_day<system_clock::duration>;

    const auto iso8601 = daq::reader::fixupIso8601("2022-09-27T00:02:03+00:00");
    std::istringstream epochString(iso8601);

    system_clock::time_point timePoint{};
    epochString >> date::parse("%FT%T%z", timePoint);

    DateTime dateTime(timePoint);
    ASSERT_EQ(2022_y / 9 / 27, dateTime.date);
    ASSERT_TRUE(dateTime.equalsTime(ClockTime{hours{0} + minutes{2} + seconds(3)}));
}

TEST_F(DateTest, ParseTimeZone)
{
    using namespace date;
    using namespace std::chrono;

    using ClockTime = time_of_day<system_clock::duration>;

    const auto iso8601 = daq::reader::fixupIso8601("2022-09-27T00:02:03+01:00");
    std::istringstream epochString(iso8601);

    system_clock::time_point timePoint{};
    epochString >> date::parse("%FT%T%z", timePoint);

    DateTime dateTime(timePoint);
    ASSERT_EQ(2022_y / 9 / 26, dateTime.date);
    ASSERT_TRUE(dateTime.equalsTime(ClockTime{hours{23} + minutes{2} + seconds(3)}));
}

TEST_F(DateTest, ParseZulu)
{
    using namespace date;
    using namespace std::chrono;

    using ClockTime = time_of_day<system_clock::duration>;

    const auto iso8601 = daq::reader::fixupIso8601("2022-09-27T00:02:03Z");
    std::istringstream epochString(iso8601);

    system_clock::time_point timePoint{};
    epochString >> date::parse("%FT%T%z", timePoint);

    DateTime dateTime(timePoint);
    ASSERT_EQ(2022_y / 9 / 27, dateTime.date);
    ASSERT_TRUE(dateTime.equalsTime(ClockTime{hours{0} + minutes{2} + seconds(3)}));
}

TEST_F(DateTest, ParseNoTimeZone)
{
    using namespace date;
    using namespace std::chrono;

    using ClockTime = time_of_day<system_clock::duration>;

    auto iso8601 = daq::reader::fixupIso8601("2022-09-27T00:02:03");
    std::istringstream epochString(iso8601);

    system_clock::time_point timePoint{};
    epochString >> date::parse("%FT%T%z", timePoint);

    DateTime dateTime(timePoint);
    ASSERT_EQ(2022_y / 9 / 27, dateTime.date);
    ASSERT_TRUE(dateTime.equalsTime(ClockTime{hours{0} + minutes{2} + seconds(3)}));
}

TEST_F(DateTest, ParseOnlyDate)
{
    using namespace date;
    using namespace std::chrono;

    using ClockTime = time_of_day<system_clock::duration>;

    auto iso8601 = daq::reader::fixupIso8601("2022-09-27");
    std::istringstream epochString(iso8601);

    system_clock::time_point timePoint{};
    epochString >> date::parse("%FT%T%z", timePoint);

    DateTime dateTime(timePoint);
    ASSERT_EQ(2022_y / 9 / 27, dateTime.date);
    ASSERT_TRUE(dateTime.equalsTime(ClockTime{hours{0} + minutes{0} + seconds(0)}));
}
