import os
import opendaq as daq

if not hasattr(daq, 'OPENDAQ_MODULES_DIR'):
    daq.OPENDAQ_MODULES_DIR = os.getcwd()

def print_indented(msg, depth):
    print('  ' * depth, msg)

# Connects to a device with a given connection string. The device is added to the instance.
def connect_device(instance, connection_string):
    try:
        print('Connecting to device with connection string: ' + connection_string)
        return instance.add_device(connection_string)
    except:
        print('Failed to connect.')

# Connects to a device with a given connection string. The device is added to the instance
# and appended to the list passed as argument
def connect_and_append_device(instance, connection_string, list_):
    try:
        print('Connecting to device with connection string: ' + connection_string)
        list_.append(instance.add_device(connection_string))
    except:
        print('Failed to connect.')

# Adds a function block with a given ID. The function block is added to the instance
# and appended to the list passed as argument
def add_and_append_fb(instance, id_, list_):
    try:
        print('Adding function block with ID: ' + id_)
        list_.append(instance.add_function_block(id_))
    except:
        print('Failed to add function block.')

# Prints all properties of a folder and its children.
# If 'recurse==True', the function recurses down the component tree
def print_component(folder, depth=0, recurse=True):
    print_indented('- ' + folder.local_id, depth)
    if len(folder.visible_properties):
        print_indented('- Properties:', depth + 1)
        print_property_object(folder, depth + 2)

    if recurse and daq.IFolder.can_cast_from(folder):
        folder_cast = daq.IFolder.cast_from(folder)
        for itm in folder_cast.items:
            if daq.IFolder.can_cast_from(itm) and not len(daq.IFolder.cast_from(itm).items):
                continue
            print_component(itm, depth + 1)

# Prints all property values of a property object. Recurses through nested
# object-type properties.
def print_property_object(obj, depth=0, hidden=[]):
    for prop in obj.visible_properties:
        name = prop.name
        if name in hidden:
            continue

        value = prop.value
        print_indented('- ' + prop.name + ': ' + str(value), depth)
        if prop.value_type == daq.CoreType.ctObject:
            # Only pass names of children to a recurse call. Remove the prefix denoting the property.
            hidden_children = [s.removeprefix(f"{name}.") for s in hidden if s.startswith(f"{name}.")]
            print_property_object(value, depth + 1, hidden_children)

# Prints the names and values of a struct object's fields. Recurses through
# nested struct-type fields.
def print_struct(struct_, depth=0):
    names = struct_.struct_type.field_names
    field_types = struct_.struct_type.field_types
    values = struct_.field_values
    for i in range(0, len(names)):
        if field_types[i] == daq.CoreType.ctStruct:
            print_indented ('- ' + names[i], depth)
            print_struct(values[i], depth + 1)
        else:
            print_indented('- ' + names[i] + ': ' + str(values[i]), depth)

def setup_simulator(**kwargs):
    """
    Sets up an openDAQ AI Signal Simulator. Uses the simulator device module
    as root and starts servers with mDNS discovery enabled.

    Customizable via keyword arguments:
    - num_channels: int - number of AI channels (defaults to 4)
    - protocols: List[str] - server protocols to enable (defaults to ['OpenDAQNativeStreaming'])
    """
    module_path = getattr(daq, "OPENDAQ_MODULES_DIR", os.getcwd())

    instance_builder = daq.InstanceBuilder()
    instance_builder.module_path = module_path
    instance_builder.add_discovery_server("mdns")

    server_instance = instance_builder.build()

    sim_type = daq.IComponentType.cast_from(
        server_instance.available_device_types["SimulatorDevice"])
    config = sim_type.create_default_config()
    config.set_property_value("NumberOfChannels",
        daq.Integer(kwargs.get("num_channels", 4)))

    server_instance.set_root_device("daq.simulator://", config)
    protocols = kwargs.get("protocols", ["OpenDAQNativeStreaming"])
    server_config = daq.PropertyObject()
    for protocol_name in protocols:
        server_instance.add_server(protocol_name, server_config).enable_discovery()

    return server_instance

# Adds the simulator created via setup_simulator to the instance passed as argument
def add_simulator(instance):
    for dev_info in instance.available_devices:
        if dev_info.serial_number == 'sim01' and dev_info.manufacturer == 'openDAQ':
            return instance.add_device(dev_info.connection_string)

# Return a string identifier based on component type
# TODO: This should be replaced by ISerializable::getSerializeId()
def get_component_id_str(component):
    if daq.IDevice.can_cast_from(component):
        return 'Device'
    elif daq.IFunctionBlock.can_cast_from(component):
        return 'Function Block'
    elif daq.IChannel.can_cast_from(component):
        return 'Channel'
    elif daq.IFolder.can_cast_from(component):
        return 'Folder'
    elif daq.IServer.can_cast_from(component):
        return 'Server'
    elif daq.ISignal.can_cast_from(component):
        return 'Signal'
    elif daq.IInputPort.can_cast_from(component):
        return 'Input Port'
    elif daq.ISyncComponent.can_cast_from(component):
        return 'Sync Component'
    elif daq.IComponent.can_cast_from(component):
        return 'Component'