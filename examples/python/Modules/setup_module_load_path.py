import opendaq as daq

if __name__ == "__main__":
    instance_builder = daq.InstanceBuilder()

    # Adds the openDAQ Python package installation modules folder to the load path
    instance_builder.add_module_path(daq.OPENDAQ_MODULES_DIR)

    # Adds a custom module path. In this case it will be the current working directory.
    custom_path = ''
    instance_builder.add_module_path(custom_path)

    instance = instance_builder.build()

    # Print out the module information of all loaded modules
    modules = instance.module_manager.modules
    half_len = len(modules) / 2
    modules = modules[:int(half_len)]  # TODO: Issue where modules appear twice in said list
    for module in modules:
        print(module.module_info)
