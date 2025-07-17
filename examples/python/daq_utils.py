import opendaq as daq

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
def print_property_object(obj, depth=0):
    for prop in obj.visible_properties:
        value = prop.value
        print_indented('- ' + prop.name + ': ' + str(value), depth)
        if prop.value_type == daq.CoreType.ctObject:
            print_property_object(value, depth + 1)

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

# Sets up an openDAQ simulator device. It uses the reference device as its root
# and instantiates an openDAQ native server. The simulator has mdns discovery enabled.
def setup_simulator():
    config = daq.PropertyObject()
    config.add_property(daq.StringProperty(daq.String(
        'Name'), daq.String('Reference device simulator'), daq.Boolean(True)))
    config.add_property(daq.StringProperty(daq.String(
        'LocalId'), daq.String('RefDevSimulator'), daq.Boolean(True)))
    config.add_property(daq.StringProperty(daq.String(
        'SerialNumber'), daq.String('sim01'), daq.Boolean(True)))

    instance_builder = daq.InstanceBuilder()
    instance_builder.add_discovery_server("mdns")
    instance_builder.set_root_device('daqref://device0', config)
    instance_builder.module_path = daq.OPENDAQ_MODULES_DIR
    server_instance = instance_builder.build()

    server_config = daq.PropertyObject()
    server_instance.add_server('OpenDAQNativeStreaming', server_config).enable_discovery()

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