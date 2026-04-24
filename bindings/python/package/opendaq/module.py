import opendaq as opendaq


class Module:
    """
    Base class for defining openDAQ modules in Python.
    Subclass and override the on_* methods to provide module behavior.
    Register with: instance.module_manager.add_module(MyModule(instance.context))
    """

    def __init__(self, 
                 context: opendaq.IContext,
                 name:str = None, 
                 version = None, 
                 id:str = None):
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

