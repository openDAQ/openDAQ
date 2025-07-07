/**
 * Empty example
 */

#include <opendaq/opendaq.h>

using namespace daq;


int main(int /*argc*/, const char* /*argv*/[])
{
    // Set your wav file destination here
    std::string filePath = "C:/Users/jakob/OneDrive/Desktop/example.wav";

    // Create an Instance, loading modules at MODULE_PATH
    const InstancePtr instance = Instance(MODULE_PATH);

    daq::FunctionBlockPtr reader = instance.addFunctionBlock("AudioDeviceModuleWavReader");

    daq::FunctionBlockPtr writerFb = instance.addFunctionBlock("AudioDeviceModuleWavWriter");
    daq::RecorderPtr writerRe = writerFb;

    writerFb.getInputPorts()[0].connect(reader.getSignals()[0]);

    reader.setPropertyValue("FilePath", filePath);
    writerFb.setPropertyValue("FileName", "output.wav");

    reader.setPropertyValue("Reading", true);
    writerRe.startRecording();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    reader.setPropertyValue("Reading", false);
    writerRe.stopRecording();
}
