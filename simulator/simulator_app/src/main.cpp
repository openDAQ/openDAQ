#include <chrono>
#include <thread>
#include <opendaq/opendaq.h>

using namespace daq;
int main(int /*argc*/, const char* /*argv*/[])
{
    using namespace std::chrono_literals;

    const StringPtr configPath = "/tmp/opendaq-config.json";
    const ConfigProviderPtr configProvider = JsonConfigProvider(configPath);
    const InstanceBuilderPtr instanceBuilder = InstanceBuilder().addConfigProvider(configProvider);
    const InstancePtr instance = InstanceFromBuilder(instanceBuilder);

    instance.addStandardServers();

    while (true)
    {
        std::this_thread::sleep_for(100ms);
    }

    return 0;
}
