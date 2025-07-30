##
# Function blocks are components that allow can process, generate, or consume signal data. They are bundled in
# openDAQ modules (plugins) where each module can give users access to 0 or more function blocks.
#
# This example prints what function blocks are available within the loaded modules and adds all of them.
##

import opendaq as daq
import sys
sys.path.append("..")

if __name__ == "__main__":
    instance = daq.Instance()

    auth_path = R'C:\Users\jakob\source\openDAQ\examples\python\Licensing\authentication_key.txt'
    lic_path = R'C:\Users\jakob\source\openDAQ\examples\python\Licensing\license.lic'

    # Create a fresh openDAQ(TM) instance that we will use for all the interactions with the openDAQ(TM) SDK
    instance = daq.Instance()

    # Find and connect to a simulator device
    for device_info in instance.available_devices:
        if device_info.name == 'Reference device simulator':
            device = instance.add_device(device_info.connection_string)
            break
    else:
        # Exit if no device is found
        #exit(0)
        pass

    # Output the name of the added device
    #print(device.info.name)

    licensing_module = None
    for module in instance.module_manager.modules:
        print(module.module_info)
        if(module.Id is 'LicensingModule')

    # Get the first signal of the first device's channel
    channel = device.channels[0]
    signal = channel.signals[0]

    instance.add_function_block("LicensingModulePassthrough")

    #authenticationConfig = 