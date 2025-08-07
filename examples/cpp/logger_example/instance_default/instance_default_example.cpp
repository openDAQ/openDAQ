/**
 * Default logger state example
 * 
 * Logger in openDAQ is container how sinks (consumers) and loggerComponents (producers) of logging information,
 * by default their construction and destruction is handled automatically.
 * Default sinks and a component get created in the constructor of an instance.
 * Default sinks can also be constructed separately and added to custom sinks.
 *
 * Configuration of default logging state:
 * - Environment variables:
 *      - OPENDAQ_SINK_CONSOLE_LEVEL -> set the level of logging on the console sink or turn it off
 *      - OPENDAQ_SINK_WINDEBUG_LOG_LEVEL -> set the level of logging on the windows debug or turn it off
 *      - OPENDAQ_SINK_FILE_LOG_LEVEL -> set the level displayed level of logging on the environment provided file sink
 *      - OPENDAQ_SINK_FILE_FILE_NAME -> set the name of the environment provided file sink
 * - Any or all of these sinks can be set to active at the same time
 * 
 */

#include "../logger_example_utils/logger_example_utils.h"

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Instance will on constructor create it's own logger that will have default sink and log level settings
    auto instance = Instance();

    // The logger can be directly accessed through Context within the instance
    auto logger = instance.getContext().getLogger();

    // Due to the logger having default settings Info level will be logged on default sinks
    logAtLevel(LogLevel::Info, logger);

    daq::ListPtr<ILoggerSink> myCustomSinks;

    // When adding the default sinks to custom ones, call DefaultSinks function
    // (the same function gets called within the default constructor)
    // Note: The name given in the argument will be superseded by the environment variable
    //        (OPENDAQ_SINK_FILE_FILE_NAME) if it is assigned.
    for (auto sink : DefaultSinks("MyCustomNameOfDefaultSinks"))
        myCustomSinks.pushBack(sink);
}
