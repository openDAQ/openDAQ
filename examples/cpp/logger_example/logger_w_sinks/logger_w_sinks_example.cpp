/**
 * Example of a logger that has externally defined sinks
 * 
 */

#include "../logger_example_utils/logger_example_utils.h"

using namespace daq;

// (note on sinks)
ListPtr<LoggerSinkPtr> setupSinks(/* LogLevel level*/)
{
    auto mySinks = DefaultSinks();
    mySinks.pushBack(BasicFileLoggerSink("./one.txt"));
    mySinks.pushBack(BasicFileLoggerSink("./two.txt"));
    return mySinks;
}

// (note on logging the stuff necessary)
void doExample(const InstancePtr& instance)
{
    auto logger = instance.getContext().getLogger();

    auto loggerComponent = logger.getOrAddComponent("Example_of_component_logging");

    // (note on why these logging levels are used)
    LOG_W("Warning level.");
    LOG_E("Error level.");
    LOG_C("Critical level");
}

int main(int /*argc*/, const char* /*argv*/[])
{
    const auto logLevel = LogLevel::Critical;

    auto mySinks = setupSinks();

    // Way 1
    // (note)::First way of doing this is to create a context and putting a logger into a custom instance
    auto logger1 = LoggerWithSinks(mySinks, logLevel);
    logger1.setLevel(logLevel);
    auto context1 = Context(Scheduler(logger1), logger1, TypeManager(), ModuleManager(MODULE_PATH));
    auto instance1 = InstanceCustom(context1, "");

    // Way 2
    // (note)::Another way is to assign a logger to a builder
    auto logger2 = LoggerWithSinks(mySinks, logLevel);
    logger2.setLevel(logLevel);
    auto instance2 = InstanceBuilder().setLogger(logger2).build();

    // Way 3
    // (note)::We can also add sinks one by one to the builder
    auto builder = InstanceBuilder();
    for (const auto& sink : setupSinks())
        builder.addLoggerSink(sink);
    builder.setGlobalLogLevel(logLevel);
    auto instance3 = builder.build();

    // Way 4
    // (note)::We can also add a logger via context into the builder
    auto logger4 = LoggerWithSinks(mySinks, logLevel);
    logger4.setLevel(logLevel);
    auto context2 = Context(Scheduler(logger4), logger4, TypeManager(), ModuleManager(MODULE_PATH));
    auto instance4 = InstanceBuilder().setContext(context2).build();

    // (note)::In the example we will create a custom logger component that will the check what
    // level of logging is available and being written to a file
    doExample(instance1);
    doExample(instance2);
    doExample(instance3);
    doExample(instance4);
}
