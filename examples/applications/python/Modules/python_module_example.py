##
# Example: defining an openDAQ module in Python by subclassing Module.
#
# Register your module with add_python_module(instance.module_manager, instance.context, my_module_instance).
# Override on_get_available_function_block_types() and on_create_function_block() to provide types and create FBs.
##

import opendaq as daq


class MyFunctionBlock(daq.FunctionBlock):
    TYPE_ID = "py_fb_example_uid"

    @staticmethod
    def create_function_block_type() -> daq.IFunctionBlockType:
        default_config = daq.PropertyObject()
        return daq.FunctionBlockType(MyFunctionBlock.TYPE_ID, "py_fb_example", "Python function block example", default_config)

    def on_connected(self, port: daq.IInputPort):
        print("MyFunctionBlock connected:", port)

    def on_disconnected(self, port: daq.IInputPort):
        print("MyFunctionBlock disconnected:", port)

    def on_packet_received(self, port: daq.IInputPort):
        print("MyFunctionBlock packet received:", port)


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
        fb_type = MyFunctionBlock.create_function_block_type()
        return {MyFunctionBlock.TYPE_ID: fb_type}

    def on_create_function_block(self, id, parent, local_id, config):
        if id != MyFunctionBlock.TYPE_ID:
            return None

        # Return Python object; C++ wrapper will turn it into a real IFunctionBlock.
        return MyFunctionBlock(self.context, parent, local_id)


def main() -> int:
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
        return 1

    for id, type in instance.available_function_block_types.items():
        if id == MyFunctionBlock.TYPE_ID:
            print("MyFunctionBlock type found:", id)
            break
    else:
        print("MyFunctionBlock type not found")
        return 1

    # Add the function block
    print("Adding function block:", MyFunctionBlock.TYPE_ID)
    instance.add_function_block(MyFunctionBlock.TYPE_ID)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
