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

## Limitations

When building with GCC < 9, Clang < 11, or AppleClang < 13.1, the renderer module (depends on SFML 3.0) and the CSV recorder module (depends on Arrow/Parquet, also requires a modern toolchain) must be disabled:

```shell
cmake --preset "x64/gcc/full/debug" \
    -DDAQMODULES_REF_FB_MODULE_ENABLE_RENDERER=OFF \
    -DDAQMODULES_BASIC_CSV_RECORDER_MODULE=OFF
```

Python and C# binding generation is not supported on Intel oneAPI (icx) builds and 32-bit Linux builds:

```shell
cmake --preset "x64/gcc/full/debug" \
    -DOPENDAQ_GENERATE_PYTHON_BINDINGS=OFF \
    -DOPENDAQ_GENERATE_CSHARP_BINDINGS=OFF
```
