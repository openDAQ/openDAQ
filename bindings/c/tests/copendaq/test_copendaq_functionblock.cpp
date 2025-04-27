#include <copendaq.h>

#include <gtest/gtest.h>

class COpendaqFunctionBlockTest : public testing::Test
{
    void SetUp() override
    {
        InstanceBuilder* builder = nullptr;
        InstanceBuilder_createInstanceBuilder(&builder);
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
        String* modulePath = nullptr;
        String_createString(&modulePath, ".");
        InstanceBuilder_addModulePath(builder, modulePath);
        BaseObject_releaseRef(modulePath);

        InstanceBuilder_build(builder, &instance);
        BaseObject_releaseRef(builder);
        Instance_getRootDevice(instance, &dev);
    }

    void TearDown() override
    {
        BaseObject_releaseRef(instance);
        BaseObject_releaseRef(dev);
    }

protected:
    Instance* instance = nullptr;
    Device* dev = nullptr;
};

TEST_F(COpendaqFunctionBlockTest, FunctionBlock)
{
    FunctionBlock* functionBlock = nullptr;
    String* typeId = nullptr;
    String_createString(&typeId, "RefFBModuleFFT");
    Device_addFunctionBlock(dev, &functionBlock, typeId, nullptr);
    ASSERT_NE(functionBlock, nullptr);
    BaseObject_releaseRef(typeId);

    FunctionBlockType* functionBlockType = nullptr;
    FunctionBlock_getFunctionBlockType(functionBlock, &functionBlockType);
    ASSERT_NE(functionBlockType, nullptr);

    BaseObject_releaseRef(functionBlockType);
    BaseObject_releaseRef(functionBlock);
}
