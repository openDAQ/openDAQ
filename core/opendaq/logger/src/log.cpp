#include <opendaq/log.h>
#include <opendaq/logger_sink_factory.h>
#include <opendaq/logger_impl.h>
#include <spdlog/spdlog.h>
#include <opendaq/logger_factory.h>
#include <opendaq/logger_sink_impl.h>
#include <spdlog/async.h>
#include <sstream>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

[[maybe_unused]]
static StringPtr getLogFileName()
{
    char timeString[16];
    auto time = std::time(nullptr);

    std::tm tm{};
#if defined(__STDC_SECURE_LIB__) || defined(__STDC_LIB_EXT1__)
    localtime_s(&tm, &time);
#else
    tm = *localtime(&time);
#endif

    std::strftime(timeString, 16, "%m-%d-%y_%H%M%S", &tm);

    std::stringstream ss;
    ss << "daqcore_" << timeString << ".log";

    return ss.str();
}

static_assert(LogLevel::Trace == (LogLevel) spdlog::level::level_enum::trace);
static_assert(LogLevel::Debug == (LogLevel) spdlog::level::level_enum::debug);
static_assert(LogLevel::Info == (LogLevel) spdlog::level::level_enum::info);
static_assert(LogLevel::Warn == (LogLevel) spdlog::level::level_enum::warn);
static_assert(LogLevel::Error == (LogLevel) spdlog::level::level_enum::err);
static_assert(LogLevel::Critical == (LogLevel) spdlog::level::level_enum::critical);

END_NAMESPACE_OPENDAQ
