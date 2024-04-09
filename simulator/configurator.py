import json
from uuid import getnode as get_mac
from xml.etree import ElementTree

# serial number = mac address
serial = str(get_mac())
header = ("<?xml version=\"1.0\" standalone='no'?>"
          "<!--*-nxml-*-->\n<!DOCTYPE service-group SYSTEM \"avahi-service.dtd\">\n")


def configure_local_id_in_json(filename, manufacturer):
    with open(filename, "r") as json_file:
        data = json.load(json_file)

    # localId = manufacturer + _ + serial number
    data["ReferenceDevice"]["LocalId"] = manufacturer + "_" + serial

    with open(filename, "w") as json_file:
        json.dump(data, json_file, indent=4)


def configure_serial_in_service(filename, keyword):
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


configure_local_id_in_json("/home/opendaq/opendaq-config.json", "opendaq")
configure_serial_in_service("/home/opendaq/opcuatms.service", "serialNumber")
configure_serial_in_service("/home/opendaq/native_streaming.service", "serialNumber")
