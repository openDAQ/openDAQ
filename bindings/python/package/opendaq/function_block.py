"""Base class for a plugin's function block delegate.

See bindings/python/py_opendaq_module: PythonFunctionBlockImpl (python_function_block_impl.cpp)
is the C++ side that wraps and drives an instance of a FunctionBlock subclass - the object a
plugin's ``Module.on_create_function_block()`` returns. A plugin never links against openDAQ's
C++ headers or subclasses ``IFunctionBlock`` directly - every hook below is read off the
delegate through ``getattr()``/``hasattr()`` (duck typing), which is why ``FunctionBlock`` has to
exist as plain Python. Subclassing it is optional, but it documents the contract in one place and
gives plugin authors safe, no-op defaults to start from.
"""

# Postpones evaluation of every annotation below to a string (PEP 563), so referring to
# `opendaq.IFunctionBlockType` etc. in a signature doesn't need `opendaq` to actually be
# importable at runtime - it never is here, see the TYPE_CHECKING guard below.
from __future__ import annotations

from typing import TYPE_CHECKING, Optional

if TYPE_CHECKING:
    # Only for static type checkers - never actually imported at runtime (both here and when
    # this file's content is exec'd directly into py_opendaq_module's embedded interpreter,
    # there is no real "opendaq" package on disk to import in the first place).
    import opendaq


class FunctionBlock:
    """Base class for a plugin's function block delegate, returned from
    ``Module.on_create_function_block()``.

    Example::

        class MyFunctionBlock(FunctionBlock):
            @staticmethod
            def create_function_block_type():
                return FunctionBlockType("MyFb", "My FB", "Does a thing", None)

            def on_init(self):
                self.add_signal(Signal(self.context, self.signals_folder, "output", None))

    Attributes:
        context (opendaq.IContext): The openDAQ context this function block was created under.
            Stored as-is by ``__init__``.
        parent (opendaq.IComponent): The component this function block was added under.
        local_id (str): The local id this function block was constructed with.
    """

    def __init__(self, context: opendaq.IContext, parent: opendaq.IComponent, local_id: str) -> None:
        """Args:
            context: The openDAQ context this function block is being created under.
            parent: The component this function block is being added under.
            local_id: The local id to construct this function block with.
        """
        self.context = context
        self.parent = parent
        self.local_id = local_id

    @staticmethod
    def create_function_block_type() -> opendaq.IFunctionBlockType:
        """Returns the ``opendaq.IFunctionBlockType`` describing this function block.

        Called both from ``Module.on_get_available_function_block_types()`` (typically, to
        advertise the type before one is created) and again right after
        ``on_create_function_block()`` returns, to attach the actual type metadata to the new
        function block. Must be overridden - the base implementation always raises.
        """
        raise NotImplementedError

    def on_init(self) -> None:
        """Runs synchronously right after construction, with ``self._cpp_fb`` already set.

        This is where a function block should add its initial signals/input ports (via
        ``add_signal()``/``add_input_port()``) and set up any other initial state - ``__init__``
        itself runs too early for that, before ``self._cpp_fb`` exists.
        """
        pass

    def on_get_status_signal(self) -> Optional[opendaq.ISignal]:
        """Returns this function block's status signal, or ``None`` if it doesn't have one.

        Unlike a signal added through ``add_signal()``, a status signal doesn't need to be
        parented under ``self.signals_folder`` - it isn't part of the regular signal list, just
        returned from this hook.
        """
        return None

    def on_accepts_signal(self, port: opendaq.IInputPort, signal: opendaq.ISignal) -> bool:
        """Returns whether ``signal`` may be connected to ``port``.

        Called synchronously, on whichever thread is attempting the connection - keep this fast
        and side-effect free. The default implementation accepts every signal.
        """
        return True

    def on_connected(self, port: opendaq.IInputPort) -> None:
        """Called asynchronously after a signal is connected to ``port``.

        Runs on ``PythonRuntime``'s dispatch thread, after the C++-side ``connect()`` call that
        triggered it has already returned to its caller - so this must not be relied on to have
        completed by the time a synchronous caller-side call returns. Any exception raised here
        is only logged, never propagated anywhere.
        """
        pass

    def on_disconnected(self, port: opendaq.IInputPort) -> None:
        """Called asynchronously after a signal is disconnected from ``port``.

        Same dispatch/exception-handling behavior as ``on_connected()``.
        """
        pass

    def on_packet_received(self, port: opendaq.IInputPort) -> None:
        """Called asynchronously when a new packet is available to read on ``port``.

        Same dispatch/exception-handling behavior as ``on_connected()``.
        """
        pass

    # ---- Thin wrappers around self._cpp_fb ----
    #
    # self._cpp_fb is set right before on_init() runs (see PythonFunctionBlockImpl's
    # constructor) and cleared right before the backing function block is destroyed - it's a
    # weak handle, so a call made after that point raises rather than touching freed memory.
    # These wrappers exist so a plugin can write e.g. self.add_signal(...) instead of
    # self._cpp_fb.add_signal(...).

    @property
    def signals_folder(self) -> opendaq.IComponent:
        """This function block's own internal signals folder.

        ``add_signal()`` requires a new signal's parent to be exactly this folder - the same one
        an internal ``create_and_add_signal()`` helper would have used, had it been exposed
        instead - so this is the only way a plugin can construct a signal it's able to add:
        ``Signal(self.context, self.signals_folder, "name", None)``.
        """
        return self._cpp_fb.signals_folder

    @property
    def input_ports_folder(self) -> opendaq.IComponent:
        """This function block's own internal input ports folder - see ``signals_folder``."""
        return self._cpp_fb.input_ports_folder

    def add_signal(self, signal: opendaq.ISignal) -> None:
        """Adds ``signal`` to this function block's signal list.

        ``signal`` must have been constructed with ``self.signals_folder`` as its parent.
        """
        self._cpp_fb.add_signal(signal)

    def remove_signal(self, signal: opendaq.ISignalConfig) -> None:
        """Removes a signal previously added with ``add_signal()``."""
        self._cpp_fb.remove_signal(signal)

    def add_input_port(self, input_port: opendaq.IInputPort) -> None:
        """Adds ``input_port`` to this function block's input port list.

        ``input_port`` must have been constructed with ``self.input_ports_folder`` as its
        parent. Also registers this function block to receive ``on_accepts_signal()``/
        ``on_connected()``/``on_disconnected()``/``on_packet_received()`` notifications for it -
        a port added any other way won't trigger those hooks.
        """
        self._cpp_fb.add_input_port(input_port)

    def remove_input_port(self, input_port: opendaq.IInputPortConfig) -> None:
        """Removes an input port previously added with ``add_input_port()``."""
        self._cpp_fb.remove_input_port(input_port)

    def add_nested_function_block(self, function_block: opendaq.IFunctionBlock) -> None:
        """Adds ``function_block`` as a nested (child) function block of this one."""
        self._cpp_fb.add_nested_function_block(function_block)

    def remove_nested_function_block(self, function_block: opendaq.IFunctionBlock) -> None:
        """Removes a nested function block previously added with
        ``add_nested_function_block()``.
        """
        self._cpp_fb.remove_nested_function_block(function_block)

    def set_status_message(self, status: opendaq.ComponentStatus, message: str = "") -> None:
        """Updates this function block's "ComponentStatus" entry to ``status``, with an optional
        message.

        Lets a plugin surface its own health/error state the same way built-in components do,
        e.g. ``self.set_status_message(ComponentStatus.Warning, "sensor disconnected")``.
        """
        self._cpp_fb.set_status_message(status, message)
