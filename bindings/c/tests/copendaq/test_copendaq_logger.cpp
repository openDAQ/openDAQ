#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqLoggerTest = testing::Test;

TEST_F(COpendaqLoggerTest, Logger)
{
    List* sinks = nullptr;
    List_createList(&sinks);

    LoggerSink* sink = nullptr;
    LoggerSink_createStdOutLoggerSink(&sink);
    List_pushBack(sinks, sink);

    Logger* logger = nullptr;
    Logger_createLogger(&logger, sinks, LogLevel::LogLevelDebug);

    ASSERT_NE(logger, nullptr);
    LogLevel level = LogLevel::LogLevelOff;
    Logger_getLevel(logger, &level);
    ASSERT_EQ(level, LogLevel::LogLevelDebug);

    BaseObject_releaseRef(logger);
    BaseObject_releaseRef(sink);
    BaseObject_releaseRef(sinks);
}
