import sys

def read_file(filename):
    with open(filename, "r") as file:
        content = file.read()
        return content
    
def write_file(filename, content):
    with open(filename, "w") as file:
        file.write(content)

def read_and_replace(filename):
    new = read_file(filename).replace(version, new_version)
    write_file(filename, new)

# this code is used to increase openDAQ version

version = read_file("opendaq_version").strip()
print("Current version: " + version)
new_version = input("New version: ").strip()

read_and_replace("docs/Antora/antora.yml")
read_and_replace("docs/Antora-specs/antora.yml")
read_and_replace("opendaq_version")

print("Version bump was successful!")
