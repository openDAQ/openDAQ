// Helpers the MultiReader2 harness files share: tick conversion, element access, the error-counting sink and the watchdog

#pragma once
#include <gtest/gtest.h>
#include <opendaq/logger_sink_base_private.h>
#include <opendaq/opendaq.h>
#include <spdlog/details/log_msg.h>

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

namespace daq::mr2test::detail
{

constexpr int64_t T0 = 1767225600;     // 2026-01-01T00:00:00Z
constexpr int64_t PsPerSecond = 1000000000000;
// Full-second ISO 8601 origin to seconds since the Unix epoch; nullopt for anything else
inline std::optional<int64_t> parseOrigin(const std::string& origin)
{
    int y, mo, d, h, mi, s;
    char z = 0;
    if (std::sscanf(origin.c_str(), "%d-%d-%dT%d:%d:%d%c", &y, &mo, &d, &h, &mi, &s, &z) != 7 || z != 'Z')
        return std::nullopt;
    // days from civil, Howard Hinnant's algorithm
    y -= mo <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const int64_t yoe = y - era * 400;
    const int64_t doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const int64_t days = era * 146097 + doe - 719468;
    return days * 86400 + h * 3600 + mi * 60 + s;
}

// A domain the harness can convert ticks through: int64_t, scalar, linear with integer delta and start, full-second origin
struct TickDomain
{
    int64_t originSeconds = T0;
    int64_t resNum = 1;
    int64_t resDen = 1000;
    int64_t delta = 1;
    int64_t start = 0;

    static std::optional<TickDomain> from(const DataDescriptorPtr& desc)
    {
        if (!desc.assigned() || desc.getSampleType() != SampleType::Int64 || desc.getDimensions().getCount() != 0)
            return std::nullopt;
        const auto rule = desc.getRule();
        if (!rule.assigned() || rule.getType() != DataRuleType::Linear)
            return std::nullopt;
        const auto res = desc.getTickResolution();
        const auto origin = desc.getOrigin();
        if (!res.assigned() || !origin.assigned() || !desc.getUnit().assigned() || desc.getUnit().getSymbol() != "s")
            return std::nullopt;
        const auto seconds = parseOrigin(origin.toStdString());
        const NumberPtr deltaNum = rule.getParameters().get("delta");
        const NumberPtr startNum = rule.getParameters().get("start");
        if (!seconds || deltaNum.getFloatValue() != std::floor(deltaNum.getFloatValue()) || deltaNum.getIntValue() <= 0 ||
            startNum.getFloatValue() != std::floor(startNum.getFloatValue()))
            return std::nullopt;
        TickDomain d{*seconds, res.getNumerator(), res.getDenominator(), deltaNum.getIntValue(), startNum.getIntValue()};
        if (PsPerSecond % d.resDen != 0)
            return std::nullopt;
        return d;
    }

    int64_t resolutionPs() const
    {
        return resNum * (PsPerSecond / resDen);
    }

    // Time of a tick as whole seconds since the epoch plus picoseconds
    std::pair<int64_t, int64_t> timeOf(int64_t tick) const
    {
        int64_t q = tick / resDen;
        int64_t r = tick % resDen;
        if (r < 0)
        {
            r += resDen;
            q -= 1;
        }
        int64_t seconds = q * resNum + (r * resNum) / resDen;
        int64_t ps = ((r * resNum) % resDen) * (PsPerSecond / resDen);
        return {seconds + originSeconds, ps};
    }

    // The tick at or before a time
    int64_t tickAt(std::pair<int64_t, int64_t> time) const
    {
        const int64_t resPs = resolutionPs();
        const int64_t rel = time.first - originSeconds;
        int64_t ps = time.second;
        int64_t ticks = rel * (PsPerSecond / resPs);
        ticks += ps / resPs;
        return ticks;
    }
};

inline SizeT elementCount(const DataDescriptorPtr& desc)
{
    SizeT n = 1;
    for (const auto& dim : desc.getDimensions())
        n *= dim.getSize();
    return n;
}

inline SizeT typeSize(SampleType type)
{
    return getSampleSize(type);
}

inline double readElement(const uint8_t* src, SampleType type)
{
    switch (type)
    {
        case SampleType::Float32:
        {
            float v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::Float64:
        {
            double v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::Int8:
        {
            int8_t v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::UInt8:
        {
            uint8_t v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::Int16:
        {
            int16_t v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::UInt16:
        {
            uint16_t v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::Int32:
        {
            int32_t v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::UInt32:
        {
            uint32_t v;
            std::memcpy(&v, src, sizeof v);
            return v;
        }
        case SampleType::Int64:
        {
            int64_t v;
            std::memcpy(&v, src, sizeof v);
            return static_cast<double>(v);
        }
        case SampleType::UInt64:
        {
            uint64_t v;
            std::memcpy(&v, src, sizeof v);
            return static_cast<double>(v);
        }
        default:
            throw std::runtime_error("harness: unsupported element type");
    }
}

inline std::string errorName(ErrCode error)
{
    switch (error)
    {
        case OPENDAQ_SUCCESS:
            return "SUCCESS";
        case OPENDAQ_ERR_NOTIMPLEMENTED:
            return "NOTIMPLEMENTED";
        case OPENDAQ_ERR_INVALIDPARAMETER:
            return "INVALIDPARAMETER";
        case OPENDAQ_ERR_ARGUMENT_NULL:
            return "ARGUMENT_NULL";
        case OPENDAQ_ERR_INVALIDSTATE:
            return "INVALIDSTATE";
        case OPENDAQ_ERR_ALREADYEXISTS:
            return "ALREADYEXISTS";
        default:
            break;
    }
    std::ostringstream s;
    s << "0x" << std::hex << error;
    return s.str();
}

// An spdlog sink that counts error entries, since openDAQ has no counting sink. Only the sink's pure virtuals are
// implemented: spdlog is compiled into the openDAQ library, and its non-virtual members are not linked here.
class ErrorCountingSink : public spdlog::sinks::sink
{
public:
    void log(const spdlog::details::log_msg& msg) override
    {
        // Only the reader's entries: an error of a port or a signal must not stand in for the one a row expects
        if (msg.level < spdlog::level::err || msg.logger_name != spdlog::string_view_t("MultiReader2"))
            return;
        {
            std::scoped_lock lock(mutex);
            ++errors;
        }
        cv.notify_all();
    }
    void flush() override
    {
    }
    void set_pattern(const std::string&) override
    {
    }
    void set_formatter(std::unique_ptr<spdlog::formatter>) override
    {
    }

    SizeT count()
    {
        std::scoped_lock lock(mutex);
        return errors;
    }

    SizeT waitForAtLeast(SizeT n)
    {
        std::unique_lock lock(mutex);
        cv.wait_for(lock, std::chrono::seconds(2), [&] { return errors >= n; });
        return errors;
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    SizeT errors = 0;
};

class ErrorCountingSinkImpl : public ImplementationOf<ILoggerSink, ILoggerSinkBasePrivate>
{
public:
    std::shared_ptr<ErrorCountingSink> sink = std::make_shared<ErrorCountingSink>();

    ErrCode INTERFACE_FUNC setLevel(LogLevel) override
    {
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC getLevel(LogLevel* level) override
    {
        *level = LogLevel::Error;
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC shouldLog(LogLevel level, Bool* willLog) override
    {
        *willLog = level >= LogLevel::Error;
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC setPattern(IString*) override
    {
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC flush() override
    {
        return OPENDAQ_SUCCESS;
    }
    ErrCode INTERFACE_FUNC getSinkImpl(SinkPtr* sinkImpl) override
    {
        *sinkImpl = sink;
        return OPENDAQ_SUCCESS;
    }
};

inline ErrorCountingSink& countingSink(const LoggerSinkPtr& sink)
{
    return *static_cast<ErrorCountingSinkImpl*>(sink.getObject())->sink;
}

// INV7: a run that has not finished within the limit is a deadlock; the process exits rather than hang the suite, naming
// the test and the seed so the run can be replayed (MR2_SEED). A deadlocked test cannot be failed and unwound
#ifdef NDEBUG
constexpr int watchdogSeconds = 10;
#else
constexpr int watchdogSeconds = 60;  // debug builds run several times slower
#endif

class Watchdog
{
public:
    explicit Watchdog(const std::atomic<uint64_t>& seed)
        : thread(
              [this, &seed]
              {
                  std::unique_lock lock(mutex);
                  if (!cv.wait_for(lock, std::chrono::seconds(watchdogSeconds), [this] { return done; }))
                  {
                      const auto* info = testing::UnitTest::GetInstance()->current_test_info();
                      std::fprintf(stderr,
                                   "INV7: %s.%s did not finish within %d s (seed %llu)\n",
                                   info ? info->test_suite_name() : "?",
                                   info ? info->name() : "?",
                                   watchdogSeconds,
                                   static_cast<unsigned long long>(seed.load()));
                      std::fflush(stderr);
                      std::_Exit(3);
                  }
              })
    {
    }
    ~Watchdog()
    {
        {
            std::scoped_lock lock(mutex);
            done = true;
        }
        cv.notify_all();
        thread.join();
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    std::thread thread;
};

inline bool isScalarNumeric(const DataDescriptorPtr& desc)
{
    if (!desc.assigned() || desc.getDimensions().getCount() != 0)
        return false;
    const auto type = desc.getSampleType();
    return type >= SampleType::Float32 && type <= SampleType::Int64;
}

inline std::string unitOf(const DataDescriptorPtr& desc)
{
    return desc.assigned() && desc.getUnit().assigned() ? desc.getUnit().getSymbol().toStdString() : "";
}

}
