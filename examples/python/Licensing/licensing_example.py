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

    auth_path = 'authentication_key.txt'
    lic_path = 'license.lic'

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
        print(module.module_info.id)
        if(module.module_info.id == 'LicensingModule'):
            print("Found!")
            licensing_module = module

    cfg = licensing_module.license_config
    cfg.set_property_value("VendorKey", "my_secret_key")
    cfg.set_property_value("LicensePath", lic_path)

    licensing_module.load_license(cfg)

    # Get the first signal of the first device's channel
    channel = device.channels[0]
    signal = channel.signals[0]

    # Crate an instance of a licensed passthrough function block
    fb = instance.add_function_block("LicensingModulePassthrough")
    fb.input_ports[0].connect(signal)

    # Create an instance of the renderer function block
    renderer = instance.add_function_block('RefFBModuleRenderer')
    renderer.input_ports[0].connect(fb.signals[0])

    input("Press any key to quit...")