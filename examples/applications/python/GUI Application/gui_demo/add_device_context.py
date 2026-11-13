"""One AddDeviceContext per device being added: its add config, and what the Add device and Add with
config dialogs offer around it.

The values the user picks live in context.config, the 0002 IAddDeviceConfig (with the 0004 and 0005
members), which today comes from AEC_stub. The dialogs read and write it as they would the real object.
The context adds only what the GUI needs on top: which types the device can be connected over, the
login methods a server advertises, what the add will reach, and copying one device's choices to others.
"""
import opendaq as daq

from . import AEC_stub as stub
from .AEC_stub import info_value

ADDRESS_TYPES = ['IPv4', 'IPv6']
STREAMING_MODES = ['MinHops', 'MinConnections', 'Disabled']


class ConnectOption:
    """One device type the device can be added over, and where."""

    def __init__(self, device_type, capability=None, connection_string=None):
        self.type = device_type
        self.type_id = device_type.id
        # type names are generic ("Device", "PseudoDevice"), so the id is what tells two types apart
        self.label = device_type.id
        self.capability = capability
        self.typed_connection_string = connection_string
        self.security_levels = stub.security_levels(capability.transports, stub.transports_of(device_type)) \
            if capability is not None else []

    @property
    def adapter(self):
        try:
            return self.type.device_kind == daq.DeviceKind.Adapter
        except Exception:
            return False

    @property
    def kind(self):
        try:
            kind = self.type.device_kind
        except Exception:
            return ''
        # a module that predates device kinds reports Unspecified, which says nothing about the type
        return '' if kind == daq.DeviceKind.Unspecified else kind.name

    @property
    def reachable(self):
        return self.capability.reachable if self.capability is not None else None

    def address(self, address_type):
        if self.capability is None:
            return self.typed_connection_string
        return self.capability.address(address_type)


class AddDeviceContext:
    """Adding one device: config is the add config, the rest is what the dialogs offer and show."""

    def __init__(self, context, parent_device, info=None, connection_string=None):
        self.context = context
        self.parent_device = parent_device
        self.info = info
        self.typed_connection_string = connection_string if info is None else None
        self.capabilities = stub.capabilities_of(info)

        self.device_types = {type_id: daq.IDeviceType.cast_from(device_type)
                             for type_id, device_type in parent_device.available_device_types.items()
                             if type_id not in stub.REMOVED_TYPES}
        self.connect_options = self._connect_options()

        self.connect = None
        self.config = None
        if self.connect_options:
            self.select_connect(self.connect_options[0].type_id)

    # MARK: - What the device can be connected over
    def _type_by_prefix(self, prefix):
        return next((t for t in self.device_types.values() if t.connection_string_prefix == prefix), None)

    @staticmethod
    def _usable(device_type):
        try:
            return device_type.device_kind != daq.DeviceKind.Root
        except Exception:
            return True

    def _connect_options(self):
        if self.info is None:
            prefix = (self.typed_connection_string or '').split('://')[0]
            device_type = self._type_by_prefix(prefix)
            if device_type is None or not self._usable(device_type):
                return []
            return [ConnectOption(device_type, connection_string=self.typed_connection_string)]

        options = []
        for capability in self.capabilities:
            device_type = self._type_by_prefix(capability.prefix)
            if device_type is None or not self._usable(device_type):
                continue
            option = ConnectOption(device_type, capability)
            # a streaming capability is offered only as the pseudo-device of its Adapter type
            if capability.protocol_type == 'Streaming' and not option.adapter:
                continue
            options.append(option)
        # 0002: native when the device offers it, then the capabilities in order, the pseudo-devices last
        options.sort(key=lambda o: (o.type_id != stub.NATIVE_TYPE, o.adapter))

        if not options and self.info.connection_string:
            # local devices (daqref://, miniaudio://) have no capabilities: their own string picks the type
            device_type = self._type_by_prefix(self.info.connection_string.split('://')[0])
            if device_type is not None and self._usable(device_type):
                options.append(ConnectOption(device_type, connection_string=self.info.connection_string))
        return options

    def connect_option(self, type_id):
        return next((o for o in self.connect_options if o.type_id == type_id), None)

    def select_connect(self, type_id):
        """0002: a dialog that switches the type creates a new config from the same info, with the prefix of
        the picked type, so every choice starts again from that type's defaults."""
        self.connect = self.connect_option(type_id)
        info = self.info if self.info is not None else self.typed_connection_string
        self.config = stub.create_add_device_config(self.parent_device, info, self.connect.type.connection_string_prefix)
        self._fit_login(self.config.authentication_config, self.login_methods)
        self._fit_streaming_login()

    @staticmethod
    def _fit_login(authentication_config, offered):
        """Selects the first offered method when the server does not offer the type's default."""
        if authentication_config is None or not offered:
            return
        selected = authentication_config.selected_authentication_method
        if selected is None or all(m.id != selected.id for m in offered):
            authentication_config.set_authentication_method(offered[0].id)

    def _fit_streaming_login(self):
        if self.streaming is not None and not self.streaming.linked:
            self._fit_login(self.streaming.authentication_config, self.streaming_login_methods)

    # MARK: - The config's streaming and logins
    @property
    def streaming_configs(self):
        options = self.config.streaming_options
        return list(options.streaming_configs.values()) if options is not None else []

    @property
    def streaming(self):
        """The selected AddStreamingConfig, None while none is selected or for a type without streaming."""
        options = self.config.streaming_options
        return options.streaming_config if options is not None else None

    @property
    def streaming_on(self):
        """A streaming is selected and the mode establishes one."""
        return self.streaming is not None and self.config.streaming_options.streaming_mode != 'Disabled'

    def select_streaming(self, type_id):
        self.config.streaming_options.streaming_type = type_id
        self._fit_streaming_login()

    @staticmethod
    def _offered(authentication_config, capability):
        """0004: a capability that lists methods offers only those; one that lists none offers all."""
        if authentication_config is None:
            return []
        methods = list(authentication_config.authentication_methods.values())
        listed = capability.authentication_methods if capability is not None else []
        if not listed:
            return methods
        return [m for m in methods if any(stub.same_id(m.id, listed_id) for listed_id in listed)]

    @property
    def login_methods(self):
        return self._offered(self.config.authentication_config, self.connect.capability)

    @property
    def login(self):
        auth = self.config.authentication_config
        return auth.selected_authentication_method if auth is not None else None

    @property
    def streaming_login_methods(self):
        """An unlinked streaming's own methods; a linked one logs in with the device's login."""
        streaming = self.streaming
        if streaming is None or streaming.linked:
            return []
        return self._offered(streaming.authentication_config, streaming.capability)

    @property
    def streaming_login(self):
        auth = self.streaming.authentication_config if self.streaming_login_methods else None
        return auth.selected_authentication_method if auth is not None else None

    # MARK: - What the add reaches
    def connect_address(self, option):
        """Where an option connects, before its config exists: at the preferred address type."""
        if option is self.connect:
            return self.connection_string()
        return option.address(self.config.primary_address_type if self.config is not None else 'IPv4')

    def connection_string(self):
        return stub.connection_string(self.config)

    def streaming_connection_string(self):
        return stub.streaming_connection_string(self.config, self.streaming)

    def device_security(self):
        """The level the device connection reaches before verification; raises where 0005 fails it."""
        return stub.device_security(self.config)

    def streaming_mode_resolved(self):
        return stub.streaming_mode_of(self.config, self.streaming)

    def streaming_security_resolved(self):
        return stub.streaming_security(self.config, self.streaming)

    def streaming_levels(self):
        return stub.streaming_levels(self.streaming)

    def expected_security(self):
        return stub.expected_security(self.config)

    def missing_credentials(self):
        """The names of required login fields left empty. Left all empty, the credential provider asks at
        add, except for a remote add, where nothing asks."""
        def missing(method, auth):
            if method is None or not method.fields:
                return []
            filled = {k: v for k, v in auth.supplied_credentials.items() if v}
            if not filled and not self.config.remote:
                return []
            return [f.name for f in stub.missing_fields(method, filled)]
        streaming = missing(self.streaming_login, self.streaming.authentication_config) \
            if self.streaming_on and self.streaming_login is not None else []
        return missing(self.login, self.config.authentication_config) + streaming

    # MARK: - Several devices
    def copy_choices_from(self, other):
        """Apply another device's choices here, as far as this device offers the same types."""
        if self.connect_option(other.connect.type_id) is None:
            return False
        self.select_connect(other.connect.type_id)
        mine, theirs = self.config, other.config
        mine.primary_address_type = theirs.primary_address_type
        mine.scan_on_add = theirs.scan_on_add
        mine.security_mode = theirs.security_mode
        mine.server_fingerprint = ''  # a pin names one host
        self._copy_login(theirs.authentication_config, mine.authentication_config)
        stub.copy_values(theirs.type_config, mine.type_config)

        if mine.streaming_options is not None and theirs.streaming_options is not None:
            options, other_options = mine.streaming_options, theirs.streaming_options
            options.streaming_mode = other_options.streaming_mode
            options.share_credentials_with_nested_devices = other_options.share_credentials_with_nested_devices
            selected = other_options.streaming_type
            options.streaming_type = selected if selected in options.streaming_configs else None
            if self.streaming is not None and other.streaming is not None:
                self.streaming.security_mode = other.streaming.security_mode
                if not self.streaming.linked:
                    self._copy_login(other.streaming.authentication_config, self.streaming.authentication_config)
                    if other.streaming.type_config is not None and self.streaming.type_config is not None:
                        stub.copy_values(other.streaming.type_config, self.streaming.type_config)
        return True

    @staticmethod
    def _copy_login(source, target):
        if source is None or target is None or source.selected_authentication_method is None:
            return
        try:
            target.set_authentication_method(source.selected_authentication_method.id)
        except stub.NotFoundException:
            return
        target.supplied_credentials = stub.credentials_for(target.selected_authentication_method,
                                                           source.supplied_credentials)
