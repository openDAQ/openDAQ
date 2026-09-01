import opendaq


class MockFunctionBlock(opendaq.FunctionBlock):
    @staticmethod
    def create_function_block_type():
        return opendaq.FunctionBlockType("MockFunctionBlock", "MockFunctionBlock", "MockFunctionBlock", None)

    def on_init(self):
        # add_signal()/add_input_port() require the new object's parent to be exactly the
        # function block's own internal folder - the same one create_and_add_signal() would
        # have used - so it has to be constructed with that folder as its parent up front.
        self.add_signal(opendaq.Signal(self.context, self.signals_folder, "output", None))
        self.add_input_port(opendaq.InputPort(self.context, self.input_ports_folder, "input", False))
        self.set_status_message(opendaq.ComponentStatus.Warning, "mock warning")

        # Deliberately not added through add_signal(): a status signal isn't part of the
        # regular signal list, just returned from getStatusSignal(), so it doesn't need a
        # parent matching the function block's own signals folder.
        self.status_signal = opendaq.Signal(self.context, None, "status", None)

    def on_get_status_signal(self):
        return self.status_signal

    def on_connected(self, port):
        # Observable from the C++ test purely through the standard IFunctionBlock API: a second
        # signal appearing on the function block proves on_connected actually ran on the
        # dispatch thread, asynchronously, after connect() on the C++ side already returned.
        self.add_signal(opendaq.Signal(self.context, self.signals_folder, "connected_marker", None))


class MockModuleWithFb(opendaq.Module):
    def __init__(self, context):
        super().__init__(context, name="MockPythonModuleWithFb", version=(1, 0, 0), id="MockPythonModuleWithFbId")

    def on_get_available_function_block_types(self):
        return {"mock_fb": MockFunctionBlock.create_function_block_type()}

    def on_create_function_block(self, id, parent, local_id, config):
        if id == "mock_fb":
            return MockFunctionBlock(self.context, parent, local_id)
        return None


def create_module(context):
    return MockModuleWithFb(context)
