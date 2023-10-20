#include <opendaq/instance_ptr.h>
#include <opendaq/opendaq.h>

#include <thread>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    const InstancePtr instance = Instance();
    const ListPtr<IDeviceInfo> discoveredDevices = instance.getAvailableDevices();
    const ListPtr<IFunctionBlockType> availableFunctionBlocks = instance.getAvailableFunctionBlockTypes();

    DevicePtr device;
    for (auto deviceInfo : discoveredDevices)
        if (deviceInfo.getSerialNumber() == "dev_ser_0")
            device = instance.addDevice(deviceInfo.getConnectionString());

    if (!device.assigned())
        return 0;

    FunctionBlockPtr renderer;
    FunctionBlockPtr statistics;

    for (auto fbInfo : availableFunctionBlocks)
    {
        if (fbInfo.getId() == "ref_fb_module_renderer")
            renderer = instance.addFunctionBlock(fbInfo.getId());
        if (fbInfo.getId() == "ref_fb_module_averager")
            statistics = instance.addFunctionBlock(fbInfo.getId());
    }

    if (!renderer.assigned() || !statistics.assigned())
        return 0;

    const auto sineChannel = device.getChannels()[0];
    const auto sineSignal = sineChannel.getSignals()[0];
    sineChannel.setPropertyValue("Frequency", 5);
    sineChannel.setPropertyValue("Amplitude", 5);

    renderer.setPropertyValue("Duration", 5);
    statistics.setPropertyValue("BlockSize", 20);

    renderer.getInputPorts()[0].connect(sineSignal);

    const auto averagedSine = statistics.getSignals()[0];
    renderer.getInputPorts()[1].connect(averagedSine);
    statistics.getInputPorts()[0].connect(sineSignal);

    double ampl_step = 0.1;
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        const double ampl = sineChannel.getPropertyValue("Amplitude");
        if (9.95 < ampl || ampl < 1.05)
            ampl_step *= -1;
        sineChannel.setPropertyValue("Amplitude", ampl + ampl_step);
    }
}
