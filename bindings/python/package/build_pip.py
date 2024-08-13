#!/usr/bin/env python

import sys
import os
import pathlib
import subprocess
import shutil
import hashlib
import re

from collections import namedtuple
from argparse import ArgumentParser


def read_opendaq_version(version_file):
    version = ''
    with open(version_file) as f:
        version = f.readline().strip()
    return version


def auto_python_version(opendaq_filename):
    version = ''
    match = re.search(r'cp.*\d+(m)?', opendaq_filename)
    if match:
        ver = re.search(r'\d+(m)?', match.group(0))
        version = ver.group(0) if ver else ''
    return version


def auto_wheel_tag(opendaq_path):
    linux_pattern = r'linux'
    windows_pattern = r'win'
    macos_pattern = r'macos|darwin'
    machine_pattern = r'x86_64|amd64|arm64'

    wheel_tag = ''
    if re.search(macos_pattern, opendaq_path):
        if 'arm64' in opendaq_path:
            wheel_tag = 'macosx_11_0_arm64'
        else:
            wheel_tag = 'macosx_10_9_x86_64'
    elif re.search(machine_pattern, opendaq_path):
        if re.search(linux_pattern, opendaq_path):
            wheel_tag = 'manylinux_2_17_x86_64.manylinux2014_x86_64'
        elif re.search(windows_pattern, opendaq_path):
            wheel_tag = 'win_amd64'
    return wheel_tag


def find_modules(bin_dir):
    libs_pattern = r'.*\.(so|dll|dylib)'
    modules_pattern = r'\.module\.'
    opendaq_pattern = r'opendaq.*\.(so|pyd)'
    py_libs_pattern = r'py_.*\.so'  # WA for linux builds
    test_libs_pattern = r'_test_'

    listing = os.listdir(bin_dir)
    libs = list(filter(lambda file: re.search(libs_pattern, file), listing))
    modules = list(filter(lambda file: re.search(modules_pattern, file), libs))
    opendaq = list(filter(lambda file: re.match(
        opendaq_pattern, file), listing))
    opendaq = opendaq[0] if opendaq else ''
    libs = list(set(libs) - set(modules))  # remove modules from libs
    # remove py_*.so libs for linux builds
    libs = list(filter(lambda file: not re.search(py_libs_pattern, file), libs))
    libs = list(filter(lambda file: not re.search(
        test_libs_pattern, file), libs))
    # remove opendaq from libs
    libs = list(filter(lambda file: file != opendaq, libs))

    ret = namedtuple('Modules', ['libs', 'modules', 'opendaq'])
    return ret(libs, modules, opendaq)


# parsing arguments
parser = ArgumentParser(
    description='Builds a pip package for the OpenDAQ Python bindings.')

parser.add_argument('-p', '--python-version', help='Python version to build for (default: geussed from module name)',
                    default='')
parser.add_argument('-v', '--package-version',
                    help='Version of the package (default: read from VERSION file)', default='')
parser.add_argument('-t', '--wheel-tag', help='Wheel tag (default: current running OS)',
                    default='')
parser.add_argument('-b', '--build-dir',
                    help='Project build dir(default: .)', default='.')
parser.add_argument('-l', '--lib-dir', help='Path to the directory containing built libs (default: bin)',
                    default='bin')
parser.add_argument('-s', '--stage-dir',
                    help='Path to the pip stage directory (default: pip/packages)', default='pip/packages')
parser.add_argument('-r', '--remove-module', help='Remove built module from binary directory', action='store_true')

args = parser.parse_args()

python_version = args.python_version
package_version = args.package_version
wheel_tag = args.wheel_tag
remove_module = args.remove_module

build_dir = args.build_dir
build_bin_dir = os.path.join(build_dir, args.lib_dir)

modules = find_modules(build_bin_dir)
python_version = auto_python_version(
    modules.opendaq) if not python_version else python_version
wheel_tag = auto_wheel_tag(modules.opendaq) if not wheel_tag else wheel_tag
path_build_pip_source_dir = os.path.dirname(__file__)
package_version = read_opendaq_version(
    os.path.join(path_build_pip_source_dir, '..', '..', '..', 'opendaq_version')) if not package_version else package_version
examples_dir = os.path.join(path_build_pip_source_dir, '..', '..', '..', 'examples', 'python')

if not modules.libs:
    print(f'Could not find any libraries in {build_bin_dir}')
    sys.exit(1)
if not modules.modules:
    print(f'Could not find any modules in {build_bin_dir}')
    sys.exit(1)
if not modules.opendaq:
    print(f'Could not find OpenDAQ module in {build_bin_dir}')
    sys.exit(1)
if not python_version:
    print('Could not detect python version')
    sys.exit(1)
if not package_version:
    print('Could not detect package version')
    sys.exit(1)

print(f'Generating pip package for OpenDAQ ver. {package_version}')
print(f'Python version: {python_version}')
print(f'Wheel tag: {wheel_tag}')

path_stage_dir = os.path.abspath(args.stage_dir)
path_stage_package = os.path.join(path_stage_dir, 'opendaq')
path_stage_dist_info = os.path.join(
    path_stage_dir, f'opendaq-{package_version}.dist-info')

# recreate stage dir
if os.path.exists(path_stage_dir):
    shutil.rmtree(path_stage_dir)
os.makedirs(path_stage_dist_info)
os.makedirs(path_stage_package)
os.makedirs(os.path.join(path_stage_package, 'modules'))

# copy files
for lib in modules.libs:
    shutil.copy(os.path.join(build_bin_dir, lib), path_stage_package)
for module in modules.modules:
    shutil.copy(os.path.join(build_bin_dir, module),
                os.path.join(path_stage_package, 'modules'))
shutil.copy(os.path.join(build_bin_dir, modules.opendaq), path_stage_package)
shutil.copy(os.path.join(examples_dir, 'gui_demo.py'), os.path.join(path_stage_package, '__main__.py'))
shutil.copytree(os.path.join(examples_dir, 'gui_demo'), os.path.join(path_stage_package, 'gui_demo'))

# an empty file signalling to python that auto-complete should be enabled
pathlib.Path(os.path.join(path_stage_package, 'py.typed')).touch()

# __init__.py
shutil.copy(os.path.join(path_build_pip_source_dir, 'opendaq',
            '__init__.py'), path_stage_package)
os.makedirs(os.path.join(path_stage_package, 'opendaq'))

# #generate stubs
subprocess.call(['pybind11-stubgen', 'opendaq'],
               env={**os.environ, 'PYTHONPATH': '.', 'PYTHONDONTWRITEBYTECODE': '1'}, cwd=path_stage_dir)
shutil.move(os.path.join(path_stage_dir, 'stubs', 'opendaq',
            '__init__.pyi'), path_stage_package)
shutil.move(os.path.join(path_stage_dir, 'stubs', 'opendaq', 'opendaq.pyi'),
            os.path.join(path_stage_package, 'opendaq', '__init__.pyi'))
shutil.rmtree(os.path.join(path_stage_dir, 'stubs'))


# metadata
def generate_metadata(destination):
    print(f'Generating {destination}')
    with open(os.path.join(path_build_pip_source_dir, 'METADATA.in')) as f:
        contents = f.readlines()
    contents = ''.join(contents).replace('@VERSION@', package_version)
    with open(destination, 'wt') as f_out:
        f_out.write(contents)


generate_metadata(os.path.join(path_stage_dist_info, 'METADATA'))


def generate_wheel_file(destination):
    lines = [
        'Wheel-Version: 1.0',
        'Generator: bdist_wheel (0.37.1)',
        'Root-Is-Purelib: false',
        f'Tag: cp{python_version.removesuffix("m")}-cp{python_version}-{wheel_tag}',
    ]
    with open(destination, 'wt') as f:
        f.writelines('\n'.join(lines))


generate_wheel_file(os.path.join(path_stage_dist_info, 'WHEEL'))


def hash_file(file_path, algorithm='sha256'):
    """Calculate the hash digest of a file using the specified algorithm."""
    hash_obj = hashlib.new(algorithm)
    with open(file_path, 'rb') as f:
        while True:
            data = f.read(65536)  # Read file in 64KB chunks
            if not data:
                break
            hash_obj.update(data)
    return hash_obj.hexdigest()


def generate_record_file(source_root, destination):
    print(f'Generating {destination}')
    lines = []
    for src_dir, dirs, files in os.walk(source_root):
        for file_ in files:
            src_file = os.path.join(src_dir, file_)
            file_name_relative = pathlib.Path(
                src_file.removeprefix(source_root)[1:]).as_posix()
            if src_file == destination:
                file_size = ''
                file_sha256 = ''
            else:
                file_size = os.path.getsize(src_file)
                file_sha256 = hash_file(src_file)
            line = f'{file_name_relative},{file_sha256},{file_size}'
            lines.append(line + '\n')
    lines.sort()
    with open(destination, 'wt') as f:
        f.writelines(lines)


generate_record_file(path_stage_dir, os.path.join(
    path_stage_dist_info, 'RECORD'))


def generate_wheel(filename):
    cwd = os.getcwd()
    os.chdir(path_stage_dir)
    # relative name here to avoid including archive to itself
    zip_file = shutil.make_archive(f'../{filename}', 'zip')
    os.rename(zip_file, f'{filename}.whl')
    os.chdir(cwd)


generate_wheel(
    f'opendaq-{package_version}-cp{python_version.removesuffix("m")}-cp{python_version}-{wheel_tag}')

if remove_module:
    os.remove(os.path.join(build_bin_dir, modules.opendaq))
