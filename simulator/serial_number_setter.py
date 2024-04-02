import json
import uuid

file_name = "opendaq-config.json"

with open(file_name, "r") as json_file:
    data = json.load(json_file)

data["ReferenceDevice"]["LocalId"] = "dummymanufacturer_" + str(uuid.getnode())

with open(file_name, "w") as json_file:
    json.dump(data, json_file, indent=4)
