#include <gtest/gtest.h>
#include "should_log.h"
#include <opendaq/logger_sink_factory.h>
#include "invalid_logger_sink.h"

using LoggerSinkTest = testing::Test;

using namespace daq;

TEST_F(LoggerSinkTest, CreateStdOutSink)
{
    ASSERT_NO_THROW(StdOutLoggerSink());
}

TEST_F(LoggerSinkTest, CreateStdErrSink)
{
    ASSERT_NO_THROW(StdErrLoggerSink());
}

TEST_F(LoggerSinkTest, CreateRotatingFileLoggerSink)
{
    ASSERT_NO_THROW(RotatingFileLoggerSink("test_log.log", 500, 1));
}

TEST_F(LoggerSinkTest, CreateRotatingFileLoggerSinkUnicode)
{
    ASSERT_NO_THROW(RotatingFileLoggerSink(u8"🍌/test_log.log", 500, 1));
}

TEST_F(LoggerSinkTest, CreateBasicFileLoggerSink)
{
    ASSERT_NO_THROW(BasicFileLoggerSink("test_simple_log.log"));
}

TEST_F(LoggerSinkTest, CreateBasicFileLoggerSinkUnicode)
{
    ASSERT_NO_THROW(BasicFileLoggerSink(R"(🍌🦍/test_simple_log.log)"));
}

#if _WIN32

TEST_F(LoggerSinkTest, CreateWinDebugLoggerSink)
{
    ASSERT_NO_THROW(WinDebugLoggerSink());
}

#endif

TEST_F(LoggerSinkTest, SetPattern)
{
    // https://github.com/gabime/spdlog/wiki/3.-Custom-formatting

    auto sink = StdOutLoggerSink();
    ASSERT_NO_THROW(sink.setPattern("%v"));
}

TEST_F(LoggerSinkTest, DefaultLevel)
{
    auto sink = StdOutLoggerSink();

    ASSERT_EQ(sink.getLevel(), LogLevel::Trace);
}

TEST_F(LoggerSinkTest, SetLevel)
{
    auto sink = StdOutLoggerSink();

    ASSERT_NO_THROW(sink.setLevel(LogLevel::Off));

    ASSERT_EQ(sink.getLevel(), LogLevel::Off);
}

TEST_F(LoggerSinkTest, GetLevelNull)
{
    auto sink = StdOutLoggerSink();

    ASSERT_EQ(sink->getLevel(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(LoggerSinkTest, SetPatternNull)
{
    auto sink = StdOutLoggerSink();

    ASSERT_EQ(sink->setPattern(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(LoggerSinkTest, SetLogOutputNull)
{
    auto sink = StdOutLoggerSink();

    ASSERT_EQ(sink->shouldLog(LogLevel::Info, nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(LoggerSinkTest, EqualsNull)
{
    auto sink = StdOutLoggerSink();

    Bool eq{false};
    sink->equals(nullptr, &eq);
    ASSERT_FALSE(eq);
}

TEST_F(LoggerSinkTest, EqualsOther)
{
    auto sink1 = StdOutLoggerSink();
    auto sink2 = StdOutLoggerSink();

    Bool eq{false};
    sink1->equals(sink2, &eq);
    ASSERT_FALSE(eq);
}

TEST_F(LoggerSinkTest, EqualsInvalid)
{
    auto sink1 = StdOutLoggerSink();

    LoggerSinkPtr sink2;
    ErrCode err = createObject<ILoggerSink, InvalidLoggerSink>(&sink2);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(err));

    Bool eq{false};
    sink1->equals(sink2, &eq);
    ASSERT_FALSE(eq);
}

// Log levels tests

//// Trace

TEST_P(ShouldNotLogTrace, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Trace);

    LogLevel level = GetParam();
    ASSERT_FALSE(sink.shouldLog(level));
}

TEST_P(ShouldLogTrace, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Trace);

    ASSERT_TRUE(sink.shouldLog(GetParam()));
}

///// Debug

TEST_P(ShouldNotLogDebug, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Debug);

    LogLevel level = GetParam();
    ASSERT_FALSE(sink.shouldLog(level));
}

TEST_P(ShouldLogDebug, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Debug);

    ASSERT_TRUE(sink.shouldLog(GetParam()));
}

///// Info

TEST_P(ShouldNotLogInfo, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Info);

    LogLevel level = GetParam();
    ASSERT_FALSE(sink.shouldLog(level));
}

TEST_P(ShouldLogInfo, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Info);

    ASSERT_TRUE(sink.shouldLog(GetParam()));
}

///// Warn

TEST_P(ShouldNotLogWarn, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Warn);

    LogLevel level = GetParam();
    ASSERT_FALSE(sink.shouldLog(level));
}

TEST_P(ShouldLogWarn, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Warn);

    ASSERT_TRUE(sink.shouldLog(GetParam()));
}

///// Error

TEST_P(ShouldNotLogError, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Error);

    LogLevel level = GetParam();
    ASSERT_FALSE(sink.shouldLog(level));
}

TEST_P(ShouldLogError, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Error);

    ASSERT_TRUE(sink.shouldLog(GetParam()));
}

///// Critical

TEST_P(ShouldNotLogCritical, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Critical);

    LogLevel level = GetParam();
    ASSERT_FALSE(sink.shouldLog(level));
}

TEST_P(ShouldLogCritical, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Critical);

    ASSERT_TRUE(sink.shouldLog(GetParam()));
}

///// Off

TEST_P(ShouldNotLogOff, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Off);

    LogLevel level = GetParam();
    ASSERT_FALSE(sink.shouldLog(level));
}

TEST_P(ShouldLogOff, Sink)
{
    auto sink = StdOutLoggerSink();
    sink.setLevel(LogLevel::Off);

    ASSERT_TRUE(sink.shouldLog(GetParam()));
}

// Trace

INSTANTIATE_TEST_SUITE_P(WithX, ShouldLogTrace, testing::Values(LogLevel::Trace), LogLevelToString());

// Debug

INSTANTIATE_TEST_SUITE_P(WithX,
    ShouldNotLogDebug,
    testing::Values(LogLevel::Trace),
    LogLevelToString());

INSTANTIATE_TEST_SUITE_P(WithX,
    ShouldLogDebug,
    testing::Values(LogLevel::Debug, LogLevel::Info, LogLevel::Warn, LogLevel::Error, LogLevel::Critical),
    LogLevelToString());

// Info

INSTANTIATE_TEST_SUITE_P(WithX,
    ShouldNotLogInfo,
    testing::Values(LogLevel::Trace, LogLevel::Debug),
    LogLevelToString());

INSTANTIATE_TEST_SUITE_P(WithX,
    ShouldLogInfo,
    testing::Values(LogLevel::Info, LogLevel::Warn, LogLevel::Error, LogLevel::Critical),
    LogLevelToString());

// Warn

INSTANTIATE_TEST_SUITE_P(WithX,
    ShouldNotLogWarn,
    testing::Values(LogLevel::Trace, LogLevel::Debug, LogLevel::Info),
    LogLevelToString());

INSTANTIATE_TEST_SUITE_P(WithX,
                        ShouldLogWarn,
                        testing::Values(LogLevel::Warn, LogLevel::Error, LogLevel::Critical),
                        LogLevelToString());

// Error

INSTANTIATE_TEST_SUITE_P(WithX,
    ShouldNotLogError,
    testing::Values(LogLevel::Trace, LogLevel::Debug, LogLevel::Info, LogLevel::Warn),
    LogLevelToString());

INSTANTIATE_TEST_SUITE_P(WithX,
                        ShouldLogError,
                        testing::Values(LogLevel::Error, LogLevel::Critical),
                        LogLevelToString());

// Critical

INSTANTIATE_TEST_SUITE_P(WithX,
    ShouldNotLogCritical,
    testing::Values(LogLevel::Trace, LogLevel::Debug, LogLevel::Info, LogLevel::Warn, LogLevel::Error),
    LogLevelToString());

INSTANTIATE_TEST_SUITE_P(WithX,
                        ShouldLogCritical,
                        testing::Values(LogLevel::Critical),
                        LogLevelToString());

// Off

INSTANTIATE_TEST_SUITE_P(
    WithX,
    ShouldNotLogOff,
    testing::Values(LogLevel::Trace, LogLevel::Debug, LogLevel::Info, LogLevel::Warn, LogLevel::Error, LogLevel::Critical),
    LogLevelToString());

INSTANTIATE_TEST_SUITE_P(WithX,
                        ShouldLogOff,
                        testing::Values(LogLevel::Off),
                        LogLevelToString());
