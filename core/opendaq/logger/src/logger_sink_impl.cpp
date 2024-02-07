#include <opendaq/logger_sink_impl.h>

#include <coretypes/coretype_traits.h>
#include <coretypes/impl.h>
#include <coretypes/ctutils.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <opendaq/logger_sink_last_message_impl.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename TSinkType, typename... Interfaces>
LoggerSinkImpl<TSinkType, Interfaces...>::LoggerSinkImpl()
    : Super(std::make_shared<TSinkType>())
{
}

template <typename TSinkType, typename... Interfaces>
LoggerSinkImpl<TSinkType, Interfaces...>::LoggerSinkImpl(typename Super::SinkPtr&& sink)
    : Super(std::move(sink))
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

template <typename... Interfaces>
LoggerSinkBase<Interfaces...>::LoggerSinkBase(SinkPtr&& sink)
    : sink(sink)
{
    this->sink->set_pattern("[tid: %t]%+");
}

template <typename... Interfaces>
ErrCode LoggerSinkBase<Interfaces...>::setLevel(LogLevel level)
{
    this->sink->set_level(static_cast<spdlog::level::level_enum>(level));
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode LoggerSinkBase<Interfaces...>::getLevel(LogLevel* level)
{
    if (level == nullptr)
    {
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot save return value to a null pointer.");
    }

    *level = static_cast<LogLevel>(sink->level());
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode LoggerSinkBase<Interfaces...>::shouldLog(LogLevel level, Bool* willLog)
{
    if (willLog == nullptr)
    {
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot save return value to a null pointer.");
    }

    *willLog = sink->should_log(static_cast<spdlog::level::level_enum>(level));
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode LoggerSinkBase<Interfaces...>::setPattern(IString* pattern)
{
    if (pattern == nullptr)
    {
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "The pattern can not be null.");
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

template <typename... Interfaces>
ErrCode LoggerSinkBase<Interfaces...>::flush()
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

template <typename... Interfaces>
ErrCode LoggerSinkBase<Interfaces...>::equals(IBaseObject* other, Bool* equals) const
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
template <typename... Interfaces>
ErrCode LoggerSinkBase<Interfaces...>::getSinkImpl(typename LoggerSinkBase<Interfaces...>::SinkPtr* sinkImp)
{
    if (sinkImp == nullptr)
       return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "SinkImp out-parameter must not be null");
    *sinkImp = sink;
    return OPENDAQ_SUCCESS;
}

ErrCode LoggerSinkLastMessageImpl::getLastMessage(IString** lastMessage)
{
    SinkType* sink = static_cast<SinkType*>(this->sink.get());
    if (sink)
        return sink->getLastMessage(lastMessage);
    return OPENDAQ_IGNORED;
}
ErrCode LoggerSinkLastMessageImpl::waitForMessage(SizeT timeoutMs, Bool* success)
{
    SinkType* sink = static_cast<SinkType*>(this->sink.get());
    if (sink)
        return sink->waitForMessage(timeoutMs, success);
    return OPENDAQ_IGNORED;
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

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY,
    LoggerSinkLastMessageImpl,
    ILoggerSink,
    createLastMessageLoggerSink
)

END_NAMESPACE_OPENDAQ
