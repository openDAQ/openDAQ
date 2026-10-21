##
# Scaffolding for investigating the credential / authentication API from Python.
#
# Discovers every available device (local and remote) and, for each one, calls
# "process_authentication" - the hook standing in for what an application does when the user
# clicks "Add authenticated device" in a GUI: take the authentication config the device type
# offers, select a method on it, supply a credential for it, and call
# "instance.add_device(connection_string, config)" with that config carrying the authentication
# config (see "with_authentication_config" below for how it's attached).
#
# The instance is built with a credential provider registered, since an authenticated connection
# needs one able to supply a credential when none is directly supplied on the config.
# CmdLineCredentialProvider prompts on stdin for whatever fields the selected method asks for.
#
# The device type itself carries the authentication methods it supports - a module declares them
# keyed by id, reached through "device_type.create_authentication_config()". That returns one
# self-contained config whose "AuthenticationMethod" selection property lists every supported
# method; switching methods means moving that selection, not fetching a different config.
#
# The credential demo module (DAQMODULES_CREDENTIAL_DEMO_MODULE) is the module that actually
# supports authentication: it advertises "Credential demo device" with the methods
# UserNamePassword (its default; user/pass), Pin (1234), PrivateKeyFile and Anonymous.
##

import opendaq as daq


# Inlined rather than imported from "../daq_utils.py" - this script is meant to also work copied
# standalone next to the built "opendaq" module (e.g. into the build's "bin" directory), where
# "daq_utils.py" isn't necessarily alongside it.
def print_indented(msg, depth):
    print('  ' * depth, msg)


# Temporary bridge until `IAuthenticationConfig` becomes a real part of the add-device/add-streaming
# config schema - a caller that wants an authenticated connection smuggles it onto an otherwise plain
# config property object under this key, which the module manager looks for and honors. This key must
# match the one used internally (see `AuthenticationConfigConfigKey` in `module_manager_impl.cpp`).
AUTHENTICATION_CONFIG_CONFIG_KEY = "__AuthenticationConfig"


def with_authentication_config(authentication_config):
    config = daq.PropertyObject()
    config.add_property(daq.ObjectProperty(AUTHENTICATION_CONFIG_CONFIG_KEY, authentication_config))
    return config


# Builds the instance the application runs on, with the credential provider a GUI would offer
# registered up front. The provider is never serialized, so a reloaded instance has to register
# its own again.
def build_instance():
    builder = daq.InstanceBuilder()

    try:
        daq.OPENDAQ_MODULES_DIR
    except:
        builder.module_path = '.'
    else:
        builder.module_path = daq.OPENDAQ_MODULES_DIR

    builder.credential_provider = daq.CmdLineCredentialProvider()

    return builder.build()


# Finds the device type that would handle a discovered device. The type carried on the device
# info is looked up in the instance's own registry, since that is the real, fully-populated type
# (with its actual supported authentication methods) the module manager resolves authentication
# against. Returns None if no type matches.
def resolve_device_type(available_device_types, device_info):
    device_type = device_info.device_type
    if device_type is None:
        return None

    if device_type.id in available_device_types.keys():
        return daq.IDeviceType.cast_from(available_device_types[device_type.id])

    return device_type


# Builds the authentication config for a device type, or returns None if the type does not
# support authentication.
def create_authentication_config(device_type):
    if device_type is None:
        return None

    try:
        return device_type.create_authentication_config()
    except RuntimeError:
        return None


# Selects one of the supported methods by id - returns False if the config doesn't support it.
def select_authentication_method(authentication_config, authentication_method_id):
    try:
        authentication_config.authentication_method_id = authentication_method_id
        return True
    except RuntimeError:
        return False


# Prints what a device offers in terms of authentication - the entry point for working out
# which methods a GUI would put in front of the user.
def print_authentication_support(authentication_config):
    if authentication_config is None:
        print_indented('- Authentication: not supported by this device type', 1)
        return

    methods = authentication_config.authentication_methods
    print_indented('- Authentication methods: ' + ', '.join(methods.keys()), 1)

    selected = authentication_config.selected_authentication_method
    print_indented('- Selected method: ' + selected.id + ' - ' + selected.description, 1)


def process_authentication(instance, device_info, authentication_config):
    # Supplied directly here for the sake of this scaffold - leaving it unset instead would make
    # the registered credential provider (CmdLineCredentialProvider) prompt for it on stdin.
    # The "supplied_credential" setter takes a real `daq.Dict`, not a plain Python dict.
    credential = daq.Dict()
    credential["UserName"] = "user"
    credential["Password"] = "pass"
    authentication_config.supplied_credential = credential

    device = None
    try:
        device = instance.add_device(device_info.connection_string, with_authentication_config(authentication_config))
    except RuntimeError:
        pass

    print("Authentication result:", "success" if device is not None else "failure")


if __name__ == "__main__":
    instance = build_instance()

    available_device_types = instance.available_device_types
    available_devices = instance.available_devices

    print('Discovered ' + str(len(available_devices)) + ' device(s):\n')
    for device_info in available_devices:
        device_type = resolve_device_type(available_device_types, device_info)
        authentication_config = create_authentication_config(device_type)

        # Selected before printing, so "Selected method" below reflects the method this scaffold is
        # actually about to authenticate with, rather than whichever one the config starts out on.
        user_pass_available = False
        if authentication_config is not None:
            user_pass_available = select_authentication_method(authentication_config, "UserNamePassword")

        print(device_info.name + ':')
        print_indented('- Connection string: ' + device_info.connection_string, 1)
        print_indented('- Device type: ' + (device_type.id if device_type else '<unknown>'), 1)
        print_indented('- Remote: ' + str(bool(len(device_info.server_capabilities))), 1)
        print_authentication_support(authentication_config)

        if user_pass_available:
            process_authentication(instance, device_info, authentication_config)
        elif authentication_config is not None:
            print("Username and password authentication is not available for this device")
        print()
