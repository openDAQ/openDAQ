/**
 * Empty example
 */

#include <iostream>
#include <opendaq/opendaq.h>
#include "licensing_example.h"

using namespace daq;

const std::string resourcePath = RESOURCE_PATH;

/*
* A brief example showcasing the use for a licensed module in openDAQ.
* Licensing is currently implemented only in the "LicensingModule", other reference module don't have licensing implemented and behave as usual.
* The module itself requires user authentication (here we use a simple txt file with a password, but in practise an RSA approach
* can be used, where the public key is embedded in the module and the secret key is owned by the relevant user).
* Then the module takes in a license file describing how the module can be used.
* The relevant files are provided with the example. To run the example, the reference quick_start_simulator can be used
* for the input signal.
*/

void tryReadSignalData(const daq::InstancePtr& instance, daq::SignalPtr& inputSignal)
{
    // Sleeping so log output gets flushed before printing..
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "\nAttempting to use a licensed function block...\n" << std::endl;

    // --- Create and connect the unlicensed passthrough function block and read some signals --- //
    const auto fb = instance.addFunctionBlock("LicensingModulePassthrough");

    fb.getInputPorts()[0].connect(inputSignal);

    const uint64_t noOfSamples = 100;

    auto reader = StreamReaderBuilder()
                      .setSignal(fb.getSignals()[0])
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
    auto readerStatus = reader.readWithDomain(data.data(), time.data(), &noOfSamplesRead);
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

        std::cerr << "Failed to read the expected number of samples. Status of pass-through function block: " << fbStatus
                  << "\nNumber of samples read: " << noOfSamplesRead << std::endl;
    }
}

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules at MODULE_PATH
    const StringPtr path = StringPtr("");
    const ModuleAuthenticatorPtr authenticator = daq::ModuleAuthenticator(path);
    const InstancePtr instance =
        daq::InstanceBuilder().setModuleAuthenticator(authenticator).setLoadAuthenticatedModulesOnly(true).setModulePath(MODULE_PATH).build();

    // Setup your paths here..
    std::string licPath = resourcePath + "license.lic";
    // Make sure you open up the quick_start_simulator example app to get a reference device!
    
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

    tryReadSignalData(instance, inputSignal);
    
    // ------------- Find the licensing module ------------------ //
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
    // ---------------------------------------------------------- //

    // --------------------------- Load the license ------------------------------------------- //
    DictPtr<IString, IString> licenseConfig = licensingModulePtr.getLicenseConfig();
    licenseConfig.set("VendorKey", "my_secret_key");
    licenseConfig.set("LicensePath", licPath);
    Bool succeeded = licensingModulePtr.loadLicense(licenseConfig);

    if (succeeded == false)
    {
        std::cout << "Failed to load license!" << std::endl;
        return 1;
    }
    // ---------------------------------------------------------------------------------------- //

    tryReadSignalData(instance, inputSignal);
    
    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
