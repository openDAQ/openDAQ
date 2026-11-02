/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <coretypes/formatter.h>
#include <coretypes/ratio_factory.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/sample_type_traits.h>

#include <date/date.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <ostream>
#include <type_traits>

BEGIN_NAMESPACE_OPENDAQ

namespace reader
{
    inline std::ostream& operator<<(std::ostream& os, std::chrono::system_clock::time_point timePoint)
    {
        using namespace date;

        auto const dp = date::floor<days>(timePoint);
        os << year_month_day(dp) << ' ' << make_time(timePoint - dp);

        return os;
    }

    template <typename T>
    std::ostream& operator<<(std::ostream& os, const Complex_Number<T>& value)
    {
        os << "[ " << value.real << ", " << value.imaginary << "i ]";
        return os;
    }

    template <typename T>
    std::ostream& operator<<(std::ostream& os, const RangeType<T>& value)
    {
        os << "[ " << value.start << ", " << value.end << " ]";
        return os;
    }

    /*!
     * @brief Converts one sample value to the read type. A real read as an integer rounds half away from zero and
     * saturates, and NaN reads as 0; an integer out of the read type's range saturates; a finite Float64 beyond the
     * Float32 range saturates. Any other conversion is a plain cast.
     */
    template <typename D, typename S>
    D convertSample(S s)
    {
        if constexpr (!std::is_arithmetic_v<D> || !std::is_arithmetic_v<S> || std::is_same_v<D, S>)
            return static_cast<D>(s);
        else if constexpr (std::is_same_v<D, float> && std::is_same_v<S, double>)
        {
            // NaN and infinity carry over
            if (std::isfinite(s))
                return static_cast<float>(std::clamp<double>(s, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()));
            return static_cast<float>(s);
        }
        else if constexpr (std::is_floating_point_v<D>)
            return static_cast<D>(s);
        else if constexpr (std::is_floating_point_v<S>)
        {
            if (std::isnan(s))
                return 0;
            const S r = std::round(s);
            if (r <= static_cast<S>(std::numeric_limits<D>::min()))
                return std::numeric_limits<D>::min();
            if (r >= static_cast<S>(std::numeric_limits<D>::max()))
                return std::numeric_limits<D>::max();
            return static_cast<D>(r);
        }
        else
        {
            if constexpr (std::is_signed_v<S>)
                if (s < 0)
                {
                    if constexpr (std::is_unsigned_v<D>)
                        return 0;
                    else if (static_cast<std::int64_t>(s) < static_cast<std::int64_t>(std::numeric_limits<D>::min()))
                        return std::numeric_limits<D>::min();
                    else
                        return static_cast<D>(s);
                }
            if (static_cast<std::uint64_t>(s) > static_cast<std::uint64_t>(std::numeric_limits<D>::max()))
                return std::numeric_limits<D>::max();
            return static_cast<D>(s);
        }
    }

    inline std::string timePointString(std::chrono::system_clock::time_point timePoint)
    {
        std::stringstream ss;
        ss << timePoint;

        return ss.str();
    }

    /*!
     * @brief Parser expects the string in "YYYY-mm-ddTHH:MM:SS+HH:MM" so coerce
     * other valid options to this format.
     */
    inline std::string fixupIso8601(std::string epoch)
    {
        const auto time = epoch.find('T');
        // If no time, assume Midnight UTC; a reduced date ("YYYY" or "YYYY-MM") starts on its first day
        if (time == std::string::npos)
        {
            if (epoch.size() == 4)
                epoch += "-01";
            if (epoch.size() == 7)
                epoch += "-01";
            return epoch + "T00:00:00+0000";
        }
        // If time-zone marked as "Zulu" (UTC) replace with offset
        if (epoch.back() == 'Z')
            return epoch.erase(epoch.size() - 1) + "+0000";
        // If not time-zone offset assume UTC
        const auto zone = epoch.find_first_of("+-", time);
        if (zone == std::string::npos)
            return epoch + "+0000";
        // %z reads the minutes of an offset only without a colon
        if (epoch.size() == zone + 6 && epoch[zone + 3] == ':')
            epoch.erase(zone + 3, 1);
        return epoch;
    }

    inline auto parseEpoch(const std::string& origin, bool* ok = nullptr)
    {
        std::chrono::system_clock::time_point epoch;

        std::istringstream epochString(fixupIso8601(origin));
        date::from_stream(epochString, "%FT%T%z", epoch);
        if (ok)
            *ok = !epochString.fail();

        return epoch;
    }

    namespace detail
    {
        template <typename T, typename = std::void_t<>>
        struct SysTime
        {
            template <typename RoundTo = std::chrono::system_clock::duration>
            static auto ToSysTime(T value, std::chrono::system_clock::time_point epoch, const RatioPtr& resolution)
            {
                using namespace std::chrono;
                using Seconds = duration<double>;

                auto offset = Seconds((resolution.getNumerator() * value) / static_cast<double>(resolution.getDenominator()));
                return round<RoundTo>(epoch + offset);
            }
        };

        template <typename T>
        struct SysTime<T, std::enable_if_t<IsTemplateOf<T, RangeType>::value>>
        {
            template <typename RoundTo = std::chrono::system_clock::duration>
            static auto ToSysTime(T /*value*/, std::chrono::system_clock::time_point /*epoch*/, const RatioPtr& /*resolution*/)
            {
                return std::chrono::system_clock::time_point{};
            }
        };

        template <typename T>
        struct SysTime<T, std::enable_if_t<IsTemplateOf<T, Complex_Number>::value>>
        {
            template <typename RoundTo = std::chrono::system_clock::duration>
            static auto ToSysTime(T /*value*/, std::chrono::system_clock::time_point /*epoch*/, const RatioPtr& /*resolution*/)
            {
                return std::chrono::system_clock::time_point{};
            }
        };
    }

    template <typename T, typename RoundTo = std::chrono::system_clock::duration>
    auto toSysTime(T value, std::chrono::system_clock::time_point epoch, const RatioPtr& resolution)
    {
        return detail::SysTime<T>:: template ToSysTime<RoundTo>(value, epoch, resolution);
    }

    inline std::int64_t getSampleRate(const DataDescriptorPtr& dataDescriptor)
    {
        const auto tickResolution = dataDescriptor.getTickResolution();
        const auto resolution = tickResolution.assigned() ? tickResolution.simplify() : Ratio(1, 1);

        NumberPtr delta = 1;
        auto rule = dataDescriptor.getRule();
        if (rule.assigned() && rule.getType() != DataRuleType::Linear)
        {
            DAQ_THROW_EXCEPTION(NotSupportedException, "Only signals with implicit linear-rule as a domain are supported.");
        }
        else
        {
            delta = rule.getParameters()["delta"];
        }

        double sampleRate = static_cast<double>(resolution.getDenominator()) / (static_cast<double>(resolution.getNumerator()) * delta.getFloatValue());
        if (sampleRate != static_cast<double>(static_cast<int64_t>(sampleRate)))
        {
            DAQ_THROW_EXCEPTION(NotSupportedException, "Only signals with integral sample-rate are supported but found signal with {} Hz", sampleRate);
        }
        return static_cast<int64_t>(sampleRate);
    }

    inline int setEnvironmentVariable(const std::string& variable)
    {
#if defined(_MSC_VER)
        return _putenv(variable.c_str());
#else
        return putenv((char*) variable.c_str());
#endif
    }
}

END_NAMESPACE_OPENDAQ

template <typename T>
struct fmt::formatter<daq::Complex_Number<T>> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const daq::Complex_Number<T>& c, FormatContext& ctx)
    {
        return format_to(ctx.out(), "({}, {})", c.real, c.imaginary);
    }
};

template <typename T>
struct fmt::formatter<daq::RangeType<T>> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const daq::RangeType<T>& c, FormatContext& ctx)
    {
        return format_to(ctx.out(), "[{}, {}]", c.start, c.end);
    }
};

template <>
struct fmt::formatter<daq::RatioPtr> : fmt::formatter<daq::ObjectPtr<daq::IRatio>>
{
};

template <>
struct fmt::formatter<std::chrono::system_clock::time_point> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(std::chrono::system_clock::time_point c, FormatContext& ctx) const
    {
        return fmt::formatter<std::string>::format(daq::reader::timePointString(c), ctx);
    }
};
