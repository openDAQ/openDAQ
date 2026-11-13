"""AEC_stub: everything the Add device dialogs need that AEC_mix does not have yet.

The dialogs and add_device_context.py reach proposals 0002, 0004 and 0005 only through this module,
so it is the one file to change when AEC_mix gets the real API. Each name stands in for the real one:

    AddDeviceConfig, AddStreamingOptions,       0002 IAddDeviceConfig, IAddStreamingOptions and
    AddStreamingConfig                          IAddStreamingConfig, with the 0004 authentication_config
                                                and 0005 security_mode and server_fingerprint members
    AuthenticationConfig                        0004 IAuthenticationConfig
    create_add_device_config(parent, info, p)   parent.create_add_device_config(info, prefix)
    CredentialProvider, CredentialRequest       0004 ICredentialProvider, ICredentialRequest
    SecurityMode, ConnectionSecurity,           0005 enums, the levels a capability accepts and
    security_levels, fingerprints               server fingerprints
    connection_string, device_security,         what core works out at add (0005), which the dialogs
    streaming_security, expected_security       also show before the add

Nothing here talks to a server. Each stand-in either maps onto what AEC_mix has, or works out the
result the proposal gives and reports it. Remove a stand-in once AEC_mix has the real call.
"""
import enum
import hashlib
import re

import opendaq as daq

# This build serves no TLS: every server type reports only {'plain': port} in available_transports.
# While True, a server that reports no TLS is taken to accept [Unsecure, Encrypted], as 0005 has every
# server do by default, and a connection asked to be Encrypted or Verified is reported as reaching
# that level although AEC_mix connects plain underneath.
SIMULATE_ENCRYPTION = True


# MARK: - 0005 enums and errors
class ConnectionSecurity(enum.IntEnum):
    """What a connection reached, and what a server accepts at an address."""
    Unsecure = 0
    Encrypted = 1
    Verified = 2


class SecurityMode(enum.Enum):
    """What a connection is asked to reach. Inherit is for a streaming config only."""
    Auto = 'Auto'
    Unsecure = 'Unsecure'
    Encrypted = 'Encrypted'
    Verified = 'Verified'
    Inherit = 'Inherit'


# 0005, Behaviour changes: OpenDAQLTStreamingSecure and daq.lts:// do not exist; daq.lt with Security set
# makes the TLS connection. AEC_mix still loads the type, so the dialogs leave it out.
REMOVED_TYPES = {'OpenDAQLTStreamingSecure'}

DEVICE_SECURITY_MODES = [SecurityMode.Auto, SecurityMode.Unsecure, SecurityMode.Encrypted, SecurityMode.Verified]
STREAMING_SECURITY_MODES = [SecurityMode.Inherit] + DEVICE_SECURITY_MODES


class UntrustedConnectionException(RuntimeError):
    """OPENDAQ_ERR_UNTRUSTED_CONNECTION: below the security mode, or another server fingerprint."""


class CredentialsRequiredException(RuntimeError):
    """OPENDAQ_ERR_CREDENTIALS_REQUIRED: no source supplied credentials for a method with fields."""


class CancelledException(RuntimeError):
    """OPENDAQ_ERR_CANCELLED: the provider's user cancelled."""


class InvalidValueException(RuntimeError):
    """OPENDAQ_ERR_INVALIDVALUE: required credential fields missing or empty."""


class InvalidParameterException(RuntimeError):
    """OPENDAQ_ERR_INVALIDPARAMETER: a connection string with a user part, for a type with methods."""


class NotSupportedException(RuntimeError):
    """OPENDAQ_ERR_NOT_SUPPORTED."""


# MARK: - 0004 standard methods, and the methods 0005 gives each type
class CredentialField:
    def __init__(self, field_id, name, kind='Text', required=True, extensions=''):
        self.id = field_id
        self.name = name
        self.kind = kind  # Text, Secret or FilePath
        self.required = required
        # metadata Extensions: comma-separated, without the dot ("pem,key"); empty for any file
        self.metadata = {'Extensions': extensions} if extensions else {}


class AuthenticationMethod:
    def __init__(self, method_id, description, fields):
        self.id = method_id
        self.description = description
        self.fields = fields


def _standard_methods():
    return {
        'Anonymous': AuthenticationMethod('Anonymous', 'No credentials required', []),
        'UsernamePassword': AuthenticationMethod('UsernamePassword', 'Username and password', [
            CredentialField('Username', 'User name'),
            CredentialField('Password', 'Password', 'Secret')]),
        'Pin': AuthenticationMethod('Pin', 'PIN code', [CredentialField('Pin', 'PIN code', 'Secret')]),
        'PrivateKeyFile': AuthenticationMethod('PrivateKeyFile', 'Private key file', [
            CredentialField('PrivateKeyFilePath', 'Private key file', 'FilePath', extensions='pem,key')]),
        # 0004 does not say which fields are optional; a key without a passphrase has none to enter
        'ClientCertificate': AuthenticationMethod('ClientCertificate', 'Client certificate with its key', [
            CredentialField('CertificateFilePath', 'Certificate file', 'FilePath', extensions='crt,pem'),
            CredentialField('PrivateKeyFilePath', 'Private key file', 'FilePath', extensions='pem,key'),
            CredentialField('KeyPassphrase', 'Key passphrase', 'Secret', required=False)]),
    }


STANDARD_METHODS = _standard_methods()


def same_id(a, b):
    """True for the same method or field id under the branch's and 0004's spelling. AEC_mix declares and
    advertises UserNamePassword with the field UserName; 0004 names them UsernamePassword and Username."""
    return (a or '').lower() == (b or '').lower()


def credentials_for(method, credentials):
    """credentials keyed by the field ids of method, matching ids by same_id; values for no field are dropped."""
    if method is None:
        return {}
    keyed = {}
    for field in method.fields:
        value = next((v for k, v in (credentials or {}).items() if same_id(k, field.id)), None)
        if value is not None:
            keyed[field.id] = value
    return keyed

# 0005, Type configs: the methods each type declares. On AEC_mix these types declare none, and native
# and OPC UA read a login from General.Username and General.Password instead.
DEVICE_TYPE_METHODS = {
    'OpenDAQNativeConfiguration': ['Anonymous', 'UsernamePassword', 'ClientCertificate'],
    'OpenDAQNativeStreaming': ['Anonymous', 'UsernamePassword', 'ClientCertificate'],  # the daq.ns pseudo-device
    'OpenDAQOPCUAConfiguration': ['Anonymous', 'UsernamePassword', 'ClientCertificate'],
    'OPCUAGeneric': ['Anonymous', 'UsernamePassword', 'ClientCertificate'],
    'OpenDAQLTStreaming': ['Anonymous', 'ClientCertificate'],
    'OpenDAQMQTTDevice': ['Anonymous', 'UsernamePassword', 'ClientCertificate'],
}
# A linked streaming type declares none of its own (the native streaming is linked); LT is not linked
STREAMING_TYPE_METHODS = {
    'OpenDAQLTStreaming': ['Anonymous', 'ClientCertificate'],
}


def device_type_methods(type_id):
    return [STANDARD_METHODS[m] for m in DEVICE_TYPE_METHODS.get(type_id, [])]


def streaming_type_methods(type_id):
    return [STANDARD_METHODS[m] for m in STREAMING_TYPE_METHODS.get(type_id, [])]


# MARK: - 0005 levels and fingerprints
def security_levels(available_transports, client_transports=None, advertised=True):
    """IServerCapability.security_levels: the levels a server accepts at an address, lowest first.

    available_transports is the capability's {name: port} from AEC_mix, client_transports the client
    type's. A transport the client type cannot speak does not count. advertised is False for an add
    by address, where nothing was advertised."""
    if not advertised:
        return []
    names = set(available_transports or {})
    if client_transports:
        names &= set(client_transports)
    levels = []
    if not names or 'plain' in names:
        levels.append(ConnectionSecurity.Unsecure)
    if names - {'plain'}:
        levels.append(ConnectionSecurity.Encrypted)
    elif SIMULATE_ENCRYPTION:
        levels.append(ConnectionSecurity.Encrypted)
    return levels


def secure_connection_port(available_transports):
    """IServerCapability.secure_connection_port: the port for Encrypted and Verified, 0 for none."""
    for name, port in (available_transports or {}).items():
        if name != 'plain':
            return int(port)
    return 0


def fingerprint_of(identity):
    """FAKE: AEC_mix reports no certificate, so a host's fingerprint is made from its identity, the
    manufacturer and serial number, else the host. One host presents one identity, so the value is the
    same over every protocol it serves, as 0005 has it."""
    return 'sha256:' + hashlib.sha256(identity.encode('utf-8')).hexdigest()


def fingerprints_equal(a, b):
    def bare(value):
        return (value or '').strip().lower().replace(':', '').removeprefix('sha256')
    return bare(a) == bare(b)


FINGERPRINT_PATTERN = re.compile(r'^sha256:?([0-9a-fA-F]{2}:?){32}$')


def is_fingerprint(value):
    return bool(FINGERPRINT_PATTERN.match((value or '').strip()))


# FAKE: 0005 records a confirmed fingerprint in ServerFingerprint of the component's copy of the add
# config, so a saved setup reloads verified. Here it is kept for the session, keyed by host identity.
RECORDED_FINGERPRINTS = {}

# FAKE: IServerCapability.security and server_fingerprint of a device's connection info, by global id
CONNECTIONS = {}


def resolve_security(mode, levels, fingerprint=''):
    """The level a connection asked for mode reaches before verification, at a server that accepts levels
    ([] for an add by address). Raises UntrustedConnectionException where 0005 fails the connect."""
    offers_plain = not levels or ConnectionSecurity.Unsecure in levels
    offers_tls = ConnectionSecurity.Encrypted in levels
    if mode == SecurityMode.Unsecure:
        if fingerprint:
            raise InvalidValueException('A server fingerprint needs Security other than Unsecure')
        if not offers_plain:
            raise UntrustedConnectionException('The server accepts no plain connection')
        return ConnectionSecurity.Unsecure
    if mode in (SecurityMode.Encrypted, SecurityMode.Verified) or fingerprint:
        if levels and not offers_tls:
            raise UntrustedConnectionException('The server accepts no encrypted connection')
        return ConnectionSecurity.Encrypted
    # Auto: the highest level advertised; plain for an add by address, where nothing was advertised
    return ConnectionSecurity.Encrypted if offers_tls else ConnectionSecurity.Unsecure


def verify_server(mode, presented, required, provider, request, identity):
    """VerifyServer and confirmServer for an encrypted connection: Verified for a matching pin or a
    recorded fingerprint, else Encrypted, or Verified when the provider confirms the server."""
    required = required or RECORDED_FINGERPRINTS.get(identity, '')
    if required:
        if not fingerprints_equal(required, presented):
            raise UntrustedConnectionException(
                f'The server presents {presented}, not {required}. Nothing was sent to it')
        return ConnectionSecurity.Verified
    # Verified always asks; Auto asks when a provider is set; Encrypted never asks
    if mode == SecurityMode.Verified or (mode == SecurityMode.Auto and provider is not None):
        trusted = False
        if provider is not None:
            try:
                trusted = provider.confirm_server(request, presented)
            except NotSupportedException:
                trusted = False
        if trusted:
            RECORDED_FINGERPRINTS[identity] = presented
            return ConnectionSecurity.Verified
        if mode == SecurityMode.Verified:
            raise UntrustedConnectionException(f'{request.connection_string} was not verified')
    return ConnectionSecurity.Encrypted


# MARK: - 0004 credential provider
class CredentialRequest:
    """What the credentials are for. A request without a method is a server request, the one
    confirm_server receives."""

    def __init__(self, component_type_id, connection_string, method=None, manufacturer='', serial_number='',
                 model='', display_name=''):
        self.component_type_id = component_type_id
        self.authentication_method = method
        self.connection_string = connection_string
        self.manufacturer = manufacturer
        self.serial_number = serial_number
        self.model = model
        self.display_name = display_name


class CredentialProvider:
    """ICredentialProvider. AEC_mix takes only CmdLineCredentialProvider from Python, so the dialogs call
    a provider of their own before add_device instead of the module manager calling it."""

    def request_credentials(self, request):
        """Field id to value for the request's method. Raise CancelledException when the user cancels."""
        raise NotSupportedException('The provider cannot ask for credentials')

    def confirm_server(self, request, fingerprint):
        """True when the user trusts the server presenting fingerprint."""
        raise NotSupportedException('The provider cannot confirm a server')


def has_user_part(connection_string):
    """A connection string with a user part, as in daq.opcua://user:secret@host."""
    match = re.match(r'^[^:/]+://([^/]*)', connection_string or '')
    return match is not None and '@' in match.group(1)


def missing_fields(method, credentials):
    if method is None:
        return []
    return [f for f in method.fields if f.required and not (credentials or {}).get(f.id)]


def resolve_credentials(method, supplied, provider, request, remote):
    """0004, Resolving credentials at add: the supplied credentials, else the provider's. A remote add
    asks no provider and takes only what was supplied. A FilePath credential of a remote add would name
    a file on the gateway."""
    if method is None or not method.fields:
        return {}
    supplied = {k: v for k, v in (supplied or {}).items() if v}
    if remote:
        files = [f.name for f in method.fields if f.kind == 'FilePath' and supplied.get(f.id)]
        if files:
            raise NotSupportedException(f'{", ".join(files)} would name a file on the gateway')
    if supplied:
        missing = missing_fields(method, supplied)
        if missing:
            raise InvalidValueException('Enter ' + ', '.join(f.name for f in missing))
        return supplied
    if remote or provider is None:
        raise CredentialsRequiredException(f'{request.connection_string} needs a login ({method.description})')
    try:
        credentials = provider.request_credentials(request)
    except NotSupportedException:
        raise CredentialsRequiredException(f'{request.connection_string} needs a login ({method.description})')
    missing = missing_fields(method, credentials)
    if missing:
        raise InvalidValueException('Enter ' + ', '.join(f.name for f in missing))
    return credentials


# MARK: - Discovery helpers
def info_value(info, name):
    try:
        value = getattr(info, name)
    except Exception:
        return ''
    return str(value) if value not in (None, '') else ''


def transports_of(component):
    """#1362: a type's or capability's transports, name to port."""
    try:
        return dict(component.available_transports)
    except Exception:
        return {}


def with_port(connection_string, port):
    """The connection string with its port replaced, or added when it has none."""
    if not connection_string or not port:
        return connection_string
    match = re.match(r'^([^:/]+://)(\[[^\]]+\]|[^/:]+)(:\d+)?(.*)$', connection_string)
    if match is None:
        return connection_string
    return f'{match.group(1)}{match.group(2)}:{port}{match.group(4)}'


def host_of(connection_string):
    match = re.match(r'^[^:/]+://(?:[^@/]*@)?(\[[^\]]+\]|[^/:]+)', connection_string or '')
    return match.group(1) if match else ''


def _property_value(obj, name, default=None):
    try:
        return obj.get_property_value(name) if obj.has_property(name) else default
    except Exception:
        return default


class Capability:
    """One server capability of a discovered device, with what 0005 adds to it (security_levels)."""

    def __init__(self, capability):
        self.protocol_id = capability.protocol_id
        self.protocol_type = str(_property_value(capability, 'ProtocolType', ''))
        self.connection_string = capability.connection_string
        # the protocol id need not be a type id ("CredentialDemo" for "CredentialDemoDevice"); the prefix is
        prefix = _property_value(capability, 'Prefix', '') or ''
        self.prefix = str(prefix) or (self.connection_string or '').split('://')[0]
        # (address type, connection string, reachable: True, False or None for not checked)
        self.addresses = []
        try:
            for address in capability.address_info:
                status = address.reachability_status.name
                reachable = True if status == 'Reachable' else False if status == 'Unreachable' else None
                self.addresses.append((str(address.type), address.connection_string, reachable))
        except Exception:
            pass
        states = [a[2] for a in self.addresses]
        self.reachable = True if True in states else False if False in states else None
        self.transports = transports_of(capability)
        try:
            self.authentication_methods = [str(m) for m in capability.authentication_methods]
        except Exception:
            self.authentication_methods = []
        self.security_levels = security_levels(self.transports)

    def address(self, address_type):
        """0002: an address of the preferred type when the device has one, reachable ones first, else
        one of the other type."""
        order = {True: 0, None: 1, False: 2}
        ranked = sorted(self.addresses, key=lambda a: (a[0] != address_type, order[a[2]]))
        return ranked[0][1] if ranked else self.connection_string


def capabilities_of(info):
    """The capabilities of a discovered info; none for a connection string."""
    if info is None or isinstance(info, str):
        return []
    return [Capability(c) for c in info.server_capabilities]


# MARK: - 0004 IAuthenticationConfig
def _method_from_sdk(method):
    """An IAuthenticationMethod of AEC_mix as the stub's AuthenticationMethod, so both read the same."""
    fields = []
    for field in method.fields.values():
        try:
            extensions = str(field.metadata.get('Extensions', '')) if field.metadata is not None else ''
        except Exception:
            extensions = ''
        fields.append(CredentialField(field.id, field.name, field.kind.name, bool(field.required), extensions))
    return AuthenticationMethod(method.id, method.description, fields)


def _sdk_methods(component_type):
    """The type's methods and the IAuthenticationConfig they came from, or ([], None) for a type without."""
    try:
        config = component_type.create_authentication_config()
    except Exception:
        return [], None
    if config is None or len(config.authentication_methods) == 0:
        return [], None
    return [_method_from_sdk(m) for m in config.authentication_methods.values()], config


class AuthenticationConfig:
    """0004 IAuthenticationConfig. Wraps AEC_mix's own config when the type declares methods there,
    else carries the methods 0005 gives the type."""

    def __init__(self, methods, sdk_config=None):
        self.authentication_methods = {m.id: m for m in methods}
        # 0004: the first declared method is selected by default
        self.selected_authentication_method = methods[0] if methods else None
        self.supplied_credentials = {}
        self.sdk_config = sdk_config

    def set_authentication_method(self, method_id):
        """Selects a method and empties supplied_credentials."""
        method = next((m for m in self.authentication_methods.values() if same_id(m.id, method_id)), None)
        if method is None:
            raise NotFoundException(f'No authentication method {method_id}')
        self.selected_authentication_method = method
        self.supplied_credentials = {}


def create_authentication_config(component_type, linked_device_type=None):
    """IComponentType.create_authentication_config: None for a type without methods. A linked streaming
    type has the methods of the device type it is linked to."""
    source = linked_device_type if linked_device_type is not None else component_type
    methods, sdk_config = _sdk_methods(source)
    if not methods:
        table = DEVICE_TYPE_METHODS if linked_device_type is not None or daq.IDeviceType.can_cast_from(source) \
            else STREAMING_TYPE_METHODS
        methods = [STANDARD_METHODS[m] for m in table.get(source.id, [])]
    return AuthenticationConfig(methods, sdk_config) if methods else None


class NotFoundException(RuntimeError):
    """OPENDAQ_ERR_NOTFOUND."""


# MARK: - 0002 add config, with the 0004 and 0005 members
class AddStreamingConfig:
    """0002 IAddStreamingConfig, with 0004 authentication_config and 0005 security_mode and
    server_fingerprint. A linked config held by a device config of its linked type returns the
    device's type_config and authentication_config."""

    def __init__(self, streaming_type, capability, device_config, linked_type_id, linked_device_type):
        self.streaming_type = streaming_type
        self.capability = capability                      # FAKE: the add config caches what it needs
        self.linked_type_id = linked_type_id
        self._device_config = device_config
        self._type_config = None if self.linked else streaming_type_config(device_config._aec_config,
                                                                            streaming_type.id)
        self._authentication_config = None if self.linked else \
            create_authentication_config(streaming_type, linked_device_type)
        self.security_mode = SecurityMode.Inherit
        self.server_fingerprint = ''

    @property
    def linked(self):
        return self.linked_type_id is not None and self.linked_type_id == self._device_config.device_type.id

    @property
    def type_config(self):
        return self._device_config.type_config if self.linked else self._type_config

    @property
    def authentication_config(self):
        return self._device_config.authentication_config if self.linked else self._authentication_config


class AddStreamingOptions:
    """0002 IAddStreamingOptions, with 0004 share_credentials_with_nested_devices."""

    def __init__(self, streaming_configs):
        self.streaming_configs = streaming_configs          # streaming type id to AddStreamingConfig
        self._streaming_type = None
        self.streaming_mode = 'MinHops'
        self.share_credentials_with_nested_devices = False

    @property
    def streaming_type(self):
        """The selected streaming type id, None while none is selected."""
        return self._streaming_type

    @streaming_type.setter
    def streaming_type(self, type_id):
        if type_id is not None and type_id not in self.streaming_configs:
            raise NotFoundException(f'No streaming config {type_id}')
        self._streaming_type = type_id

    @property
    def streaming_config(self):
        return self.streaming_configs.get(self._streaming_type) if self._streaming_type else None


class AddDeviceConfig:
    """0002 IAddDeviceConfig, with 0004 authentication_config and 0005 security_mode and server_fingerprint.
    Under it sits AEC_mix's three-section config, which add_device writes the choices onto."""

    def __init__(self, parent, info, device_type, capability):
        self.device_type = device_type
        self.info = info if not isinstance(info, str) else None   # FAKE: the add config caches what it needs
        self.connection_string = info if isinstance(info, str) else info_value(info, 'connection_string')
        self.capability = capability
        self.remote = _is_remote(parent)                    # 0004 IAddDeviceConfigPrivate.remote
        self._aec_config = parent.create_default_add_device_config()
        general = self._aec_config.get_property_value('General')
        self.type_config = _property_value(self._aec_config.get_property_value('Device'), device_type.id)
        self.primary_address_type = 'IPv4'
        # the module manager seeds it from the instance option ModuleManager.ScanOnAdd
        self.scan_on_add = bool(_property_value(general, 'ScanOnAdd', True))
        self.authentication_config = create_authentication_config(device_type)
        self.security_mode = SecurityMode.Auto
        self.server_fingerprint = ''
        self.streaming_options = None


def _is_remote(parent):
    """An add through a gateway: the parent is a device below this instance's root."""
    if daq.IInstance.can_cast_from(parent):
        return False
    top = parent
    while top.parent is not None:
        top = top.parent
    return parent.global_id != top.global_id


def _device_kind(device_type):
    try:
        return daq.IDeviceType.cast_from(device_type).device_kind
    except Exception:
        return None


def _loaded_streaming_types(parent):
    """Stands in for IDevice.available_streaming_types (#1361), which has no Python binding yet: the
    streaming types of the loaded modules."""
    types = {}
    for module in parent.context.module_manager.modules:
        try:
            available = module.available_streaming_types
        except RuntimeError:
            continue
        for type_id, streaming_type in available.items():
            if type_id not in REMOVED_TYPES:
                types[type_id] = daq.IStreamingType.cast_from(streaming_type)
    return types


def _linked_type_id(streaming_type):
    try:
        linked = streaming_type.linked_device_type
        return linked.id if linked is not None else None
    except Exception:
        return None


NATIVE_TYPE = 'OpenDAQNativeConfiguration'


def create_add_device_config(parent, info, prefix=None):
    """Stands in for IDevice.create_add_device_config(info, prefix), as 0002, Creating an add config, has
    it. info is a discovered IDeviceInfo, or a connection string for DeviceInfo(connectionString)."""
    device_types = {type_id: daq.IDeviceType.cast_from(t) for type_id, t in parent.available_device_types.items()
                    if type_id not in REMOVED_TYPES}

    def by_prefix(value):
        return next((t for t in device_types.values() if t.connection_string_prefix == value), None)

    capabilities = capabilities_of(info)
    connection_string = info if isinstance(info, str) else info_value(info, 'connection_string')
    if prefix is not None:
        device_type = by_prefix(prefix)
    elif capabilities:
        native = device_types.get(NATIVE_TYPE)
        offered = [c.prefix for c in capabilities]
        device_type = native if native is not None and native.connection_string_prefix in offered else \
            next((by_prefix(p) for p in offered if by_prefix(p) is not None), None)
    else:
        device_type = by_prefix((connection_string or '').split('://')[0])
    if device_type is None:
        raise NotFoundException(f'No loaded device type for {prefix or connection_string}')
    capability = next((c for c in capabilities if c.prefix == device_type.connection_string_prefix), None)
    config = AddDeviceConfig(parent, info, device_type, capability)

    # 0002: streaming settings exist only for Mirrored device types (and Unspecified ones)
    if _device_kind(device_type) not in (daq.DeviceKind.Mirrored, daq.DeviceKind.Unspecified):
        return config
    streaming_types = _loaded_streaming_types(parent)
    entries = []
    if capabilities:
        # one config per advertised streaming capability
        for c in capabilities:
            if c.protocol_type not in ('Streaming', 'ConfigurationAndStreaming'):
                continue
            streaming_type = next((t for t in streaming_types.values() if t.connection_string_prefix == c.prefix), None)
            if streaming_type is not None:
                entries.append((streaming_type, c))
    else:
        # one per loaded streaming type, since the device's capabilities are not known yet
        entries = [(t, None) for t in streaming_types.values()]
    configs = {}
    for streaming_type, c in entries:
        linked_id = _linked_type_id(streaming_type)
        configs[streaming_type.id] = AddStreamingConfig(streaming_type, c, config, linked_id,
                                                        device_types.get(linked_id) if linked_id else None)
    options = AddStreamingOptions(configs)
    linked = [type_id for type_id, s in configs.items() if s.linked]
    # 0002: the linked type when listed, else the first listed type for a discovered info
    options.streaming_type = linked[0] if linked else next(iter(configs), None) if capabilities else None
    config.streaming_options = options
    return config


def streaming_type_config(aec_config, type_id):
    return _property_value(aec_config.get_property_value('Streaming'), type_id)


# MARK: - What core works out at add (0005), also shown before the add
def _secure_port(capability, type_transports):
    transports = capability.transports if capability is not None else type_transports
    return secure_connection_port({n: p for n, p in transports.items() if n in type_transports})


def device_levels(config):
    """The levels the server accepts at the device's address; [] for an add by address."""
    if config.capability is None:
        return []
    return security_levels(config.capability.transports, transports_of(config.device_type))


def device_security(config):
    """The level the device connection reaches before verification; raises where 0005 fails it."""
    return resolve_security(config.security_mode, device_levels(config), config.server_fingerprint.strip())


def connection_string(config):
    """Where the device connection goes: an address of the preferred type, on the encrypted transport's
    own port when the connection is encrypted and the server serves TLS on another port."""
    address = config.capability.address(config.primary_address_type) if config.capability is not None \
        else config.connection_string
    try:
        security = device_security(config)
    except Exception:
        security = None
    port = _secure_port(config.capability, transports_of(config.device_type))
    if security is not None and security >= ConnectionSecurity.Encrypted and port:
        address = with_port(address, port)
    return address


def identity(config):
    """The host identity a fingerprint belongs to."""
    if config.info is not None and info_value(config.info, 'serial_number'):
        return f"{info_value(config.info, 'manufacturer')}|{info_value(config.info, 'serial_number')}"
    return host_of(connection_string(config))


def streaming_mode_of(config, streaming):
    return config.security_mode if streaming.security_mode == SecurityMode.Inherit else streaming.security_mode


def streaming_levels(streaming):
    if streaming.capability is None:
        return []
    return security_levels(streaming.capability.transports, transports_of(streaming.streaming_type))


def streaming_connection_string(config, streaming):
    if streaming is None or streaming.capability is None:
        return ''
    address = streaming.capability.address(config.primary_address_type)
    port = _secure_port(streaming.capability, transports_of(streaming.streaming_type))
    if port and address:
        try:
            if streaming_security(config, streaming) >= ConnectionSecurity.Encrypted:
                address = with_port(address, port)
        except Exception:
            pass
    return address


def streaming_fingerprint(config, streaming):
    """0005: empty, the device's fingerprint applies at the device's host, none elsewhere."""
    own = streaming.server_fingerprint.strip()
    if own or streaming_mode_of(config, streaming) == SecurityMode.Unsecure:
        return own
    address = streaming.capability.address(config.primary_address_type) if streaming.capability is not None else ''
    same_host = host_of(address) in ('', host_of(connection_string(config)))
    return config.server_fingerprint.strip() if same_host else ''


def streaming_security(config, streaming):
    return resolve_security(streaming_mode_of(config, streaming), streaming_levels(streaming),
                            streaming_fingerprint(config, streaming))


def expected_security(config):
    """The level the device connection ends at, and what verifies it."""
    if device_security(config) == ConnectionSecurity.Unsecure:
        return 'Unsecure'
    if config.server_fingerprint.strip():
        return 'Verified against the pinned fingerprint'
    if RECORDED_FINGERPRINTS.get(identity(config)):
        return 'Verified against the fingerprint recorded earlier'
    if config.remote:
        # 0005: nothing asks to confirm a server for a remote add
        return 'Verified only through the gateway\'s trust store' if config.security_mode == SecurityMode.Verified \
            else 'Encrypted'
    if config.security_mode == SecurityMode.Verified:
        return 'Verified once you confirm the server'
    if config.security_mode == SecurityMode.Auto:
        return 'Verified once you confirm the server, else Encrypted'
    return 'Encrypted'
