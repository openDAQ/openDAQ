import json
import netifaces
from xml.etree import ElementTree

# serial number = mac address
serial = netifaces.ifaddresses("enp0s3")[netifaces.AF_LINK][0]["addr"]

def configure_opendaq_config():
    filename = "/home/opendaq/opendaq-config.json"
    
    with open(filename, "r") as json_file:
        data = json.load(json_file)

    modules = data.setdefault("Modules", {})
    
    # localId = manufacturer + _ + serial number
    modules.setdefault("ReferenceDevice", {})["LocalId"] = "opendaq_" + serial
    
    # serial number
    modules.setdefault("ReferenceDevice", {})["SerialNumber"] = serial

    with open(filename, "w") as json_file:
        json.dump(data, json_file, indent = 4)

configure_opendaq_config()
