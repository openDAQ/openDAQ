/**
 * Logger with sinks
 * 
 * Logger can be created on it's own that allows the user to exert greater control over the locations of sinks
 * and their type. OpenDAQ supports two different types of file sink (Basic and Rotating).
 * Rotating file sink is limited in size (limited size of file and number of files created)
 * Basic file sink is not limited (aside from system limitations (space in the computer)).
 *
 * Sinks can be added to either Loggers constructed outside of an Instance or
 * they can be added directly to the the instance builder.
 *
 * Because Logger is part of the Context that gets constructed in the creation of the Instance object,
 * the Context itself can also be created on its own. Logger itself can therefore be added to the constructor of Context.
 * 
 */
#include "../logger_example_utils/logger_example_utils.h"

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Logger can be created by simply adding a name of the sink file
    auto logger = Logger("text_burrow.txt");

    // You can add the logger to the instance builder
    auto instance = InstanceBuilder().setLogger(logger).build();

    logAtLevel(LogLevel::Info, logger);

    // Basic file sink only needs the name of the file
    auto sinks2 = BasicFileLoggerSink("text_hive.txt");

    // Logger can also be created with a list of sinks (the parameter can be a list of a single sink)
    auto logger2 = LoggerWithSinks(List<ILoggerSink>(sinks2));

    // Logger is one of the building blocks of Context and can be either added through constructor (below)
    // or into the builder (similar procedure to above with InstanceBuilder)
    auto context = Context(nullptr, logger2, nullptr, nullptr);

    auto instance2 = InstanceBuilder().setContext(context).build();

    logAtLevel(LogLevel::Info, logger2);

    // Rotating file sink needs the name of the file, the maximum size and the amount of files
    auto sinks3 = RotatingFileLoggerSink("text_warren.txt", 200, 5);

    // Sinks can also be added to directly to the Instance, without adding them to logger previously
    auto instance3 = InstanceBuilder().addLoggerSink(sinks3).build();

    logAtLevel(LogLevel::Info, instance3.getContext().getLogger());
}
