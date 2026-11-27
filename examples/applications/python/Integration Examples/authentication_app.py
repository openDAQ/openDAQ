##
# Scaffolding for investigating the credential / authentication API from Python.
#
# Discovers every available device (local and remote) and, for each one, calls
# "process_authentication" - the hook standing in for what an application does when the user
# clicks "Add authenticated device" in a GUI: create the add-device config for the device, select a
# method on its authentication config, supply a credential for it, and call
# "instance.add_device(connection_string, config)" with that add-device config.
#
# The instance is built with a credential provider registered, since an authenticated connection
# needs one able to supply a credential when none is directly supplied on the config.
# CmdLineCredentialProvider prompts on stdin for whatever fields the selected method asks for.
#
# The device type itself carries the authentication methods it supports - a module declares them
# keyed by id. The add-device config's "authentication_config" is one self-contained config whose
# "AuthenticationMethod" selection property lists every supported method; switching methods means
# moving that selection, not fetching a different config.
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


# Creates the add-device config for a discovered device, with the device type the module manager
# would add it with. Returns None if no loaded device type can add it.
def create_add_device_config(instance, device_info):
    try:
        return instance.create_add_device_config(device_info)
    except RuntimeError:
        return None


# The authentication config of an add-device config, or None if its device type does not support
# authentication.
def get_authentication_config(add_device_config):
    if add_device_config is None:
        return None

    try:
        return add_device_config.authentication_config
    except RuntimeError:
        return None


# Selects one of the supported methods by id - returns False if the config doesn't support it.
def select_authentication_method(authentication_config, authentication_method_id):
    try:
        authentication_config.authentication_method = authentication_method_id
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


def process_authentication(instance, device_info, add_device_config):
    # Supplied directly here for the sake of this scaffold - leaving it unset instead would make
    # the registered credential provider (CmdLineCredentialProvider) prompt for it on stdin.
    # The "supplied_credential" setter takes a real `daq.Dict`, not a plain Python dict.
    credential = daq.Dict()
    credential["UserName"] = "user"
    credential["Password"] = "pass"
    add_device_config.authentication_config.supplied_credential = credential

    device = None
    try:
        # The binding doesn't expose IAddDeviceConfig as an IPropertyObject - "add_device" needs it cast
        device = instance.add_device(device_info.connection_string, daq.IPropertyObject.cast_from(add_device_config))
    except RuntimeError:
        pass

    print("Authentication result:", "success" if device is not None else "failure")


if __name__ == "__main__":
    instance = build_instance()

    available_devices = instance.available_devices

    print('Discovered ' + str(len(available_devices)) + ' device(s):\n')
    for device_info in available_devices:
        add_device_config = create_add_device_config(instance, device_info)
        device_type = add_device_config.device_type if add_device_config is not None else None
        authentication_config = get_authentication_config(add_device_config)

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
            process_authentication(instance, device_info, add_device_config)
        elif authentication_config is not None:
            print("Username and password authentication is not available for this device")
        print()
