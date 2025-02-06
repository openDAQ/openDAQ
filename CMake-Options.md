# CMake Build System Options for openDAQ SDK

This document provides a summary of CMake options that can be used to build openDAQ SDK.
The following options might be used to customize the build passing them in the **configure step** like so: `-D<option_name>=<value>`.

## Table of contents:
* [Feature options](#feature-options)
* [Bindings options](#bindings-options)
* [Modules-build options](#modules-build-options)
* [Test capabilities options](#test-capabilities-options)
* [External dependencies options](#external-dependencies-options)
* [Documentation and examples options](#documentation-and-examples-options)
* [Other options](#other-options)

## Feature options
| Option Name | Type | Default Value | Description | Conditions, if any |
|-|-|-|-|-|
| `OPENDAQ_ENABLE_PARAMETER_VALIDATION` | Bool | `ON` | Enable parameter validation in functions | - |
| `OPENDAQ_THREAD_SAFE` | Bool | `ON` | Enable thread-safe implementations where available | - |
| `OPENDAQ_MIMALLOC_SUPPORT` | Bool | `OFF` | Enable MiMalloc-based packet allocator | - |
| `OPENDAQ_USE_SYNCHRONOUS_LOGGER` | Bool | `OFF` | Output log messages immediately (blocks until finished) | - |
| `OPENDAQ_ENABLE_WEBSOCKET_STREAMING` | Bool | `OFF` | Enable openDAQ websocket LT-protocol streaming | - |
| `OPENDAQ_ENABLE_NATIVE_STREAMING` | Bool | `OFF` | Enable openDAQ native protocol streaming | - |
| `OPENDAQ_ENABLE_OPCUA` | Bool | `OFF` | Enable OpcUa | - |
| `OPCUA_ENABLE_ENCRYPTION` | Bool | `OFF` | Enable OpcUa encryption | Only relevant if `OPENDAQ_ENABLE_OPCUA` is ON |

## Bindings options
| Option name | Type | Default value | Description | Conditions, if any |
|-|-|-|-|-|
| `OPENDAQ_GENERATE_DELPHI_BINDINGS` | Bool | `OFF` | Generate Delphi bindings | Delphi bindings only available in Windows |
| `OPENDAQ_GENERATE_CSHARP_BINDINGS` | Bool | `OFF` | Generate CSharp bindings | - |
| `OPENDAQ_GENERATE_PYTHON_BINDINGS` | Bool | `OFF` | Generate Python bindings | - |
| `OPENDAQ_GENERATE_PYTHON_BINDINGS_STUBS` | Bool | `OFF` | Generate Python bindings stubs for auto-completion | Only relevant if `OPENDAQ_GENERATE_PYTHON_BINDINGS` is ON |

## Modules-build options
| Option name | Type | Default value | Description | Conditions, if any |
|-|-|-|-|-|
| `DAQMODULES_OPENDAQ_CLIENT_MODULE` | Bool | `OFF` | Build openDAQ client modules | - |
| `DAQMODULES_OPENDAQ_SERVER_MODULE` | Bool | `OFF` | Build openDAQ server modules | - |
| `DAQMODULES_EMPTY_MODULE` | Bool | `OFF` | Build demo empty module | - |
| `DAQMODULES_AUDIO_DEVICE_MODULE` | Bool | `OFF` | Build audio device module | - |
| `DAQMODULES_REF_DEVICE_MODULE` | Bool | `OFF` | Build reference device module | - |
| `DAQMODULES_REF_FB_MODULE` | Bool | `OFF` | Build reference function block module | - |
| `DAQMODULES_REF_FB_MODULE_ENABLE_RENDERER` | Bool | `ON` | Enable renderer function block | Only relevant if `DAQMODULES_REF_FB_MODULE` is ON |

## Test capabilities options
| Option name | Type | Default value | Description | Conditions, if any |
|-|-|-|-|-|
| `OPENDAQ_ENABLE_TESTS` | Bool | `ON` | Enable testing | Requires `OPENDAQ_ENABLE_TEST_UTILS` to be ON |
| `OPENDAQ_ENABLE_TEST_UTILS` | Bool | `ON` | Enable testing utils library | - |
| `OPENDAQ_ENABLE_OPTIONAL_TESTS` | Bool | `OFF` | Enable optional (debugging) tests.<Br>When the option value is OFF, all test fixtures defined as `TEST_F_OPTIONAL` are disabled by adding the `DISABLED_` prefix to the test fixture name.  | - |
| `OPENDAQ_ENABLE_COVERAGE` | Bool | `OFF` | Enable code coverage in testing | Only relevant if `OPENDAQ_ENABLE_TESTS` is ON.<Br>Coverage supported only for GCC, G++ and MSVC |
| `OPENDAQ_ENABLE_REGRESSION_TESTS` | Bool | `OFF` | Enable protocol level regression testing framework, that runs as a part of project's GitHub Actions. | Must be set OFF |
| `OPENDAQ_ENABLE_UNSTABLE_TEST_LABELS` | Bool | `OFF` | Enable labeling unstable tests.<Br>When the option value is ON, for all test fixtures defined as `TEST_F_UNSTABLE_SKIPPED` the `UNSTABLE_SKIPPED_` prefix is added to the test fixture name. | - |
| `OPENDAQ_SKIP_UNSTABLE_TESTS` | Bool | `ON` | Skip tests marked as unstable.<Br>When the option value is ON, all test fixtures defined as `TEST_F_UNSTABLE_SKIPPED` are skipped. | Only relevant if `OPENDAQ_ENABLE_UNSTABLE_TEST_LABELS` is ON |
| `OPENDAQ_ENABLE_DELPHI_BINDINGS_TESTS` | Bool | `OFF` | Enable Delphi bindings tests | Only relevant if `OPENDAQ_GENERATE_DELPHI_BINDINGS` and `OPENDAQ_ENABLE_TESTS` are ON |
| `OPENDAQ_ENABLE_PYTHON_BINDINGS_TESTS` | Bool | `OFF` | Enable Python bindings tests | Only relevant if `OPENDAQ_GENERATE_PYTHON_BINDINGS` and `OPENDAQ_ENABLE_TESTS` are ON.<Br>Requires `OPENDAQ_ENABLE_OPCUA`,<br>`OPENDAQ_ENABLE_NATIVE_STREAMING`,<br>`OPENDAQ_ENABLE_WEBSOCKET_STREAMING`,<br>`DAQMODULES_OPENDAQ_CLIENT_MODULE`,<br>`DAQMODULES_OPENDAQ_SERVER_MODULE`,<br>`DAQMODULES_REF_FB_MODULE` and <br>`DAQMODULES_REF_DEVICE_MODULE`<br> to be ON, otherwise, the option is ignored |
| `OPENDAQ_ENABLE_PYTHON_BINDINGS_TESTS_ONLY` | Bool | `OFF` | Enable only the python bindings tests | Only relevant if `OPENDAQ_GENERATE_PYTHON_BINDINGS` is ON.<Br>Requires `OPENDAQ_ENABLE_OPCUA`,<br>`OPENDAQ_ENABLE_NATIVE_STREAMING`,<br>`OPENDAQ_ENABLE_WEBSOCKET_STREAMING`,<br>`DAQMODULES_OPENDAQ_CLIENT_MODULE`,<br>`DAQMODULES_OPENDAQ_SERVER_MODULE`,<br>`DAQMODULES_REF_FB_MODULE` and <br>`DAQMODULES_REF_DEVICE_MODULE`<br> to be ON, otherwise, the option is ignored |
| `OPENDAQ_VENV_PYTHON_EXECUTABLE_TESTS` | Bool | `OFF` | Use python executable from virtual environment for tests | Only relevant if `OPENDAQ_ENABLE_PYTHON_BINDINGS_TESTS` or `OPENDAQ_ENABLE_PYTHON_BINDINGS_TESTS_ONLY` is ON |
| `OPENDAQ_ENABLE_CSHARP_BINDINGS_TESTS` | Bool | `OFF` | Enable CSharp bindings tests | Only relevant if `OPENDAQ_GENERATE_CSHARP_BINDINGS` and `OPENDAQ_ENABLE_TESTS` are ON |
| `OPENDAQ_DOCUMENTATION_TESTS` | Bool | `OFF` | Enable openDAQ documentation tests | Requires `OPENDAQ_ENABLE_OPCUA`,<br>`OPENDAQ_ENABLE_NATIVE_STREAMING`,<br>`OPENDAQ_ENABLE_WEBSOCKET_STREAMING`,<br>`DAQMODULES_OPENDAQ_CLIENT_MODULE`,<br>`DAQMODULES_OPENDAQ_SERVER_MODULE`,<br>`DAQMODULES_REF_FB_MODULE`,<br>`DAQMODULES_REF_DEVICE_MODULE`<br> and `OPENDAQ_ENABLE_TESTS` to be ON, otherwise, the option is ignored |
| `OPENDAQ_ENABLE_OPCUA_INTEGRATION_TESTS` | Bool | `OFF` | Enable OpcUa integration testing | Only relevant if `OPENDAQ_ENABLE_TESTS` and `OPENDAQ_ENABLE_OPCUA` are ON |
| `OPENDAQ_ENABLE_WS_SIGGEN_INTEGRATION_TESTS` | Bool | `OFF` | Enable websocket LT-streaming integration tests | Only relevant for linux platforms and if `OPENDAQ_ENABLE_TESTS`,<br>`DAQMODULES_OPENDAQ_CLIENT_MODULE` and `OPENDAQ_ENABLE_WEBSOCKET_STREAMING` are ON |

## External dependencies options
| Option Name | Type | Default Value | Description | Conditions, if any |
|-|-|-|-|-|
| `OPENDAQ_ALWAYS_FETCH_DEPENDENCIES` | Bool | `ON` | Ignore any installed libraries and always build all dependencies from source | - |
| `OPENDAQ_ALWAYS_FETCH_name` | Bool | `ON` | Specify whether to always fetch or whether to allow CMake to find a locally installed version of 'name' external library | The default value is inherited from `OPENDAQ_ALWAYS_FETCH_DEPENDENCIES` |

### [openDAQ companion specification](https://github.com/openDAQ/opc-ua-companion-spec) options
| Option Name | Type | Default Value | Description | Conditions, if any |
|-|-|-|-|-|
| `HBK_NODESET` | Bool | `ON` | Adds an HBK Nodeset on top of the DAQ ESP specification | - |

### RapidJSON options
| Option Name | Type | Default Value | Description | Conditions, if any |
|-|-|-|-|-|
| `RAPIDJSON_SSE2` | Bool | `OFF` | Enable SSE2 support | Only relevant for "x86" platforms |
| `RAPIDJSON_SSE42` | Bool | `OFF` | Enable SSE4.2 support | Only relevant for "x86" and "AMD" 64-bit platforms |
| `RAPIDJSON_NEON` | Bool | `OFF` | Enable ARM NEON support | Only relevant for "arm" platforms |

## Documentation and examples options
| Option Name | Type | Default Value | Description | Conditions, if any |
|-|-|-|-|-|
| `OPENDAQ_BUILD_DOCUMENTATION` | Bool | `OFF` | Build Doxygen documentation | Doxygen must be installed to generate the documentation |
| `APP_ENABLE_EXAMPLE_APPS` | Bool | `OFF` | Enable example openDAQ applications | Requires `DAQMODULES_REF_FB_MODULE` and `DAQMODULES_REF_DEVICE_MODULE`<br> to be ON, otherwise, the applications build is skipped |
| `APP_ENABLE_AUDIO_APP` | Bool | `OFF` | Enable openDAQ audio application | Requires `DAQMODULES_AUDIO_DEVICE_MODULE` to be ON, otherwise, the audio application build is skipped |
| `APP_ENABLE_WEBPAGE_EXAMPLES` | Bool | `OFF` | Enable webpage examples | - |
| `DAQSIMULATOR_ENABLE_SIMULATOR_APP` | Bool | `OFF` | Enable device simulator application | Requires `OPENDAQ_ENABLE_OPCUA`,<br>`OPENDAQ_ENABLE_NATIVE_STREAMING`,<br>`DAQMODULES_OPENDAQ_SERVER_MODULE`,<br>`DAQMODULES_REF_FB_MODULE` and <br>`DAQMODULES_REF_DEVICE_MODULE`<br> to be ON, otherwise, the simulator application build is skipped |

## Other options
| Option Name | Type | Default Value | Description | Conditions, if any |
|-|-|-|-|-|
| `OPENDAQ_FORCE_COMPILE_32BIT` | Bool | `OFF` | Compile 32Bit on non MSVC | Only relevant for non-MSVC compiler |
| `OPENDAQ_MSVC_SINGLE_PROCESS_BUILD` | Bool | `OFF` | Do not include /MP compile option <br> Ensures that single-threaded builds are used in MSVC environments where multi-threading could cause issues <br> https://learn.microsoft.com/en-us/cpp/build/reference/mp-build-with-multiple-processes | Only relevant for MSVC |
| `OPENDAQ_LINK_RUNTIME_STATICALLY` | Bool | `OFF` | Link the C++ runtime statically (embed it) | - |
| `OPENDAQ_LINK_3RD_PARTY_LIBS_STATICALY` | Bool | `ON` | Link the 3rd party libraries statically (embedd it) | - |
| `OPENDAQ_DISABLE_DEBUG_POSTFIX` | Bool | `OFF` | Disable debug ('-debug') postfix | - |
| `OPENDAQ_DEBUG_WARNINGS_AS_ERRORS` | Bool | `OFF` | Treat debug warnings as errors | - |
| `OPENDAQ_RELEASE_WARNINGS_AS_ERRORS` | Bool | `ON` | Treat release warnings as errors | - |
| `OPENDAQ_USE_CCACHE` | Bool | `ON` | Use compiler cache driver if available | - |
| `OPENDAQ_FORCE_LLD_LINKER` | Bool | `OFF` | Force the use of the fast LLVM LLD linker | - |
