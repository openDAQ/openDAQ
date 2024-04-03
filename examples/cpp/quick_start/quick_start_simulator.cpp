/**
 * Part of the openDAQ stand-alone application quick start guide. Starts
 * an openDAQ server on localhost that contains a simulated device.
 */

#include <chrono>
#include <thread>
#include <opendaq/opendaq.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace std::chrono_literals;

    const InstancePtr instance = Instance(MODULE_PATH);

    instance.setRootDevice("daqref://device1");
    instance.addServer("openDAQ StreamingLT", nullptr);
    instance.addStandardServers();

    while(true)
        std::this_thread::sleep_for(100ms);
}
