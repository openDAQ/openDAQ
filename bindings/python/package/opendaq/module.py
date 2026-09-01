"""Base class for a plugin's module object.

See bindings/python/py_opendaq_module: PythonModule (python_module.cpp) is the C++ side that
loads a plugin file, calls its module-level ``create_module(context)`` function, and drives the
returned object for the module's whole lifetime. A plugin never links against openDAQ's C++
headers or subclasses ``IModule`` directly - every hook below is read off the returned object
through ``getattr()``/``hasattr()`` (duck typing), which is why ``Module`` has to exist as plain
Python. Subclassing it is optional, but it documents the contract in one place and gives plugin
authors safe, no-op defaults to start from.
"""

# Postpones evaluation of every annotation below to a string (PEP 563), so referring to
# `opendaq.IContext` etc. in a signature doesn't need `opendaq` to actually be importable at
# runtime - it never is here, see the TYPE_CHECKING guard below.
from __future__ import annotations

from typing import TYPE_CHECKING, Dict, Optional, Tuple, Union

if TYPE_CHECKING:
    # Only for static type checkers - never actually imported at runtime (both here and when
    # this file's content is exec'd directly into py_opendaq_module's embedded interpreter,
    # there is no real "opendaq" package on disk to import in the first place).
    import opendaq


class Module:
    """Base class for a plugin's module object - the value the plugin file's module-level
    ``create_module(context)`` function returns.

    Example::

        class MyModule(Module):
            def __init__(self, context):
                super().__init__(context, name="MyModule", version=(1, 0, 0), id="MyModuleId")

            def on_get_available_function_block_types(self):
                return {"my_fb": MyFunctionBlock.create_function_block_type()}

            def on_create_function_block(self, id, parent, local_id, config):
                if id == "my_fb":
                    return MyFunctionBlock(self.context, parent, local_id)
                return None

        def create_module(context):
            return MyModule(context)

    Attributes:
        context (opendaq.IContext): The openDAQ context the module was created with. Stored
            as-is by ``__init__`` so subclasses can use it (e.g. to construct
            signals/components) without redeclaring the ``__init__`` parameter themselves.
        name (str): The module's display name. Read once by ``PythonModule``, right after
            ``create_module()`` returns; ``None`` is treated as an empty string.
        version (Union[Tuple[int, int, int], opendaq.IVersionInfo]): The module's version -
            either a plain ``(major, minor, patch)`` tuple of ints, or an
            ``opendaq.IVersionInfo``. Read once, same as ``name``. Unlike ``name``/``id``, a
            module with no version fails to load: ``PythonModule`` raises if this is ``None``.
        id (str): The module's unique id string. Read once, same as ``name``; ``None`` is
            treated as an empty string.
    """

    def __init__(
        self,
        context: opendaq.IContext,
        name: str = "",
        version: Optional[Union[Tuple[int, int, int], opendaq.IVersionInfo]] = None,
        id: str = "",
    ) -> None:
        """Args:
            context: The openDAQ context this module is being created under.
            name: The module's display name.
            version: The module's version, as a ``(major, minor, patch)`` tuple or an
                ``opendaq.IVersionInfo``. Required - loading fails if this is left as ``None``.
            id: The module's unique id string.
        """
        self.context = context
        self.name = name
        self.version = version
        self.id = id

    def on_get_available_function_block_types(self) -> Dict[str, opendaq.IFunctionBlockType]:
        """Returns the function block types this module can create, keyed by type id.

        Called every time a caller asks what function block types are available (e.g. when
        listing a device's capabilities) - the result isn't cached on the C++ side, so this
        should be cheap to compute. The default implementation offers no function block types.

        Returns:
            A dict mapping each supported type id to its ``opendaq.IFunctionBlockType``
            (typically built via a corresponding ``FunctionBlock`` subclass's
            ``create_function_block_type()``).
        """
        return {}

    def on_create_function_block(
        self,
        id: str,
        parent: opendaq.IComponent,
        local_id: str,
        config: Optional[opendaq.IPropertyObject],
    ) -> Optional[opendaq.FunctionBlock]:
        """Creates and returns a new function block delegate for the given type id.

        Args:
            id: One of the type ids returned by ``on_get_available_function_block_types()``.
            parent: The openDAQ component the new function block will be added under.
            local_id: The local id the function block should be constructed with.
            config: An optional configuration property object, or ``None`` if the caller didn't
                supply one.

        Returns:
            A new instance of a ``FunctionBlock`` subclass - plain, not yet wrapped by the C++
            side, that only happens after this method returns - or ``None`` if ``id`` isn't a
            type this module supports. Returning ``None`` makes function block creation fail
            with a "not found" error on the C++ side.
        """
        return None
