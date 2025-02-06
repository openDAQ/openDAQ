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
