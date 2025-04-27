#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqOpendaqTest = testing::Test;

TEST_F(COpendaqOpendaqTest, ConfigProvider)
{
    ConfigProvider* configProvider = nullptr;
    ConfigProvider_createEnvConfigProvider(&configProvider);
    ASSERT_NE(configProvider, nullptr);
    BaseObject_releaseRef(configProvider);
}

TEST_F(COpendaqOpendaqTest, InstanceAndBuilder)
{
    InstanceBuilder* builder = nullptr;
    InstanceBuilder_createInstanceBuilder(&builder);
    ASSERT_NE(builder, nullptr);
    InstanceBuilder_setGlobalLogLevel(builder, LogLevel::LogLevelDebug);
    String* component = nullptr;
    String_createString(&component, "Instance");
    InstanceBuilder_setComponentLogLevel(builder, component, LogLevel::LogLevelError);
    BaseObject_releaseRef(component);
    LoggerSink* sink = nullptr;
    LoggerSink_createStdErrLoggerSink(&sink);
    InstanceBuilder_addLoggerSink(builder, sink);
    BaseObject_releaseRef(sink);
    InstanceBuilder_setSchedulerWorkerNum(builder, 1);

    Instance* instance = nullptr;
    InstanceBuilder_build(builder, &instance);
    ASSERT_NE(instance, nullptr);

    BaseObject_releaseRef(builder);
    BaseObject_releaseRef(instance);
}
