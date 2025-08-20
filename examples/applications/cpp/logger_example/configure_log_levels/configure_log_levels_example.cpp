/**
 * Configure log levels
 * 
 * Level of logging can be adjusted on several different levels (at the level of sinks, loggers,
 * builders and the logger component level). When adjusting the expected level of severity displayed,
 * only elevation of severity is possible (Info -> Warn, not the other way around)
 * Object in sequence of creation keep the limitations of logging from prior objects (sinks -> logger -> loggerComponent)
 * (if sinks have their levels set to warn, no logs with info severity will be displayed in them,
 * even if the level on the logger components that they belong to are set to info).
 */

#include "../logger_example_utils/logger_example_utils.h"

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // We can set the level of logging on the logger itself
    auto logger = Logger("battleOfIssus.txt");
    logger.setLevel(LogLevel::Warn);

    logAtLevel(LogLevel::Info, logger);
    logAtLevel(LogLevel::Warn, logger);

    // Each individual sink can have their log levels adjusted
    auto sink = BasicFileLoggerSink("battleOfGranicus.txt");
    sink.setLevel(LogLevel::Debug);

    // The default level of all logger components can be set via GlobalLogLevel in the builder of the Instance object
    auto instanceBuilder = InstanceBuilder().addLoggerSink(sink).setGlobalLogLevel(LogLevel::Warn);
    auto instance = instanceBuilder.build();

    // Not visible because instanceBuilder sets the level to Warning
    logAtLevel(LogLevel::Info, instance.getContext().getLogger());

    // Not visible due to loggerComponent sets level to error
    logAtLevel(LogLevel::Warn, instance.getContext().getLogger());

    // Visible because the Error is lowest level allowed 
    logAtLevel(LogLevel::Error, instance.getContext().getLogger());

    // Highest level defined (LogLevel cannot be elevated above this one (only way to disable it is to disable logging alltogether))
    logAtLevel(LogLevel::Critical, instance.getContext().getLogger());

    // Additional logger components can be added to the logger after its creation and can have their logging level adjusted
    auto loggerComponent = instance.getContext().getLogger().getOrAddComponent("Checker");
    loggerComponent.setLevel(LogLevel::Error);

    // The first of the macros underneath logs at the Info level and the second one logs at the error level.
    // More information on them is available in the Writing log messages example
    LOG_I("Invisible.");
    LOG_E("Visible.");

}
