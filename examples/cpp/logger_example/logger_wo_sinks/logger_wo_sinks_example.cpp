/**
 * Default Logger creation via Instace or its assosiated builder
 */

#include <opendaq/custom_log.h>
#include <opendaq/opendaq.h>
#include <chrono>
#include <iostream>

using namespace daq;

void doExample(const InstancePtr instance)
{
    auto logger = instance.getContext().getLogger();
    // When using Logger macros, a variable named loggerComponent of a LoggerComponentPtr must be present to ensure correct logging
    // The following is an example of how to create a new loggerComponent
    auto loggerComponent = logger.getOrAddComponent("Example_of_component_logging");

    // To access these lower levels of logging, you need to change the level at which
    // the openDAQ library is buld in CMake, otherwise these levels are inaccessible (aka will not be seen in any of the sinks)
    LOG_T("Trace level.");
    LOG_D("Debug level.");
    // This line below will be printed only in Debug configuration
    LOG_I("Info level.");
    // From below here everything will be printed in release configuration
    LOG_W("Warning level.");
    LOG_E("Error level.");
    LOG_C("Critical level");
}

int main(int /*argc*/, const char* /*argv*/[])
{
    auto instance1 = Instance();
    auto instance2 = InstanceBuilder().build();

    doExample(instance1);
    doExample(instance2);
}
