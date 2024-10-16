import sys

def read_file(filename):
    with open(filename, "r") as file:
        content = file.read()
        return content
    
def write_file(filename, content):
    with open(filename, "w") as file:
        file.write(content)

def read_and_replace(filename):
    new= read_file(filename).replace("opendaq_version", version)
    write_file(filename, new)

# this code is used to inject opqnDAQ version into Antora docs during CI
version = read_file("opendaq_version").strip()

read_and_replace("docs/Antora/antora.yml")
read_and_replace("docs/Antora-specs/antora.yml")
read_and_replace("docs/Antora/modules/getting_started/pages/quick_start_setting_up_cpp.adoc")
read_and_replace("docs/Antora/modules/getting_started/pages/quick_start_setting_up_python.adoc")

