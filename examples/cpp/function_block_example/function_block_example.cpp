/**
 * Adds a reference device that outputs sine waves. Its noisy output signal is averagedSine
 * by an averager function block. Both the noisy and averaged sine waves are rendered with
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

    // Add averager and renderer function block
    FunctionBlockPtr averager = instance.addFunctionBlock("ref_fb_module_averager");
    FunctionBlockPtr renderer = instance.addFunctionBlock("ref_fb_module_renderer");

    // Set renderer to draw 2.5s of data
    renderer.setPropertyValue("Duration", 2.5);

    // Get channel and signal of reference device
    const auto sineChannel = device.getChannels()[0];
    const auto sineSignal = sineChannel.getSignals()[0];
    
    // Add noise to the sine wave
    sineChannel.setPropertyValue("NoiseAmplitude", 1);

    // Connect the signals to the renderer and averager
    averager.getInputPorts()[0].connect(sineSignal);
    const auto averagedSine = averager.getSignalsRecursive()[0];
    
    renderer.getInputPorts()[0].connect(sineSignal);
    renderer.getInputPorts()[1].connect(averagedSine);

    // Process and render data for 10s, modulating the amplitude
    double ampl_step = 0.1;
    for (int i = 0; i < 400; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        const double ampl = sineChannel.getPropertyValue("Amplitude");
        if (9.95 < ampl || ampl < 3.05)
            ampl_step *= -1;
        sineChannel.setPropertyValue("Amplitude", ampl + ampl_step);
    }

    return 0;
}
