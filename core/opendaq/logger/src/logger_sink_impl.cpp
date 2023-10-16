#include <opendaq/logger_sink_impl.h>

#include <coretypes/coretype_traits.h>
#include <coretypes/impl.h>
#include <coretypes/ctutils.h>
#include <coretypes/coretype_traits.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TSinkType>
LoggerSinkImpl<TSinkType>::LoggerSinkImpl()
    : LoggerSinkBase(std::make_shared<TSinkType>())
{
}

template <typename TSinkType>
LoggerSinkImpl<TSinkType>::LoggerSinkImpl(SinkPtr&& sink)
    : LoggerSinkBase(std::move(sink))
{
}

static auto toFileName(IString* fileName)
{
#if _WIN32
    return CoreTypeHelper<std::wstring>::ToWString(fileName);
#else
    return toStdString(fileName);
#endif
}

LoggerSinkImpl<spdlog::sinks::rotating_file_sink_mt>::LoggerSinkImpl(IString* fileName, SizeT maxFileSize, SizeT maxFiles)
    : LoggerSinkImpl<void>(
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            toFileName(fileName),
            maxFileSize,
            maxFiles
        )
      )
{
}

LoggerSinkImpl<spdlog::sinks::basic_file_sink_mt>::LoggerSinkImpl(IString* fileName)
    : LoggerSinkImpl<void>(std::make_shared<spdlog::sinks::basic_file_sink_mt>(toFileName(fileName)))
{
}

LoggerSinkBase::LoggerSinkBase(SinkPtr&& sink)
    : sink(sink)
{
    this->sink->set_pattern("[tid: %t]%+");
}

ErrCode LoggerSinkBase::setLevel(LogLevel level)
{
    this->sink->set_level(static_cast<spdlog::level::level_enum>(level));
    return OPENDAQ_SUCCESS;
}

ErrCode LoggerSinkBase::getLevel(LogLevel* level)
{
    if (level == nullptr)
    {
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot save return value to a null pointer.");
    }

    *level = static_cast<LogLevel>(sink->level());
    return OPENDAQ_SUCCESS;
}

ErrCode LoggerSinkBase::shouldLog(LogLevel level, Bool* willLog)
{
    if (willLog == nullptr)
    {
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot save return value to a null pointer.");
    }

    *willLog = sink->should_log(static_cast<spdlog::level::level_enum>(level));
    return OPENDAQ_SUCCESS;
}

ErrCode LoggerSinkBase::setPattern(IString* pattern)
{
    if (pattern == nullptr)
    {
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "The pattern can not be null.");
    }

    try
    {
        this->sink->set_pattern(toStdString(pattern));
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }
    catch (const std::exception& e)
    {
        return errorFromException(e);
    }

    return OPENDAQ_SUCCESS;
}

ErrCode LoggerSinkBase::flush()
{
    try
    {
        this->sink->flush();
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }
    catch (const std::exception& e)
    {
        return errorFromException(e);
    }

    return OPENDAQ_SUCCESS;
}

ErrCode LoggerSinkBase::equals(IBaseObject* other, Bool* equals) const
{
    if (equals == nullptr)
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

    *equals = false;
    if (other == nullptr)
        return OPENDAQ_SUCCESS;

    if (auto* otherSink = dynamic_cast<LoggerSinkBase*>(other))
    {
        *equals = otherSink->sink == this->sink;
    }
    return OPENDAQ_SUCCESS;
}

LoggerSinkBase::SinkPtr LoggerSinkBase::getSinkImpl() const
{
    return sink;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY,
    LoggerSinkImpl<spdlog::sinks::stderr_color_sink_mt>,
    ILoggerSink,
    createStdErrLoggerSink
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY,
    LoggerSinkImpl<spdlog::sinks::stdout_color_sink_mt>,
    ILoggerSink,
    createStdOutLoggerSink
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY,
    LoggerSinkImpl<spdlog::sinks::rotating_file_sink_mt>,
    ILoggerSink,
    createRotatingFileLoggerSink,
    IString*,
    fileName,
    SizeT,
    maxFileByteSize,
    SizeT,
    maxFiles
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY,
    LoggerSinkImpl<spdlog::sinks::basic_file_sink_mt>,
    ILoggerSink,
    createBasicFileLoggerSink,
    IString*,
    fileName
)

#ifdef _WIN32

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY,
    LoggerSinkImpl<spdlog::sinks::msvc_sink_mt>,
    ILoggerSink,
    createWinDebugLoggerSink
)

#endif

END_NAMESPACE_OPENDAQ
