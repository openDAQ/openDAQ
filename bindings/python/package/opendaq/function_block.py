import opendaq as opendaq


class FunctionBlock:
    """
    Base class for defining openDAQ function blocks in Python.
    Subclass and override the on_* methods to provide function block behavior.
    """

    def __init__(self, context: opendaq.IContext = None, parent: opendaq.IComponent = None, local_id: str = None):
        self.context = context
        self.parent = parent
        self.local_id = local_id

    @staticmethod
    def create_function_block_type() -> opendaq.IFunctionBlockType:
        return opendaq.FunctionBlockType("MockFunctionBlock", "MockFunctionBlock", "MockFunctionBlock")

    def on_connected(self, port: opendaq.IInputPort):
        pass

    def on_disconnected(self, port: opendaq.IInputPort):
        pass

    def on_packet_received(self, port: opendaq.IInputPort):
        pass

    def create_and_add_input_port(
        self,
        local_id: str,
        notification_method: opendaq.PacketReadyNotification,
        custom_data: opendaq.IBaseObject = None,
        request_gap_packets: bool = False,
        permissions: opendaq.IPermissions = None,
    ) -> opendaq.IInputPortConfig:
        return self._cpp_fb._create_and_add_input_port(local_id, notification_method, custom_data, request_gap_packets, permissions)

    def remove_input_port(self, port: opendaq.IInputPortConfig):
        self._cpp_fb._remove_input_port(port)

    def create_and_add_signal(
        self,
        local_id: str,
        descriptor: opendaq.IDataDescriptor = None,
        visible: bool = True,
        is_public: bool = True,
        permissions: opendaq.IPermissions = None,
    ) -> opendaq.ISignalConfig:
        return self._cpp_fb._create_and_add_signal(local_id, descriptor, visible, is_public, permissions)

    def remove_signal(self, signal: opendaq.ISignalConfig):
        self._cpp_fb._remove_signal(signal)

