/**
 * Empty example
 */

#include <iostream>
#include <opendaq/opendaq.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules at MODULE_PATH
    const InstancePtr instance = daq::InstanceBuilder().setModulePath(MODULE_PATH).setUsingSchedulerMainLoop(true).build(); 



    // ------------------- Find and connect to a simulator device ------------------- //
    const auto availableDevices = instance.getAvailableDevices();
    daq::DevicePtr device;
    for (const auto& deviceInfo : availableDevices)
    {
        if (deviceInfo.getName() == "Reference device simulator")
        {
            device = instance.addDevice(deviceInfo.getConnectionString());
            break;
        }
    }

    if (!device.assigned())
    {
        std::cout << "No input device found!" << std::endl;
        return 1;
    }

    std::cout << device.getInfo().getName() << std::endl;

    daq::ChannelPtr inputChannel = device.getChannels()[0];
    daq::SignalPtr inputSignal = inputChannel.getSignals()[0];
    // ------------------------------------------------------------------------------ //



    // ------ Find the licensing module, authenticate it, and load the license ------ //
    const auto modules = instance.getModuleManager().getModules();
    auto itFound = std::find_if(modules.begin(),
                                modules.end(),
                                [](const ModulePtr& module) { return module.getModuleInfo().getName() == "LicensingModule"; });

    if (itFound == modules.end())
    {
        std::cerr << "The 'LicensingModule' was not found!" << std::endl << "Press any key to continue...";
        std::cin.get();
        return 1;
    }

    ModulePtr licensingModulePtr = *itFound;
    Bool succeeded = false;

    PropertyObjectPtr authenticationConfig = PropertyObject();
    authenticationConfig.addProperty(StringProperty(
        "AuthenticationKeyPath", "C:\\Users\\jakob\\source\\openDAQ\\examples\\cpp\\licensing_example\\authentication_key.txt"));
    succeeded = licensingModulePtr.authenticate(authenticationConfig);

    if (succeeded == false)
    {
        std::cout << "Authentication failed!" << std::endl;
        return 1;
    }

    PropertyObjectPtr licenseConfig = PropertyObject();
    licenseConfig.addProperty(
        StringProperty("LicensePath", "C:\\Users\\jakob\\source\\openDAQ\\examples\\cpp\\licensing_example\\license.lic"));
    succeeded = licensingModulePtr.loadLicense(licenseConfig);

    if (succeeded == false)
    {
        std::cout << "Failed to load license!" << std::endl;
        return 1;
    }
    // ------------------------------------------------------------------------------- //



    // --- Create and connect the licensed passthrough function block and read some signals --- //
    const auto fb = instance.addFunctionBlock("LicensingModulePassthrough");

    fb.getInputPorts()[0].connect(inputSignal);

    const auto outputSignal = fb.getSignals()[0];

    const uint64_t noOfSamples = 100;

    const auto reader = StreamReaderBuilder() 
                            .setSignal(outputSignal)
                            .setValueReadType(SampleType::Float32)
                            .setDomainReadType(SampleType::Int64)
                            .setReadMode(ReadMode::Scaled)
                            .setReadTimeoutType(ReadTimeoutType::All)
                            .setSkipEvents(true)
                            .build();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::vector<float> data(noOfSamples);
    std::vector<int64_t> time(noOfSamples);

    SizeT noOfSamplesRead = noOfSamples;
    const auto readerStatus = reader.readWithDomain(data.data(), time.data(), &noOfSamplesRead);
    // Check if we can read the data (otherwise, we might have had a license problem)...
    if (noOfSamplesRead == noOfSamples)
    {
        std::cout << "Read all " << noOfSamplesRead << " samples" << std::endl;
    }
    else
    {

        auto fbStatus = fb.getStatusContainer().getStatusMessage("ComponentStatus");
        if (fbStatus.getLength() == 0)
            fbStatus = "<OK>";

        std::cerr << "Failed to read the expected number of samples. Status of pass-through function block: " << fbStatus << std::endl;
    }
    // ---------------------------------------------------------------------------------------- //

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
