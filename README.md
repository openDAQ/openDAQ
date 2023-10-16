# openDAQ

## LFS

The openDAQ repository uses Git LFS. Please make sure when cloning/pulling the repository that the LFS files
are retrieved properly. To do so, within the openDAQ repository folder, run:

    git lfs install
    git lfs pull

## Documentation

The Doxygen documentation can be built by enabling the OPENDAQ_BUILD_DOCUMENTATION cmake flag. The user guide
documentation can be built with Antora by following the guide found in docs/Antora/README.md.

## Building openDAQ

#### Supported compilers and platforms:

<table>
  <tr>
   <td><strong>OS</strong></td>
   <td><strong>Platform</strong></td>
   <td><strong>GCC 7.3.1+</strong></td>
   <td><strong>Clang 5+</strong></td>
   <td><strong>VC++ (v14.1+)</strong></td>
  </tr>
  <tr>
   <td rowspan="2">Windows <br>(Visual Studio)</td>
   <td>x86, x64</td>
   <td rowspan="2">/</td>
   <td>☑️</td>
   <td>✅</td>
  </tr>
  <tr>
   <td>arm64</td>
   <td>⚠️🛠️</td>
   <td>⚠️🛠️</td>
  </tr>
  <tr>
   <td rowspan="1">Windows <br>(MinGW)</td>
   <td>x86, x64</td>
   <td>☑️</td>
   <td>☑️</td>
   <td rowspan="10">/
   </td>
  </tr>
  <tr>
   <td rowspan="2">Linux</td>
   <td>x86, x86</td>
   <td>✅</td>
   <td>☑️</td>
  </tr>
  <tr>
   <td>armhfv7, aarch64</td>
   <td>☑️</td>
   <td>☑️</td>
  </tr>
  <tr>
   <td rowspan="2">MacOS <br>(>= 10.15)</td>
   <td>x64</td>
   <td>☑️</td>
   <td>☑️</td>
  </tr>
  <tr>
   <td>arm64</td>
   <td>☑️</td>
   <td>✅</td>
  </tr>
  <tr>
   <td>iOS</td>
   <td>arm64</td>
   <td>🛠️</td>
   <td>🛠️</td>
  </tr>
  <tr>
   <td>Android</td>
   <td>aarch64</td>
   <td>🛠️</td>
   <td>🛠️</td>
  </tr>
</table>

<table>
  <tr>
   <td>✅</td>
   <td>Actively supported (checked with CI)</td>
  </tr>
  <tr>
   <td>☑️</td>
   <td>Actively supported (no CI)</td>
  </tr>
  <tr>
   <td>⚠️</td>
   <td>Not actively supported</td>
  </tr>
  <tr>
   <td>🛠️</td>
   <td>Requires some manual changes or special configuration</td>
  </tr>
</table>

### Required tools before building

 - CMake 3.24 or higher: https://cmake.org/ (might come with development environment like Visual Studio)
 - Git: https://git-scm.com/
 - Git-LFS: https://git-lfs.github.com/
 - Compiler:
   - (msvc) Visual Studio 2017 or higher with installed Workload for C++
   - (gcc windows) MSYS2: http://www.msys2.org, https://github.com/msys2/msys2/wiki/MSYS2-installation
   - (gcc) Ninja build system: https://ninja-build.org/
   - Python3: https://www.python.org/downloads/
 - (optional) Boost C++ Library: https://sourceforge.net/projects/boost/files/boost-binaries/ , http://theboostcpplibraries.com
   - If installed, set CMake option `OPENDAQ_ALWAYS_FETCH_BOOST=OFF` to allow the SDK to use it.
   - See also document [BUILD.md](BUILD.md).
 - (optional) OpenCPPCoverage 0.9.6.1 or higher: https://github.com/OpenCppCoverage/OpenCppCoverage

### Building on Windows

#### 1. Install all required tools / packages.

See [Required tools before building](#required-tools-before-building) above.

#### 2. Clone openDAQ repo.

```shell
git clone git@github.com:openDAQ/openDAQ.git
cd openDAQ
git lfs install
git lfs pull
```

#### 3. Generate CMake project for specific compiler / preset.

In the repository root folder execute the following command to list available presets
then select the one that fits you and generate CMake project:
```shell
cmake --list-presets=all
cmake --preset "x64/msvc-22/full"
```

> ℹ️ If for any reason there is no preset for your compiler (version) you can list the "CMake generators" and
> specify one to override the closest preset (e.g. to use Visual Studio 2019):
> ```shell
> cmake -G
> cmake --preset "x64/msvc-17/full" -G "Visual Studio 16 2019"
> ```

> :warning: When cloning from a stakeholder mirror you have to specify `OPENDAQ_REPO_PREFIX` CMake variable to redirect cloning of the
> dependent libraries to the mirror you have access to. To do this just append the variable to the end of the command like below.
> ```shell
> cmake --preset <your-preset> -DOPENDAQ_REPO_PREFIX=git@gitlab.hbkworld.com:blueberrydaq
> ```

#### 4. Build the project

Open and build `build/x64/msvc-22/full/openDAQ.sln` using Visual Studio (if one ``msvc`` preset had been used above).

Or use command line:
```shell
# build from repository root
cmake --build build/x64/msvc-22/full
# or move to build directory
cd build/x64/msvc-22/full
cmake --build .
```

For other compilers than ``msvc`` one can add parameter ``-j 4`` to the build command to specify the number of parallel builds
(see _cmake.org_: [Build a Project with CMake](https://cmake.org/cmake/help/v3.24/manual/cmake.1.html#build-a-project)).

### Building on Linux

#### 1. Install all required tools / packages.

For example in Ubuntu
```shell
sudo apt-get update
sudo apt-get install -y git git-lfs cmake g++ ninja-build mono-complete python3
```

#### 2. Clone openDAQ repo.

```shell
git clone git@github.com:openDAQ/openDAQ.git
cd openDAQ
git lfs install
git lfs pull
```

#### 3. Generate CMake project for specific compiler / preset.

In the repository root folder execute the following command to list available presets
then select the one that fits you and generate CMake project:
```shell
cmake --list-presets=all
cmake --preset "x64/gcc/full/debug"
```

#### 4. Build the project.

```shell
# build from repository root
cmake --build build/x64/gcc/full/debug
# or move to build directory
cd build/x64/gcc/full/debug
cmake --build .
```

### Additional building info

More information about advanced building options can be found in [BUILD.md](BUILD.md) document.
