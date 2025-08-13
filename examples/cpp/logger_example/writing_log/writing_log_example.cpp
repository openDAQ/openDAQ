/**
 * Writing log messages
 * 
 * In openDAQ the recommended way of logging is with the use of macros that allow easy access to every level of logging.
 * They are all made in a pattern of LOG_X, where X represents the initial of the log level
 * (T for trace, D for debug, I for info, W for warning, E for error, C for critical).
 *
 * The level from which the macros are defined is dictated by two variables in CMake configuration of openDAQ.
 * The two variables are:
 *  - OPENDAQ_LOG_LEVEL_DEBUG for the debug configuration and
 *  - OPENDAQ_LOG_LEVEL_RELEASE for the release configuration of openDAQ.
 * Macros corresponding to or above (in terms of severity) the level provided in the variable are will log the message
 * contained in the call of the macro itself. Ones below will not log anything instead.
 * (OPENDAQ_LOG_LEVEL_DEBUG is set to "Warn", therefore LOG_W, LOG_E and LOG_C will pass on the messages normally, while
 * LOG_T, LOG_D and LOG_I will not do anything).
 * ANY CHANGES TO the CMake VARIABLES REQUIRE the REBUILD of the ENTIRE openDAQ project.
 */

#include "../logger_example_utils/logger_example_utils.h"

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    auto instance = Instance();

    auto logger = instance.getContext().getLogger();
    // When using Logger macros, a variable named loggerComponent of a LoggerComponentPtr must be present to ensure correct logging
    // The following is an example of how to create a new loggerComponent
    auto loggerComponent = logger.getOrAddComponent("Example_of_component_logging");

    // This logs at trace level
    LOG_T("Trace level.");

    // This logs at debug level
    LOG_D("Debug level.");

    // This logs at info level
    LOG_I("Info level.");

    // This logs at warning level
    LOG_W("Warning level.");

    // This logs at error level
    LOG_E("Error level.");

    // This logs at the critical level
    LOG_C("Critical level");
}
