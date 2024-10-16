import sys

def read_file(filename):
    with open(filename, "r") as file:
        content = file.read()
        return content
    
def write_file(filename, content):
    with open(filename, "w") as file:
        file.write(content)

# this code is used to inject opqnDAQ version into Antora docs during CI

f1 = "docs/Antora/antora.yml"
f2 = "docs/Antora-specs/antora.yml"
version = read_file("opendaq_version").strip()
a1 = read_file(f1).replace("opendaq_version", version)
write_file(f1, a1)
a2 = read_file(f2).replace("opendaq_version", version)
write_file(f2, a2)
