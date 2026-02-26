# Basic building

Basic building instructions can be found in [README.md](README.md) document.

# Additional building options

The complete list of build options can be found in [CMake-Options.md](CMake-Options.md) document.

## BOOST

If you want to use a **custom BOOST version**:

- Download desired BOOST version (min supported is 1.62.0) from https://sourceforge.net/projects/boost/files/boost-binaries/
- Set cmake parameter `OPENDAQ_ALWAYS_FETCH_BOOST=OFF`

```shell
cmake --preset "x64/gcc/full/debug" -DOPENDAQ_ALWAYS_FETCH_BOOST=OFF"
```

# Compilers

## ARM notes

Build all libs and dependencies with `CMAKE_POSITION_INDEPENDENT_CODE=ON`

Build boost steps:

- Create user-config.jam file in home directory. In the file write: `using gcc : aarch64 : aarch64-linux-gnu-g++`
- Configure with `./bootstrap.sh --prefix=<path>boost_1_69_0-Linux --with-libraries=regex,date_time,filesystem,system`
- Build with `./b2 install -j 2  cxxflags=-fPIC cflags=-fPIC toolset=gcc-aarch64`

## Intel oneAPI DPC++/C++ (icx) notes

Requires Intel oneAPI Base Toolkit with the `intel.oneapi.win.cpp-dpcpp-common` component.

See also: https://github.com/oneapi-src/oneapi-ci for official CI integration examples.

### Windows

**Installation**

1. Download Intel oneAPI Base Toolkit from https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit.html
2. Run the installer and select at least the **DPC++/C++ Compiler** component (`intel.oneapi.win.cpp-dpcpp-common`)
3. Set up the environment before configuring:
   ```shell
   "C:\Program Files (x86)\Intel\oneAPI\setvars-vcvarsall.bat"
   "C:\Program Files (x86)\Intel\oneAPI\compiler\latest\env\vars.bat"
   ```

**Building**

```shell
cmake -B build -G Ninja -DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=icpx
cmake --build build
```
