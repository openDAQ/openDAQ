##
# Modules in openDAQ are shared libraries that are loaded during runtime. They allow users to add different
# components that are bundled within the module. Each module is a bundle of different component types:
# Devices, Function Blocks, Servers, Streaming objects.
#
# On instance creation, the module manager loads all modules found in the specified module load path.
# By default, the modules are loaded from the current program location. However, paths relative to the
# current working directory, or absolute paths, can be specified through the Instance Builder as
# shown in this example
##

import opendaq as daq

if __name__ == "__main__":
    instance_builder = daq.InstanceBuilder()

    # Adds the openDAQ Python package installation modules folder to the load path
    instance_builder.add_module_path(daq.OPENDAQ_MODULES_DIR)

    # Adds a custom module path. In this case it will be the current working directory.
    # Note that if there are multiple openDAQ installations on the system, we might load
    # the same module twice.
    custom_path = ''
    instance_builder.add_module_path(custom_path)

    instance = instance_builder.build()

    # Print out the module information of all loaded modules
    modules = instance.module_manager.modules
    for module in modules:
        print(module.module_info)
