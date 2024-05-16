#include <opendaq/opendaq.h>
#include <chrono>
#include <thread>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace std::chrono_literals;

    const InstancePtr instance = Instance("");
    instance.setRootDevice("daqref://device1");
    instance.addFunctionBlock("ref_fb_module_trigger");
    instance.addServer("openDAQ LT Streaming", nullptr);
    instance.addStandardServers();

    while (true)
        std::this_thread::sleep_for(100ms);
}
