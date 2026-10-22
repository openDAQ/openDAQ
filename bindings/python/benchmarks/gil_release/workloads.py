"""Typical openDAQ Python workloads, one call of each function is one "op".

Call setup() once before running any workload. It adds a reference device and
creates the shared fixtures; teardown() removes them again.
"""
import time

import opendaq

instance = None
device = None
property_object = None
reader = None


def setup():
    global instance, device, property_object, reader
    instance = opendaq.Instance()
    device = instance.add_device('daqref://device0')
    value_signal = next(s for s in device.signals_recursive if s.domain_signal is not None)

    property_object = opendaq.PropertyObject()
    for i in range(10):
        property_object.add_property(opendaq.IntProperty(opendaq.String(f'int{i}'), opendaq.Integer(i), opendaq.Boolean(True)))
        property_object.add_property(opendaq.StringProperty(opendaq.String(f'str{i}'), opendaq.String('x'), opendaq.Boolean(True)))

    reader = opendaq.StreamReader(value_signal)
    # Let the reference device produce some data for stream_read
    time.sleep(0.5)


def teardown():
    global instance, device, property_object, reader
    reader = None
    property_object = None
    instance.remove_device(device)
    device = None
    instance = None


def scalars():
    # Explicitly constructed scalar wrappers, each the last owner of its object
    for i in range(10):
        opendaq.Integer(i)
        opendaq.Float(i + 0.5)
        opendaq.String('abc')
        opendaq.Boolean(True)


def list_build():
    # Build and drop an openDAQ list of 20 integers
    lst = opendaq.List()
    for i in range(20):
        lst.push_back(i)
    del lst


def property_read():
    # Read 20 property values by name; values convert to native Python types
    for i in range(10):
        property_object.get_property_value(f'int{i}')
        property_object.get_property_value(f'str{i}')


def property_iterate():
    # Iterate property metadata; Property wrappers are not the last owners
    for p in property_object.visible_properties:
        p.name
        p.value_type
        p.default_value


def property_object_create():
    # Create a property object with 5 properties and drop it
    po = opendaq.PropertyObject()
    for i in range(5):
        po.add_property(opendaq.IntProperty(opendaq.String(f'p{i}'), opendaq.Integer(i), opendaq.Boolean(True)))
    del po


def device_tree_walk():
    # Inspect all signals of the device and their descriptors
    for s in device.signals_recursive:
        s.name
        s.global_id
        d = s.descriptor
        if d is not None:
            d.sample_type
            d.unit
            d.rule


def channel_props():
    # Read the configuration of every channel
    for ch in device.channels_recursive:
        for p in ch.visible_properties:
            p.name
            ch.get_property_value(p.name)


def stream_read():
    # Read whatever data is available, without waiting
    reader.read(100, 0)


def packet_create():
    # Create a data packet with its descriptor and drop it
    builder = opendaq.DataDescriptorBuilder()
    builder.sample_type = opendaq.SampleType.Float64
    descriptor = builder.build()
    packet = opendaq.DataPacket(descriptor, 100, 0)
    del packet, descriptor, builder


WORKLOADS = [scalars, list_build, property_read, property_iterate, property_object_create,
             device_tree_walk, channel_props, stream_read, packet_create]


def select(names):
    if not names:
        return WORKLOADS
    by_name = {w.__name__: w for w in WORKLOADS}
    return [by_name[n] for n in names]
