import json
import netifaces
from xml.etree import ElementTree

# serial number = mac address
serial = netifaces.ifaddresses("enp0s3")[netifaces.AF_LINK][0]["addr"]
header = ("<?xml version=\"1.0\" standalone='no'?>"
          "<!--*-nxml-*-->\n<!DOCTYPE service-group SYSTEM \"avahi-service.dtd\">\n")


def configure_opendaq_config():
    filename = "/home/opendaq/opendaq-config.json"
    
    with open(filename, "r") as json_file:
        data = json.load(json_file)

    # localId = manufacturer + _ + serial number
    data.setdefault("ReferenceDevice", {})["LocalId"] = "opendaq_" + serial
    
    # serial number
    data.setdefault("ReferenceDevice", {})["Serial"] = serial

    with open(filename, "w") as json_file:
        json.dump(data, json_file, indent = 4)

def configure_serial_in_service():
    filename = "/home/opendaq/opcuatms.service"
    keyword = "serialNumber"
    keyword_length = len(keyword)
    tree = ElementTree.parse(filename)
    root = tree.getroot()
    records = root.find("service").findall("txt-record")
    for record in records:
        text = record.text.lstrip()[:keyword_length]
        if text == keyword:
            record.text = keyword + "=" + serial
            break
    string = header + ElementTree.tostring(root, encoding="unicode")
    with open(filename, "w") as file:
        file.write(string)

configure_opendaq_config()
configure_serial_in_service()
