/*
 * Copyright 2022-2024 openDAQ d.o.o.
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

#include <date/date.h>
#include <chrono>
#include <ostream>

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
        if (epoch.find('T') == std::string::npos)
        {
            // If no time, assume Midnight UTC
            epoch += "T00:00:00+00:00";
        }
        else if (epoch[epoch.size() - 1] == 'Z')
        {
            // If time-zone marked as "Zulu" (UTC) replace with offset
            epoch = epoch.erase(epoch.size() - 1) + "+00:00";
        }
        else if (epoch.find('+') == std::string::npos)
        {
            // If not time-zone offset assume UTC
            epoch += "+00:00";
        }

        return epoch;
    }

    inline auto parseEpoch(const std::string& origin)
    {
        std::chrono::system_clock::time_point epoch;

        std::istringstream epochString(fixupIso8601(origin));
        epochString >> date::parse("%FT%T%z", epoch);

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
        const auto resolution = dataDescriptor.getTickResolution().simplify();

        NumberPtr delta = 1;
        auto rule = dataDescriptor.getRule();
        if (rule.assigned() && rule.getType() != DataRuleType::Linear)
        {
            throw NotSupportedException("Only signals with implicit linear-rule as a domain are supported.");
        }
        else
        {
            delta = rule.getParameters()["delta"];
        }

        double sampleRate = static_cast<double>(resolution.getDenominator()) / (static_cast<double>(resolution.getNumerator()) * delta.getFloatValue());
        if (sampleRate != static_cast<double>(static_cast<int64_t>(sampleRate)))
        {
            throw NotSupportedException("Only signals with integral sample-rate are supported but found signal with {} Hz", sampleRate);
        }
        return static_cast<int64_t>(sampleRate);
    }

    inline std::string getErrorMessage()
    {
        std::string errorMessage;

        ErrorInfoPtr errorInfo;
        daqGetErrorInfo(&errorInfo);
        if (errorInfo.assigned())
        {
            StringPtr message;
            errorInfo->getMessage(&message);

            if (message.assigned())
                errorMessage = message.toStdString();
        }

        return errorMessage;
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
