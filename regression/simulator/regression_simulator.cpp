#include <opendaq/opendaq.h>
#include <chrono>
#include <fstream>
#include <thread>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace std::chrono_literals;

    const InstancePtr instance = Instance("");
    instance.setRootDevice("daqref://device1");
    instance.addFunctionBlock("ref_fb_module_trigger");
    instance.addServer("openDAQ OpcUa", nullptr);
    instance.addServer("openDAQ Native Streaming", nullptr);
    instance.addServer("openDAQ LT Streaming", nullptr);

    // Create an empty file named "ready" to let regression test suite know
    // the Simulator is up and running and ready for tests
    std::fstream fs;
    fs.open("ready", std::ios::out);
    fs.close();
    // Github Action will wait until "ready" file exists,
    // then start the regression tests
    // This file is later deleted in GitHub Action via shell
    // so we can test another prococol

    while (true)
        std::this_thread::sleep_for(100ms);
}
