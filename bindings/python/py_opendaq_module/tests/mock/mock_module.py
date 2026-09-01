import opendaq


class MockModule(opendaq.Module):
    def __init__(self, context):
        super().__init__(context, name="MockPythonModule", version=(1, 2, 3), id="MockPythonModuleId")


def create_module(context):
    return MockModule(context)
