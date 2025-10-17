/**
 * Empty example
 */

#include <iostream>
#include <fstream>
#include <opendaq/opendaq.h>
#include <licensing_example/module_authenticator_impl_win.h>

using namespace daq;

const std::string resourcePath = RESOURCE_PATH;

/*
* An example imlementation of a ModuleAuthenticator. The example takes in a "certificate" file, which contains the names of the valid modules.
* When loading modules with the module manager, only the files matching these names are loaded and stored into the list (dict) of valid modules.
* The dict can be retrieved later and shows the file used to "authenticate" each module.
* A real implementation would have multiple paths to certificate files, then use something like the windows crypto/trust API to verify each DLL file loaded.
* The user could then check which certificate was used for which file.

class ModuleAuthenticatorImpl : public ModuleAuthenticator
{
public:
    explicit ModuleAuthenticatorImpl(const StringPtr& path, const StringPtr& key)
        : certificatePath(path)
        , key(key)
        , validModules()
    {
        std::string endsWith(".module.dll");

        char* pathStr;
        certificatePath->toString(&pathStr);

        std::ifstream stream;
        stream.open(pathStr, std::ifstream::in);

        if (stream.good())
        {
            for (std::string line; std::getline(stream, line);)
            {
                if (line.compare(line.size() - endsWith.size(), endsWith.size(), endsWith) == 0)
                {
                    validModules.push_back(line);
                }
            }
        }
    }

    Bool onAuthenticateModuleBinary(StringPtr& vendorKey, const StringPtr& binaryPath) override
    {
        std::string pathToModule(binaryPath);

        for (const std::string& moduleName : validModules)
        {
            if (pathToModule.compare(pathToModule.size() - moduleName.size(), moduleName.size(), moduleName) == 0)
            {
                vendorKey = key;
                
                return true;
            }
        }

        return false;
    }

private:
    StringPtr certificatePath;
    StringPtr key;

    std::vector<std::string> validModules;
};
*/

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
    const StringPtr path = StringPtr(resourcePath);

    // Add your signature hash here
    const StringPtr key = StringPtr("84331B1E06F3D0A4AE25AC50E87412B42CBE05B4");

    const ModuleAuthenticatorPtr authenticator =
        daq::createWithImplementation<IModuleAuthenticator, daq::ModuleAuthenticatorImpl>(path);

    const InstancePtr instance =
        daq::InstanceBuilder().setModuleAuthenticator(authenticator).setLoadAuthenticatedModulesOnly(false).setModulePath(MODULE_PATH).build();

    std::cout << std::endl << "Loaded modules:" << std::endl;
    auto vendorKeys = instance.getModuleManager().getVendorKeys();
    
    std::cout << "Vendor Keys:" << vendorKeys.getCount() << std::endl;
    if(vendorKeys == nullptr){
        std::cout << "  No Dict!" << std::endl;

    }else if(vendorKeys.getCount() == 0){
        std::cout << "  No Keys!" << std::endl;
    }else{
        std::cout << "  Keys!" << std::endl;

        for(const auto& key : vendorKeys.getKeys()){
            std::cout << " - Key: " << key.toStdString() << std::endl;
        }    

        for(const auto& val : vendorKeys.getValues()){
            std::cout << " - Value: " << val.toStdString() << std::endl;
        }
    }
    // Setup your paths here..
    std::string licPath = resourcePath + "/license.lic";
    // Make sure you open up the quick_start_simulator example app to get a reference device!
    
    std::cerr << "License path: \"" << licPath << "\"" << std::endl;

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

    std::cout << "\nAttempting to use an unlicensed function block...\n" << std::endl;
    tryReadSignalData(instance, inputSignal);

    // ------------- Find the licensing module ------------------ //
    const auto modules = instance.getModuleManager().getModules();

    // --------------------------- Load the license ------------------------------------------- //
    Bool succeeded = false;
    for (auto module : modules)
    {
        if (key.equals(vendorKeys.get(module.getModuleInfo().getId())))
        {
            std::cerr << "Attempting to authenticate module: \"" << module.getModuleInfo().getName() << "\"" << std::endl;

            if (module.getModuleInfo().getName() == "LicensingModule")
            {
                DictPtr<IString, IString> licenseConfig = module.getLicenseConfig();
                licenseConfig.set("VendorKey", "my_secret_key");
                licenseConfig.set("LicensePath", licPath);
                succeeded = module.loadLicense(licenseConfig);
                std::cerr << "Authenticated: " << succeeded << std::endl;
                
            }
        }
        if (succeeded)
        {
            break;
        }
    }
    if (succeeded == false)
    {
        std::cout << "Failed to load license!" << std::endl;
        return 1;
    }
    // ---------------------------------------------------------------------------------------- //

    std::cout << "\nAttempting to use a licensed function block...\n" << std::endl;
    tryReadSignalData(instance, inputSignal);

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
