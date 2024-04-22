#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using HowToConfigureInstance = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_instance.adoc

// TODO: These just check if the code runs, they don't check correctness

TEST_F(HowToConfigureInstance, InstanceBuilder)
{
    InstanceBuilderPtr instanceBuilder = InstanceBuilder();
    InstancePtr instance = instanceBuilder.build();
}

TEST_F(HowToConfigureInstance, InstanceBuilderLogger1)
{
    InstanceBuilderPtr instanceBuilder = InstanceBuilder();
    instanceBuilder.setGlobalLogLevel(LogLevel::Warn);
    InstancePtr instance = instanceBuilder.build();
}

TEST_F(HowToConfigureInstance, InstanceBuilderLogger2)
{
    InstanceBuilderPtr instanceBuilder = InstanceBuilder()
                                             .addLoggerSink(StdOutLoggerSink())
                                             .setSinkLogLevel(BasicFileLoggerSink("logfile.log"), LogLevel::Error)
                                             .setComponentLogLevel("Instance", LogLevel::Warn)
                                             .setComponentLogLevel("Some Module", LogLevel::Off);
    InstancePtr instance = instanceBuilder.build();
}

TEST_F(HowToConfigureInstance, InstanceBuilderLogger3)
{
    ListPtr<ILoggerSink> sinks = List<ILoggerSink>(StdOutLoggerSink());
    LoggerPtr logger = LoggerWithSinks(sinks, LogLevel::Warn);
    InstanceBuilderPtr instanceBuilder = InstanceBuilder().setLogger(logger);
    InstancePtr instance = instanceBuilder.build();
}

/*
// Throws "The specified path does not exist."
TEST_F(HowToConfigureInstance, InstanceBuilderModuleManager1)
{
    InstanceBuilderPtr instanceBuilder = InstanceBuilder().setModulePath("/path/to/modules");
    InstancePtr instance = instanceBuilder.build();
}
*/

/*
// Throws "The specified path does not exist."
TEST_F(HowToConfigureInstance, InstanceBuilderModuleManager2)
{
    ModuleManagerPtr moduleManager = ModuleManager("/path/to/modules");
    InstanceBuilderPtr instanceBuilder = InstanceBuilder().setModuleManager(moduleManager);
    InstancePtr instance = instanceBuilder.build();
}
*/
TEST_F(HowToConfigureInstance, InstanceBuilderScheduler1)
{
    InstanceBuilderPtr instanceBuilder = InstanceBuilder().setSchedulerWorkerNum(2);
    InstancePtr instance = instanceBuilder.build();
}

TEST_F(HowToConfigureInstance, InstanceBuilderScheduler2)
{
    LoggerPtr logger = Logger();
    SchedulerPtr scheduler = Scheduler(logger, 4);
    InstanceBuilderPtr instanceBuilder = InstanceBuilder().setScheduler(scheduler);
    InstancePtr instance = instanceBuilder.build();
}

TEST_F(HowToConfigureInstance, InstanceBuilderDefaultRootDevice)
{
    DeviceInfoConfigPtr defaultRootDeviceInfo = DeviceInfo("daqref://defaultRootDevice");
    defaultRootDeviceInfo.setSerialNumber("ABCD-0000-0000-0000");
    InstanceBuilderPtr instanceBuilder =
        InstanceBuilder().setDefaultRootDeviceInfo(defaultRootDeviceInfo).setDefaultRootDeviceLocalId("defaultRootDeviceLocalId");
    InstancePtr instance = instanceBuilder.build();

    assert(instance.getInfo() == defaultRootDeviceInfo);
}

TEST_F(HowToConfigureInstance, InstanceBuilderRootDevice)
{
    InstanceBuilderPtr instanceBuilder = InstanceBuilder().setRootDevice("daqref://device0");
    InstancePtr instance = instanceBuilder.build();
}

END_NAMESPACE_OPENDAQ
