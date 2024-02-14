#include <opendaq/logger_component_impl.h>
#include <coretypes/impl.h>

#include <opendaq/logger_sink_base_private_ptr.h>
#include <opendaq/logger_thread_pool_private.h>
#include <opendaq/logger_thread_pool_factory.h>

#include <functional>
#include <utility>

#include <spdlog/async.h>
#include <spdlog/spdlog.h>

BEGIN_NAMESPACE_OPENDAQ

static ILoggerThreadPoolPrivate::ThreadPoolPtr getThreadPool(const LoggerThreadPoolPtr& threadPool)
{
    ILoggerThreadPoolPrivate::ThreadPoolPtr threadPoolImpl;
    ErrCode err = threadPool.asPtr<ILoggerThreadPoolPrivate>()->getThreadPoolImpl(&threadPoolImpl);
    checkErrorInfo(err);

    return threadPoolImpl;
}

LoggerComponentImpl::LoggerComponentImpl(const StringPtr& name, const ListPtr<ILoggerSink>& sinks,
                                         const LoggerThreadPoolPtr& threadPool, LogLevel level)
#ifdef OPENDAQ_LOGGER_SYNC
    : spdlogLogger(std::make_shared<LoggerComponentType>(
        name,
        std::initializer_list<spdlog::sink_ptr>())
    )
#else
    : spdlogLogger(std::make_shared<LoggerComponentType>(
        name,
        std::initializer_list<spdlog::sink_ptr>(),
        getThreadPool(threadPool),
        spdlog::async_overflow_policy::block)
    )
#endif
    , threadPool(std::move(threadPool))
{
    spdlogLogger->set_level(static_cast<spdlog::level::level_enum>(getLogLevelFromParam(level)));

    if (!sinks.assigned())
    {
        throw ArgumentNullException("Sinks List must not be null.");
    }
    for (const ObjectPtr<ILoggerSink>& sink : sinks)
    {
        if(!sink.assigned())
        {
            throw ArgumentNullException("Sink must not be null");
        }
        auto sinkPtr = sink.asPtrOrNull<ILoggerSinkBasePrivate>();
        if (sinkPtr == nullptr)
        {
            throw InvalidTypeException("Sink must have valid type");
        }
        spdlogLogger->sinks().push_back(sinkPtr.getSinkImpl());
    }
}

ErrCode LoggerComponentImpl::getName(IString** name)
{
    if (name == nullptr)
    {
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Can not return by a null pointer.");
    }

    StringPtr nameStr(spdlogLogger->name());

    *name = nameStr.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LoggerComponentImpl::setLevel(LogLevel level)
{
    spdlogLogger->set_level(static_cast<spdlog::level::level_enum>(getLogLevelFromParam(level)));
    return OPENDAQ_SUCCESS;
}

ErrCode LoggerComponentImpl::getLevel(LogLevel* level)
{
    if (level == nullptr)
    {
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Can not return by a null pointer.");
    }
    *level = (LogLevel) spdlogLogger->level();

    return OPENDAQ_SUCCESS;
}

ErrCode LoggerComponentImpl::logMessage(SourceLocation location, ConstCharPtr msg, LogLevel level)
{
    spdlogLogger->log(
        spdlog::source_loc(location.fileName, location.line, location.funcName), static_cast<spdlog::level::level_enum>(level), msg);
    return OPENDAQ_SUCCESS;
}

ErrCode LoggerComponentImpl::setPattern(IString* pattern)
{
    if (pattern == nullptr)
    {
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Pattern can not be null.");
    }

    spdlogLogger->set_pattern(toStdString(pattern));
    return OPENDAQ_SUCCESS;
}

ErrCode LoggerComponentImpl::shouldLog(LogLevel level, Bool* willLog)
{
    if (willLog == nullptr)
    {
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Can not return by a null pointer.");
    }

    *willLog = spdlogLogger->should_log(static_cast<spdlog::level::level_enum>(level));
    return OPENDAQ_SUCCESS;
}

ErrCode LoggerComponentImpl::flush()
{
    spdlogLogger->flush();
    return OPENDAQ_SUCCESS;
}

ErrCode LoggerComponentImpl::flushOnLevel(LogLevel level)
{
    spdlogLogger->flush_on(static_cast<spdlog::level::level_enum>(level));
    return OPENDAQ_SUCCESS;
}

ErrCode LoggerComponentImpl::toString(CharPtr* str)
{
    return daqDuplicateCharPtr(spdlogLogger->name().data(), str);
}

LogLevel LoggerComponentImpl::getDefaultLogLevel()
{
    int logLevel = -1;
    auto& loggerNameStr = spdlogLogger->name();

    std::string uppercaseLoggerNameStr;
    std::transform(loggerNameStr.begin(), loggerNameStr.end(), std::back_inserter(uppercaseLoggerNameStr), ::toupper);

    std::string componentEnv = "OPENDAQ_" + uppercaseLoggerNameStr + "_LOG_LEVEL";
    char* env = std::getenv(componentEnv.c_str());
    if (env != nullptr)
    {
        try
        {
            logLevel = std::stoi(env);
        }
        catch (...)
        {
            logLevel = -1;
        }
    }

    if (logLevel < 0)
    {
        env = std::getenv("OPENDAQ_LOG_LEVEL");
        if (env != nullptr)
        {
            try
            {
                logLevel = std::stoi(env);
            }
            catch (...)
            {
                logLevel = -1;
            }
        }
    }

    if (logLevel < 0 || logLevel > OPENDAQ_LOG_LEVEL_OFF)
        return static_cast<LogLevel>(OPENDAQ_LOG_LEVEL);

    return static_cast<LogLevel>(logLevel);
}

LogLevel LoggerComponentImpl::getLogLevelFromParam(LogLevel logLevel)
{
    return logLevel == LogLevel::Default ? getDefaultLogLevel() : logLevel;
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, LoggerComponent,
    IString*, name,
    IList*, sinks,
    ILoggerThreadPool*, threadPool,
    LogLevel, level)

END_NAMESPACE_OPENDAQ
