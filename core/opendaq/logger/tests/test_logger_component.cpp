#ifdef OPENDAQ_LOG_LEVEL
#undef OPENDAQ_LOG_LEVEL
#endif

#define OPENDAQ_LOG_LEVEL OPENDAQ_LOG_LEVEL_TRACE

#include <gtest/gtest.h>
#include <opendaq/logger_sink_factory.h>
#include <opendaq/logger_component_factory.h>
#include <opendaq/logger_thread_pool_factory.h>

#include <opendaq/log.h>
#include <opendaq/custom_log.h>
#include "should_log.h"
#include "invalid_logger_sink.h"
#include <coretypes/listobject_factory.h>
#include <coretypes/impl.h>
#include <opendaq/logger_sink_ptr.h>

#include <chrono>
#include <thread>

using namespace daq;

class LoggerComponentTest : public testing::Test
{
public:
    void TearDown() override
    {
        using namespace std::chrono_literals;

        // Wait for Async logger to flush
        std::this_thread::sleep_for(100ms);
    }
};

TEST_F(LoggerComponentTest, Create)
{
    ASSERT_NO_THROW(LoggerComponent("test"));
}

TEST_F(LoggerComponentTest, CreateWithParameters)
{
    ASSERT_NO_THROW(LoggerComponent("test", DefaultSinks(), LoggerThreadPool(), LogLevel::Trace));
}

TEST_F(LoggerComponentTest, CreateWithoutSinks)
{
    ASSERT_THROW(LoggerComponent("test", {}), ArgumentNullException);
}

TEST_F(LoggerComponentTest, CreateWithInvalidSink)
{
    LoggerSinkPtr sink;
    ErrCode err = createObject<ILoggerSink, InvalidLoggerSink>(&sink);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(err));

    ASSERT_THROW(LoggerComponent("test", {sink}), InvalidTypeException);
}

TEST_F(LoggerComponentTest, CreateWithSinkStdErr)
{
    auto sink = StdErrLoggerSink();

    ASSERT_NO_THROW(LoggerComponent("test", {sink}));
}

TEST_F(LoggerComponentTest, CreateWithSinkStdOut)
{
    auto sink = StdOutLoggerSink();

    ASSERT_NO_THROW(LoggerComponent("test", {sink}));
}

TEST_F(LoggerComponentTest, CreateWithSinkRotatingFile)
{
    auto sink = RotatingFileLoggerSink("test_log.log", 500, 1);

    ASSERT_NO_THROW(LoggerComponent("test", {sink}));
}

TEST_F(LoggerComponentTest, CreateWithMultipleSinks)
{
    auto sink = RotatingFileLoggerSink("test_log.log", 500, 1);

    ASSERT_NO_THROW(LoggerComponent("test", {sink, StdOutLoggerSink(), StdErrLoggerSink()}));
}

TEST_F(LoggerComponentTest, GetName)
{
    auto testLoggerComponent = LoggerComponent("test");
    ASSERT_STREQ(testLoggerComponent.getName().getCharPtr(), "test");
}

TEST_F(LoggerComponentTest, GetNameNull)
{
    auto testLoggerComponent = LoggerComponent("test");
    ASSERT_EQ(testLoggerComponent->getName(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(LoggerComponentTest, SetPattern)
{
    auto loggerComponent = LoggerComponent("test");
    ASSERT_NO_THROW(loggerComponent.setPattern("%v"));
}

TEST_F(LoggerComponentTest, SetPatternNull)
{
    auto loggerComponent = LoggerComponent("test");
    ASSERT_EQ(loggerComponent->setPattern(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(LoggerComponentTest, SimpleLog)
{
    auto loggerComponent = LoggerComponent("testSimple", {StdErrLoggerSink()},
                                           LoggerThreadPool(), LogLevel::Trace);

    loggerComponent.logMessage(SourceLocation{__FILE__, __LINE__, OPENDAQ_CURRENT_FUNCTION},
                                "test", LogLevel::Trace);
    loggerComponent.logMessage(SourceLocation{__FILE__, __LINE__, OPENDAQ_CURRENT_FUNCTION},
                               "test", LogLevel::Debug);
    loggerComponent.logMessage(SourceLocation{__FILE__, __LINE__, OPENDAQ_CURRENT_FUNCTION},
                               "test", LogLevel::Info);
    loggerComponent.logMessage(SourceLocation{__FILE__, __LINE__, OPENDAQ_CURRENT_FUNCTION},
                               "test", LogLevel::Warn);
    loggerComponent.logMessage(SourceLocation{__FILE__, __LINE__, OPENDAQ_CURRENT_FUNCTION},
                               "test", LogLevel::Error);
    loggerComponent.logMessage(SourceLocation{__FILE__, __LINE__, OPENDAQ_CURRENT_FUNCTION},
                               "test", LogLevel::Critical);

    loggerComponent.flush();
}

TEST_F(LoggerComponentTest, LogMacro)
{
    auto loggerComponent = LoggerComponent("testMacro", {StdErrLoggerSink()},
                                           LoggerThreadPool(), LogLevel::Trace);

    DAQLOG_T(loggerComponent, "trace");
    DAQLOG_D(loggerComponent, "debug");
    DAQLOG_I(loggerComponent, "info");
    DAQLOG_W(loggerComponent, "warning");
    DAQLOG_E(loggerComponent, "error");
    DAQLOG_C(loggerComponent, "critical");

    DAQLOGF_T(loggerComponent, "trace {}", 1)
    DAQLOGF_D(loggerComponent, "debug {}", 1)
    DAQLOGF_I(loggerComponent, "info {}", 1)
    DAQLOGF_W(loggerComponent, "warning {}", 1)
    DAQLOGF_E(loggerComponent, "error {}", 1)
    DAQLOGF_C(loggerComponent, "critical {}", 1)

    LOGP_T("trace")
    LOGP_D("debug")
    LOGP_I("info")
    LOGP_W("warning")
    LOGP_E("error")
    LOGP_C("critical")

    LOG_T("trace")
    LOG_D("debug")
    LOG_I("info")
    LOG_W("warning")
    LOG_E("error")
    LOG_C("critical")

    LOG_T("trace {}", 1)
    LOG_D("debug {}", 1)
    LOG_I("info {}", 1)
    LOG_W("warning {}", 1)
    LOG_E("error {}", 1)
    LOG_C("critical {}", 1)

    loggerComponent.flush();
}

TEST_F(LoggerComponentTest, LogFromThread)
{
    auto loggerComponent = LoggerComponent("testThread", {StdErrLoggerSink()},
                                           LoggerThreadPool(), LogLevel::Trace);

    auto func = [loggerComponent](int threadNumber){
        LOG_I("log from thread number {}", threadNumber)
    };

    func(0);

    std::thread thread1(func, 1);
    thread1.join();

    std::thread thread2(func, 2);
    thread2.join();

    func(0);

    loggerComponent.flush();
}

TEST_F(LoggerComponentTest, GetLevelNull)
{
    auto loggerComponent = LoggerComponent("test");

    ASSERT_EQ(loggerComponent->getLevel(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(LoggerComponentTest, SetLevel)
{
    auto loggerComponent = LoggerComponent("test");

    loggerComponent.setLevel(LogLevel::Trace);
    ASSERT_EQ(loggerComponent.getLevel(), LogLevel::Trace);

    loggerComponent.setLevel(LogLevel::Debug);
    ASSERT_EQ(loggerComponent.getLevel(), LogLevel::Debug);

    loggerComponent.setLevel(LogLevel::Info);
    ASSERT_EQ(loggerComponent.getLevel(), LogLevel::Info);

    loggerComponent.setLevel(LogLevel::Warn);
    ASSERT_EQ(loggerComponent.getLevel(), LogLevel::Warn);

    loggerComponent.setLevel(LogLevel::Error);
    ASSERT_EQ(loggerComponent.getLevel(), LogLevel::Error);

    loggerComponent.setLevel(LogLevel::Critical);
    ASSERT_EQ(loggerComponent.getLevel(), LogLevel::Critical);

    loggerComponent.setLevel(LogLevel::Off);
    ASSERT_EQ(loggerComponent.getLevel(), LogLevel::Off);
}



TEST_F(LoggerComponentTest, DefaultLevel)
{
    auto testLoggerComponent = LoggerComponent("level");

    ASSERT_EQ(testLoggerComponent.getLevel(), (LogLevel) OPENDAQ_LOG_LEVEL);
}

TEST_F(LoggerComponentTest, ShouldLogNullOutput)
{
    auto loggerComponent = LoggerComponent("test");
    ErrCode err = loggerComponent->shouldLog(LogLevel::Critical, nullptr);

    ASSERT_EQ(err, OPENDAQ_ERR_ARGUMENT_NULL);
}

// Log levels tests

//// Trace

TEST_P(ShouldNotLogTrace, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Trace);

    LogLevel level = GetParam();
    ASSERT_FALSE(loggerComponent.shouldLog(level));
}

TEST_P(ShouldLogTrace, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Trace);

    ASSERT_TRUE(loggerComponent.shouldLog(GetParam()));
}

///// Debug

TEST_P(ShouldNotLogDebug, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Debug);

    ASSERT_FALSE(loggerComponent.shouldLog(GetParam()));
}

TEST_P(ShouldLogDebug, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Debug);

    ASSERT_TRUE(loggerComponent.shouldLog(GetParam()));
}

///// Info

TEST_P(ShouldNotLogInfo, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Info);

    LogLevel level = GetParam();
    ASSERT_FALSE(loggerComponent.shouldLog(level));
}

TEST_P(ShouldLogInfo, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Info);

    ASSERT_TRUE(loggerComponent.shouldLog(GetParam()));
}

///// Warn

TEST_P(ShouldNotLogWarn, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Warn);

    LogLevel level = GetParam();
    ASSERT_FALSE(loggerComponent.shouldLog(level));
}

TEST_P(ShouldLogWarn, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Warn);

    ASSERT_TRUE(loggerComponent.shouldLog(GetParam()));
}

///// Error

TEST_P(ShouldNotLogError, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Error);

    LogLevel level = GetParam();
    ASSERT_FALSE(loggerComponent.shouldLog(level));
}

TEST_P(ShouldLogError, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Error);

    ASSERT_TRUE(loggerComponent.shouldLog(GetParam()));
}

///// Critical

TEST_P(ShouldNotLogCritical, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Critical);

    LogLevel level = GetParam();
    ASSERT_FALSE(loggerComponent.shouldLog(level));
}

TEST_P(ShouldLogCritical, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Critical);

    ASSERT_TRUE(loggerComponent.shouldLog(GetParam()));
}

///// Off

TEST_P(ShouldNotLogOff, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Off);

    LogLevel level = GetParam();
    ASSERT_FALSE(loggerComponent.shouldLog(level));
}

TEST_P(ShouldLogOff, Component)
{
    auto loggerComponent = LoggerComponent("test");
    loggerComponent.setLevel(LogLevel::Off);

    ASSERT_TRUE(loggerComponent.shouldLog(GetParam()));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ShouldNotLogTrace);
