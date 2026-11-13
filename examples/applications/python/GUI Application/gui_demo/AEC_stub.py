"""AEC_stub: what the Add device dialogs need from proposals 0004 and 0005 that AEC_mix does not have yet.

The dialogs reach these proposals only through this module, so it is the one file to change when
AEC_mix gets the real API. Each name stands in for the real one:

    CredentialProvider, CredentialRequest       0004 ICredentialProvider, ICredentialRequest
    SecurityMode, ConnectionSecurity,           0005 enums, the levels a capability accepts and
    security_levels, fingerprints               server fingerprints

Nothing here talks to a server. Each stand-in either maps onto what AEC_mix has, or works out the
result the proposal gives and reports it. Remove a stand-in once AEC_mix has the real call.
"""
import enum
import hashlib
import re

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
