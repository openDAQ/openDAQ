from .opendaq import *
import os

OPENDAQ_MODULES_DIR = os.path.dirname(os.path.abspath(__file__))
OPENDAQ_CWD = os.getcwd()
__version__ = '@VERSION@'


class Module:
    """
    Base class for defining openDAQ modules in Python.
    Subclass and override the on_* methods to provide module behavior.
    Register with: instance.module_manager.add_module(MyModule(instance.context))
    """

    def __init__(self, context=None, name=None, version=None, id=None):
        """
        Args:
            context: Instance context (IContext). Required when using add_module(MyModule(instance.context)).
            name: Human-readable module name (str).
            version: (major, minor, patch) or IVersionInfo.
            id: Unique module id (str).
        """
        self.context = context
        self.name = name if name is not None else getattr(self, "name", "PythonModule")
        self.version = version if version is not None else getattr(self, "version", (1, 0, 0))
        self.id = id if id is not None else getattr(self, "id", "PythonModule")

    def on_get_available_function_block_types(self):
        """Return dict of id -> IFunctionBlockType. Default: empty dict."""
        return {}

    def on_create_function_block(self, id, parent, local_id, config):
        """Create and return IFunctionBlock, or None if not supported."""
        return None

# BUG: in mac os creating the project with `-> IInstance:` is failing`. 
# To build successfully, we need to remove the `-> IInstance:` from the code.
def Instance(*args):
    if len(args) == 0:
        builder = opendaq.InstanceBuilder()
        builder.module_path = OPENDAQ_MODULES_DIR
        return opendaq.InstanceFromBuilder(builder)
    else:
        return opendaq.Instance(*args)

def InstanceBuilder() -> IInstanceBuilder:
    builder = opendaq.InstanceBuilder()
    builder.module_path = OPENDAQ_MODULES_DIR
    return builder