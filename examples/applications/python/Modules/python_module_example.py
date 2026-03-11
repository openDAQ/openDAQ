##
# Example: defining an openDAQ module in Python by subclassing Module.
#
# Register your module with add_python_module(instance.module_manager, instance.context, my_module_instance).
# Override on_get_available_function_block_types() and on_create_function_block() to provide types and create FBs.
##

import opendaq as daq


class MyModule(daq.Module):
    """Minimal Python module: no function block types, on_create_function_block returns None."""

    def __init__(self, context=None):
        super().__init__(
            context=context,
            name="MyPythonModule",
            version=(1, 0, 0),
            id="MyPythonModule",
        )

    def on_get_available_function_block_types(self):
        return {}

    def on_create_function_block(self, id, parent, local_id, config):
        return None


if __name__ == "__main__":
    builder = daq.InstanceBuilder()
    builder.module_path = "[[none]]"
    instance = builder.build()

    # Register the Python module: pass context so add_module can use it
    instance.module_manager.add_module(MyModule(instance.context))

    # Verify the module is loaded
    for module in instance.module_manager.modules:
        if module.module_info.id == "MyPythonModule":
            print("Python module loaded:", module.module_info)
            break
    else:
        print("MyPythonModule not found in loaded modules")
