/**
 * Adds a reference device that outputs sine waves. Its noisy output signal is averagedSine
 * by an statistics function block. Both the noisy and averaged sine waves are rendered with
 * the renderer function block for 10s.
 */

#include <chrono>
#include <thread>
#include <opendaq/opendaq.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules at MODULE_PATH
    const InstancePtr instance = Instance(MODULE_PATH);

    // Add a reference device and set it as root
    auto device = instance.addDevice("daqref://device0");

    FunctionBlockPtr parquet = instance.addFunctionBlock("file_writer_module_parquet");

     // Get channel and signal of reference device
    const auto firstChannel = device.getChannels()[0];
    const auto firstSignal = firstChannel.getSignals()[0];

    const auto secondChannel = device.getChannels()[1];
    const auto secondSignal = secondChannel.getSignals()[0];

    parquet.getInputPorts()[0].connect(firstSignal);
    parquet.getInputPorts()[1].connect(secondSignal);

    // You can also manipulate the path. By default is stores in the execution directory.
    //parquet.setPropertyValue("Path", "xxx");
    parquet.setPropertyValue("FileName", "myData");
    parquet.setPropertyValue("WriteBatchCylceInSec", 10);
    parquet.setPropertyValue("RecordingActive", true);

    // Process and render data for 10s, modulating the amplitude
    double ampl_step = 0.1;
    for (int i = 0; i < 400; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        const double ampl = firstChannel.getPropertyValue("Amplitude");
        if (9.95 < ampl || ampl < 3.05)
            ampl_step *= -1;
        firstChannel.setPropertyValue("Amplitude", ampl + ampl_step);
    }
    parquet.setPropertyValue("RecordingActive", false);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    return 0;
}
