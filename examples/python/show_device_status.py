import opendaq

def main():
    instance = opendaq.Instance()

    # show available devices' connection strings
    for dev in instance.available_devices:
        print(dev.connection_string)

    # add device, use real address instead of localhost
    device = instance.add_device("daq.nd://127.0.0.1")
    
    connection_status = device.status_container.get_status("ConnectionStatus")
    
    # print all possible statuses
    print(connection_status.enumeration_type.enumerator_names)
    
    # print string value of current status
    print(connection_status.name)
    
    # print int value value of current status
    print(connection_status.value)

if __name__ == "__main__":
    main()
