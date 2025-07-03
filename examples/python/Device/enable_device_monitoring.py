from gc import enable
import opendaq
import time

def set_device_config(config: opendaq.IPropertyObject):
    
    def enable_monitoring_on_transport_layer(config: opendaq.IPropertyObject):
        transport_layer_config = opendaq.IPropertyObject.cast_from(config.get_property_value("TransportLayerConfig"))
        transport_layer_config.set_property_value("MonitoringEnabled", True)
            
    streaming_config = opendaq.IPropertyObject.cast_from(config.get_property_value("Streaming"))
    native_streaming_config = opendaq.IPropertyObject.cast_from(streaming_config.get_property_value("OpenDAQNativeStreaming"))
    enable_monitoring_on_transport_layer(native_streaming_config)

    device_config = opendaq.IPropertyObject.cast_from(config.get_property_value("Device"))
    native_device_config = opendaq.IPropertyObject.cast_from(device_config.get_property_value("OpenDAQNativeConfiguration"))
    enable_monitoring_on_transport_layer(native_device_config)

def main():
    instance = opendaq.Instance()
    
    device_config = instance.create_default_add_device_config()
    set_device_config(device_config)

    device = instance.add_device("daq.nd://127.0.0.1", device_config) # use proper connection string here
    
    signal = device.channels[0].signals_recursive[0]
    
    reader = opendaq.StreamReader(signal)

    while True:
        data = reader.read(10000, 100)
        print(data)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
        
