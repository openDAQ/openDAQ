#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqLoggerTest = testing::Test;

TEST_F(COpendaqLoggerTest, Logger)
{
    daqList* sinks = nullptr;
    daqList_createList(&sinks);

    daqLoggerSink* sink = nullptr;
    daqLoggerSink_createStdOutLoggerSink(&sink);
    daqList_pushBack(sinks, sink);

    daqLogger* logger = nullptr;
    daqLogger_createLogger(&logger, sinks, daqLogLevel::daqLogLevelDebug);

    ASSERT_NE(logger, nullptr);
    daqLogLevel level = daqLogLevel::daqLogLevelOff;
    daqLogger_getLevel(logger, &level);
    ASSERT_EQ(level, daqLogLevel::daqLogLevelDebug);

    daqBaseObject_releaseRef(logger);
    daqBaseObject_releaseRef(sink);
    daqBaseObject_releaseRef(sinks);
}
