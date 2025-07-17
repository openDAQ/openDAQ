##
# Example that showcases how to enable device monitoring for devices connected via the Native Configuration
# (and Streaming) protocol. Monitoring enables a heartbeat mechanism hat can detect connection loss when the
# device does not gracefully disconnect.
#
# This example requires a running openDAQ device with a Native Protocol server on the local machine.
# The "Integration Examples/simulator.py" can be run to provide such a device.
##

import time
import opendaq

# Configures the device config object used to set up the connection parameters when adding devices
def set_device_config(config: opendaq.IPropertyObject):
    def enable_monitoring_on_transport_layer(config: opendaq.IPropertyObject):
        transport_layer_config = opendaq.IPropertyObject.cast_from(config.get_property_value("TransportLayerConfig"))
        transport_layer_config.set_property_value("MonitoringEnabled", True)

    device_config = opendaq.IPropertyObject.cast_from(config.get_property_value("Device"))
    native_device_config = opendaq.IPropertyObject.cast_from(device_config.get_property_value("OpenDAQNativeConfiguration"))
    enable_monitoring_on_transport_layer(native_device_config)

if __name__ == "__main__":
    instance = opendaq.Instance()

    device_config = instance.create_default_add_device_config()
    set_device_config(device_config)

    # Connects to a Native Protocol device on the local network. If the device terminates during the
    # runtime of this example, the disconnection will be detected.
    instance.add_device("daq.nd://127.0.0.1", device_config)
    time.sleep(5)
        
