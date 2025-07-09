##
# Example that shows how the module manager can be used to inspect what modules are loaded. This gives us
# information on what component types can be added by an instance that is the owner of said module manager.
#
# Modules in openDAQ are shared libraries that are loaded during runtime. They allow users to add different
# components that are bundled within the module. Each module is a bundle of different component types:
# Devices, Function Blocks, Servers, Streaming objects.
##

import opendaq as daq

if __name__ == "__main__":
    instance = daq.Instance()

    module_manager = instance.module_manager
    modules = module_manager.modules

    # Modules can contain factories for 4 different object types:
    # Devices, Function blocks, Servers, and Streaming objects.
    for module in modules:
        # The Module info contains information on the version, name, ID and description of the module
        print(module.module_info)
        print('  Device Types:        ', list(module.available_device_types.keys()))
        print('  Devices:             ', [info.connection_string for info in module.available_devices])
        print('  Function Block Types:', list(module.available_function_block_types.keys()))
        print('  Server Types:        ', list(module.available_server_types.keys()))
        print('  Streaming Types:     ', list(module.available_streaming_types.keys()))
        print()
