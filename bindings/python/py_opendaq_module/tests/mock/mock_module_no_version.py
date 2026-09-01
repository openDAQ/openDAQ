import opendaq


class MockModuleNoVersion(opendaq.Module):
    def __init__(self, context):
        super().__init__(context, name="MockPythonModuleNoVersion", version=None, id="MockPythonModuleNoVersionId")


def create_module(context):
    return MockModuleNoVersion(context)
