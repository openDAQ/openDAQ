import json
import netifaces

# serial number = mac address
serial = netifaces.ifaddresses("enp0s3")[netifaces.AF_LINK][0]["addr"]

def configure_opendaq_config(filename, manufacturer):
    with open(filename, "r") as json_file:
        data = json.load(json_file)

    # localId = manufacturer + _ + serial number
    data.setdefault("ReferenceDevice", {})["LocalId"] = manufacturer + "_" + serial

    modules = data.setdefault("Modules", {})
    modules.setdefault("NativeStreamingServer", {})["SerialNumber"] = serial
    modules.setdefault("OpcUaServer", {})["SerialNumber"] = serial
    modules.setdefault("WebsocketStreamingServer", {})["SerialNumber"] = serial

    with open(filename, "w") as json_file:
        json.dump(data, json_file, indent = 4)

configure_opendaq_config("/home/opendaq/opendaq-config.json", "opendaq")
