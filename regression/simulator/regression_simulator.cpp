#include <opendaq/opendaq.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

using namespace daq;
using namespace std::chrono_literals;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Set up the simulator
    const InstancePtr instance = Instance("");
    instance.setRootDevice("daqref://device1");
    instance.addFunctionBlock("ref_fb_module_trigger");
    instance.addServer("openDAQ OpcUa", nullptr);
    instance.addServer("openDAQ Native Streaming", nullptr);
    instance.addServer("openDAQ LT Streaming", nullptr);

    // Create an empty file named "ready" to let regression test suite know
    // the simulator is up and running and ready for tests
    std::ofstream out;
    out.open("ready", std::ios::out);
    out.close();

    // Github Action will delete the "ready" file after
    // the tests for one protocol are done, which means
    // we can then gracefully shut down the simulator
    while (std::filesystem::exists("ready"))
    {
        std::this_thread::sleep_for(100ms);
    }

    return 0;
}
