def read_file(filename):
    with open(filename, 'r') as file:
        return file.read()

def write_file(filename, content):
    with open(filename, 'w') as file:
        file.write(content)

def read_and_replace(filename, string, new_string):
    write_file(filename, read_file(filename).replace(string, new_string))
    print('Wrote `' + new_string + '` instead of `' + string + '` in `' +  filename + '`')

def parse_yml(filename, key):
    file = read_file(filename)
    start = file.find(':', file.find(key)) + 1
    finish = file.find('\n', start)
    return file[start:finish].strip()

# this code is used to bump openDAQ version

print()
old_version = read_file('opendaq_version').strip()
print('Current version (`MAJOR.MINOR.PATCH...`): `' + old_version + '`')
old_antora_version = parse_yml('docs/Antora/antora.yml', 'version:')
print('Current Antora version: `' + old_antora_version + '`')
print()
new_version = input('New version (`MAJOR.MINOR.PATCH`): ').strip()
suffix = input('Suffix: Release (none, PRESS ENTER), development (dev), or release candidate (rc)?: ').strip().lower()
print()
print('(Release will write "MAJOR.MINOR" in Antora docs version, development  will write "dev", release candidate will write "rc")')
if suffix == 'dev':
    new_antora_version = 'dev' 
    new_version = new_version + 'dev' 
elif suffix == 'rc':
    new_antora_version = 'rc' 
    new_version = new_version + 'rc' 
else: # release
    new_antora_version = new_version[:new_version.rindex('.')]

new_antora_version = '\"' + new_antora_version + '\"'

read_and_replace("opendaq_version", old_version, new_version)
read_and_replace("examples/cpp/opendaq_examples_version", old_version, new_version)
read_and_replace("docs/Antora/antora.yml", old_antora_version, new_antora_version)
read_and_replace("docs/Antora-specs/antora.yml", old_antora_version, new_antora_version)
print('Version bump finished!')
print()
