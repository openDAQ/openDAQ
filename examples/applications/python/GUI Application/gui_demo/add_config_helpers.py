"""What the Add device and Add with config dialogs need around the real IAddDeviceConfig.

The dialogs read and write the config directly. These helpers only list the ways a device can be connected,
reach the transport selection and login of a config, and say where an add will connect.
"""
from dataclasses import dataclass, field

import opendaq as daq

ADDRESS_TYPES = ['IPv4', 'IPv6']
STREAMING_MODES = ['MinHops', 'MinConnections']
NATIVE_TYPE = 'OpenDAQNativeConfiguration'
# where a device or streaming type keeps its transport selection in its type config
DEVICE_TRANSPORT = 'AddDeviceConfig'
STREAMING_TRANSPORT = 'AddStreamingConfig'
AUTHENTICATION_CONFIG_KEY = '__AuthenticationConfig'
# the type config entries the Connection and Login cards show
HIDDEN_SETTINGS = [DEVICE_TRANSPORT, STREAMING_TRANSPORT, 'Username', 'Password']


def info_value(info, name, default=''):
    if info is None:
        return default
    try:
        value = getattr(info, name)
    except Exception:
        return default
    return default if value is None else value


# MARK: - Capabilities and device types
def capabilities_of(info):
    try:
        return [daq.IServerCapability.cast_from(c) for c in info.server_capabilities] if info is not None else []
    except Exception:
        return []


def reachable(capability):
    """True, False, or None when no address reports reachability."""
    if capability is None:
        return None
    statuses = [a.reachability_status for a in capability.address_info]
    if any(s == daq.AddressReachabilityStatus.Reachable for s in statuses):
        return True
    if statuses and all(s == daq.AddressReachabilityStatus.Unreachable for s in statuses):
        return False
    return None


def address_of(capability, address_type):
    """The capability's connection string at the preferred address type, else at the other one."""
    infos = list(capability.address_info)
    for wanted in (address_type, *[t for t in ADDRESS_TYPES if t != address_type]):
        match = next((a for a in infos if a.type == wanted and a.connection_string), None)
        if match is not None:
            return match.connection_string
    return capability.connection_string


def kind_of(device_type):
    try:
        kind = device_type.device_kind
    except Exception:
        return None
    return None if kind == daq.DeviceKind.Unspecified else kind


@dataclass
class ConnectRow:
    """One device type the device can be added over, and the capability it would connect to."""
    device_type: object
    capability: object = None

    @property
    def type_id(self):
        return self.device_type.id

    @property
    def prefix(self):
        return self.device_type.connection_string_prefix

    @property
    def kind(self):
        kind = kind_of(self.device_type)
        return kind.name if kind is not None else ''

    @property
    def adapter(self):
        return kind_of(self.device_type) == daq.DeviceKind.Adapter

    @property
    def default_config(self):
        try:
            return self.device_type.create_default_config()
        except Exception:
            return None

    @property
    def transports(self):
        """name -> port: the ones the capability announces, else the type's defaults."""
        source = self.capability if self.capability is not None else self.device_type
        try:
            return dict(source.available_transports.items())
        except Exception:
            return {}


def device_types_of(parent):
    return {type_id: daq.IDeviceType.cast_from(t) for type_id, t in parent.available_device_types.items()}


def connect_rows(parent, info, typed=None):
    """The device types the device can be added over: native first, the Adapter pseudo-devices last."""
    types = [t for t in device_types_of(parent).values() if kind_of(t) != daq.DeviceKind.Root]

    def by_prefix(prefix):
        return next((t for t in types if t.connection_string_prefix == prefix), None)

    if info is None:
        device_type = by_prefix((typed or '').split('://')[0])
        return [ConnectRow(device_type)] if device_type is not None else []

    rows = []
    for capability in capabilities_of(info):
        device_type = by_prefix(capability.prefix)
        if device_type is None or any(r.type_id == device_type.id for r in rows):
            continue
        row = ConnectRow(device_type, capability)
        # a streaming capability is offered only as the pseudo-device of its Adapter type
        if capability.protocol_type == daq.ProtocolType.Streaming and not row.adapter:
            continue
        rows.append(row)
    rows.sort(key=lambda r: (r.type_id != NATIVE_TYPE, r.adapter))

    if not rows and info.connection_string:
        # local devices (daqref://, miniaudio://) have no capabilities: their own string picks the type
        device_type = by_prefix(info.connection_string.split('://')[0])
        if device_type is not None:
            rows.append(ConnectRow(device_type))
    return rows


# MARK: - Transport selection
def transport_config(type_config, name=DEVICE_TRANSPORT):
    """The IAddComponentConfig a type config carries its transport selection in, or None."""
    if type_config is None or not type_config.has_property(name):
        return None
    try:
        return daq.IAddComponentConfig.cast_from(type_config.get_property_value(name))
    except Exception:
        return None


def transport_names(selection):
    return list(selection.available_transports) if selection is not None else []


def port_override(type_config):
    """The Port of a type config, or None when it has none."""
    if type_config is None or not type_config.has_property('Port'):
        return None
    return type_config.get_property_value('Port')


# MARK: - Streaming
def streaming_configs(cfg):
    """streaming type id -> IAddStreamingConfig, or {} for a type without streaming."""
    options = cfg.streaming_options
    if options is None:
        return {}
    configs = getattr(options, 'available_streaming_configs', None)
    if configs is None:
        configs = options.streaming_configs
    return {type_id: daq.IAddStreamingConfig.cast_from(c) for type_id, c in configs.items()}


def selected_streaming(cfg):
    options = cfg.streaming_options
    if options is None:
        return None
    config = options.streaming_config
    return daq.IAddStreamingConfig.cast_from(config) if config is not None else None


def linked(cfg, streaming_config):
    try:
        link = streaming_config.streaming_type.linked_device_type
    except Exception:
        return False
    return link is not None and link.id == cfg.device_type.id


# MARK: - Login
def authentication_config(device_type):
    """The type's IAuthenticationConfig, or None for a type that declares no methods."""
    try:
        return device_type.create_authentication_config()
    except Exception:
        return None


def offered_methods(auth, capability):
    """The type's methods, as far as a capability that lists methods offers them."""
    if auth is None:
        return []
    methods = list(auth.authentication_methods.values())
    listed = [m.lower() for m in capability.authentication_methods] if capability is not None else []
    if not listed:
        return methods
    return [m for m in methods if m.id.lower() in listed]


def file_types(field):
    """The file dialog filter for a FilePath credential field, from its "Extensions" metadata."""
    try:
        extensions = str(field.metadata['Extensions'])
    except Exception:
        extensions = ''
    patterns = ' '.join(f'*.{e.strip()}' for e in extensions.split(',') if e.strip())
    return ([(field.name, patterns)] if patterns else []) + [('All files', '*.*')]


@dataclass
class PendingAdd:
    """A device the dialogs are about to add: what it is, the row it connects over and its config. The login
    values stay here until the add, since IAuthenticationConfig accepts only a credential complete for its
    method."""
    parent: object
    info: object = None
    typed: str = None
    row: ConnectRow = None
    cfg: object = None
    auth: object = None
    credentials: dict = field(default_factory=dict)

    @property
    def name(self):
        return info_value(self.info, 'name') or self.typed


def pending(parent, info=None, typed=None, type_id=None):
    """A PendingAdd connecting over the row with type_id, else over the first row; row None when nothing fits."""
    rows = connect_rows(parent, info, typed)
    p = PendingAdd(parent, info, typed)
    row = next((r for r in rows if r.type_id == type_id), rows[0] if rows else None)
    if row is not None:
        select_row(p, row)
    return p


def select_row(p, row):
    """A new config for the row's type, so every choice starts again from that type's defaults."""
    info = p.info if p.info is not None else daq.DeviceInfoConfig(p.typed, p.typed)
    p.row = row
    p.cfg = p.parent.create_add_device_config(info, row.prefix)
    p.auth = authentication_config(row.device_type)
    p.credentials = {}
    methods = offered_methods(p.auth, row.capability)
    if p.auth is not None and methods and all(m.id != p.auth.selected_authentication_method.id for m in methods):
        p.auth.authentication_method_id = methods[0].id


def login_method(p):
    return p.auth.selected_authentication_method if p.auth is not None else None


def missing_credentials(p):
    """The required login fields left empty. Nothing asks for them at add, so each must be filled."""
    method = login_method(p)
    if method is None:
        return []
    return [f.name for f in method.fields.values() if f.required and not p.credentials.get(f.id)]


def connection_string(p):
    """The string the add is given: the discovered one, which core resolves, or the typed one."""
    return p.info.connection_string if p.info is not None else p.typed


def connects_to(p):
    """Where the device connection goes: the capability's address at the preferred type, at the port of the
    selected transport or the Port typed into the settings."""
    if p.cfg is None:
        return ''
    if p.row is None or p.row.capability is None:
        return connection_string(p)
    address = address_of(p.row.capability, p.cfg.primary_address_type.name)
    selection = transport_config(p.cfg.type_config)
    port = p.row.transports.get(selection.transport) if selection is not None else None
    # a Port other than the type's default wins over the announced one
    typed_port, default_port = port_override(p.cfg.type_config), port_override(p.row.default_config)
    if typed_port is not None and typed_port != default_port:
        port = typed_port
    return with_port(address, port) if port else address


def has_user_part(connection_string):
    """A connection string with a login in it, as in daq.opcua://user:secret@host."""
    head, sep, rest = (connection_string or '').partition('://')
    return bool(sep) and '@' in rest.split('/')[0]


def with_port(connection_string, port):
    head, sep, rest = connection_string.partition('://')
    if not sep:
        return connection_string
    host, slash, path = rest.partition('/')
    if host.startswith('['):
        host = host[:host.index(']') + 1]
    elif ':' in host:
        host = host.rsplit(':', 1)[0]
    return f'{head}://{host}:{port}{slash}{path}'


def prepare(p):
    """Puts the login onto the config right before the add."""
    # the add config is a Property object underneath; core reads the login from it there
    config = daq.IPropertyObject.cast_from(p.cfg)
    if config.has_property(AUTHENTICATION_CONFIG_KEY):
        config.remove_property(AUTHENTICATION_CONFIG_KEY)
    if p.auth is None:
        return
    filled = {k: v for k, v in p.credentials.items() if v}
    if filled:
        credential = daq.Dict()
        for key, value in filled.items():
            credential[key] = value
        p.auth.supplied_credential = credential
    config.add_property(daq.ObjectProperty(AUTHENTICATION_CONFIG_KEY, p.auth))


# MARK: - Several devices
def copy_choices(source, target):
    """Apply one device's choices to another, as far as the other offers the same type."""
    rows = connect_rows(target.parent, target.info, target.typed)
    row = next((r for r in rows if r.type_id == source.row.type_id), None)
    if row is None:
        return False
    select_row(target, row)
    mine, theirs = target.cfg, source.cfg
    mine.primary_address_type = theirs.primary_address_type
    mine.scan_on_add = theirs.scan_on_add
    copy_values(theirs.type_config, mine.type_config)
    copy_transport(transport_config(theirs.type_config), transport_config(mine.type_config))

    if mine.streaming_options is not None and theirs.streaming_options is not None:
        mine.streaming_options.streaming_mode = theirs.streaming_options.streaming_mode
        chosen = selected_streaming(source.cfg)
        if chosen is not None and chosen.streaming_type.id in streaming_configs(mine):
            mine.streaming_options.streaming_type = chosen.streaming_type.id
            copy_values(chosen.type_config, selected_streaming(mine).type_config)
    if source.auth is not None and target.auth is not None:
        try:
            target.auth.authentication_method_id = source.auth.selected_authentication_method.id
            target.credentials = dict(source.credentials)
        except Exception:
            pass
    return True


def copy_transport(source, target):
    if source is not None and target is not None and source.transport in transport_names(target):
        target.transport = source.transport


def copy_values(source, target):
    """Copies the settings of one property object onto another of the same type, leaving out the transports."""
    if source is None or target is None:
        return
    for prop in source.all_properties:
        name = prop.name
        if name in (DEVICE_TRANSPORT, STREAMING_TRANSPORT) or not target.has_property(name):
            continue
        value = source.get_property_value(name)
        try:
            if daq.IPropertyObject.can_cast_from(value):
                copy_values(daq.IPropertyObject.cast_from(value), daq.IPropertyObject.cast_from(target.get_property_value(name)))
            else:
                target.set_property_value(name, value)
        except Exception:
            pass


# MARK: - Adding
def add_all(parent, pendings):
    """Adds every device with its config. Returns ({name: device}, {name: error message})."""
    added, errors = {}, {}
    for p in pendings:
        try:
            prepare(p)
            # add_device's Python binding takes a Property object, which the add config is underneath
            added[p.name] = parent.add_device(connection_string(p), daq.IPropertyObject.cast_from(p.cfg))
        except Exception as e:
            errors[p.name] = str(e)
    return added, errors
