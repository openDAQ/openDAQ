set_cmake_folder_context(TARGET_FOLDER_NAME)

message(STATUS "=== .NET Bindings ===")
message(DEBUG "CMAKE_SOURCE_DIR               = '${CMAKE_SOURCE_DIR}'")
message(DEBUG "CMAKE_CURRENT_BINARY_DIR       = '${CMAKE_CURRENT_BINARY_DIR}'")
message(DEBUG "CMAKE_CURRENT_SOURCE_DIR       = '${CMAKE_CURRENT_SOURCE_DIR}'")
message(DEBUG "CMAKE_RUNTIME_OUTPUT_DIRECTORY = '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}'")
message(DEBUG "---")
#-- [openDAQ.bindings.dotnet] === .NET Bindings ===
#-- [openDAQ.bindings.dotnet] CMAKE_SOURCE_DIR               = 'C:/VS_NETProjects/C++/BlueBerry/openDAQ'
#-- [openDAQ.bindings.dotnet] CMAKE_CURRENT_BINARY_DIR       = 'C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/x64/msvc-22/full/bindings/dotnet'
#-- [openDAQ.bindings.dotnet] CMAKE_CURRENT_SOURCE_DIR       = 'C:/VS_NETProjects/C++/BlueBerry/openDAQ/bindings/dotnet'
#-- [openDAQ.bindings.dotnet] CMAKE_RUNTIME_OUTPUT_DIRECTORY = 'C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/x64/msvc-22/full/bin'
#-- [openDAQ.bindings.dotnet] ---

set(targetName "DotNetBindings")
set(projectName "openDAQ.NET")
set(projectFileName "${projectName}.sln")
set(outputFileName "${projectName}.dll")

set(fullProjectDir ${CMAKE_CURRENT_SOURCE_DIR}/${projectName})

if(BUILD_64Bit)
    set(platform x64)
else()
    set(platform x86)
endif()

if(MSVC OR GENERATOR_IS_MULTI_CONFIG)
#    set(MSVC_CXX "IS_MSVC;")
else()
    message(STATUS "no MSVC: returning early")
    return()
endif()

set(buildDir ${CMAKE_SOURCE_DIR}/build)
set(bindingsDir ${buildDir}/bindings/CSharp)
set(bindingsSrcDir ${bindingsDir}/core)
set(outputBinDir ${bindingsDir}/bin/${platform}/$<CONFIG>)
set(test_path ${bindingsDir}/test_results)
set(outFile ${outputBinDir}/${outputFileName})
set(nuget_config --configfile "./.nuget/NuGet.Config" --ignore-failed-sources) # "/property:Configuration=$<CONFIG>_Build")
set(dotnet_config --configuration $<CONFIG>_Build)
# suppress warnings:
# > CS1591: Missing XML comment for publicly visible type or member 'xyz'.
# > NU1801: The local source 'path' doesn't exist.
set(msbuild_parameters -p:NoWarn="CS1591,NU1801" -p:CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY} -p:OPENDAQ_PACKAGE_VERSION=${OPENDAQ_PACKAGE_VERSION})
set(dotnet_cmd_args ${projectFileName} ${dotnet_config} --verbosity minimal ${nuget_config} ${msbuild_parameters})
set(dotnet_test_args ${projectFileName} --no-build ${dotnet_config} --logger """junit\\\;LogFilePath=${test_path}/test_${targetName}.xml""")
#set(dotnet_pack_args ${projectFileName} --no-build ${dotnet_config} ${msbuild_parameters})
set(cleanCmd ${CMAKE_COMMAND} -E rm -r -f ${outputBinDir})         # clear outputBinDir (no warning if it doesn't exist)
#set(restoreCmd dotnet restore ${projectFileName} ${nuget_config})  # https://learn.microsoft.com/en-us/dotnet/core/tools/dotnet-restore
set(buildCmd dotnet build ${dotnet_cmd_args})                      # https://learn.microsoft.com/en-us/dotnet/core/tools/dotnet-build
set(testCmd dotnet test ${dotnet_test_args})                       # https://learn.microsoft.com/en-us/dotnet/core/tools/dotnet-test
#set(packCmd dotnet pack ${dotnet_pack_args})                       # https://learn.microsoft.com/en-us/dotnet/core/tools/dotnet-pack

set(configEnabled "$<CONFIG:Debug,Release>")
set(configRelease "$<CONFIG:Release>")

message(DEBUG "targetName                     = '${targetName}'")
message(DEBUG "projectName                    = '${projectName}'")
message(DEBUG "projectFileName                = '${projectFileName}'")
message(DEBUG "outputFileName                 = '${outputFileName}'")
message(DEBUG "fullProjectDir                 = '${fullProjectDir}'")
message(DEBUG "platform                       = '${platform}'")
message(DEBUG "bindingsSrcDir                 = '${bindingsSrcDir}'")
message(DEBUG "outputBinDir                   = '${outputBinDir}'")
message(DEBUG "outFile                        = '${outFile}'")
message(DEBUG "test_path                      = '${test_path}'")
#message(DEBUG "SHORT_SHA                      = '$ENV{SHORT_SHA}'")
message(DEBUG "cleanCmd                       = '${cleanCmd}'")
#message(DEBUG "restoreCmd                     = '${restoreCmd}'")
message(DEBUG "buildCmd                       = '${buildCmd}'")
message(DEBUG "testCmd                        = '${testCmd}'")
#message(DEBUG "packCmd                        = '${packCmd}'")
message(DEBUG "configEnabled                  = '${configEnabled}'")
#-- [openDAQ.bindings.dotnet] targetName                     = 'DotNetBindings'
#-- [openDAQ.bindings.dotnet] projectName                    = 'openDAQ.NET'
#-- [openDAQ.bindings.dotnet] projectFileName                = 'openDAQ.NET.sln'
#-- [openDAQ.bindings.dotnet] outputFileName                 = 'openDAQ.NET.dll'
#-- [openDAQ.bindings.dotnet] fullProjectDir                 = 'C:/VS_NETProjects/C++/BlueBerry/openDAQ/bindings/dotnet/openDAQ.NET'
#-- [openDAQ.bindings.dotnet] platform                       = 'x64'
#-- [openDAQ.bindings.dotnet] bindingsSrcDir                 = 'C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/bindings/CSharp/core'
#-- [openDAQ.bindings.dotnet] outputBinDir                   = 'C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/bindings/CSharp/bin/x64/$<CONFIG>'
#-- [openDAQ.bindings.dotnet] outFile                        = 'C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/bindings/CSharp/bin/x64/$<CONFIG>/openDAQ.NET.dll'
#-- [openDAQ.bindings.dotnet] test_path                      = 'C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/bindings/CSharp/test_results'
#-- [openDAQ.bindings.dotnet] cleanCmd                       = 'C:/Program Files/Microsoft Visual Studio/2022/Professional/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe;-E;rm;-r;-f;C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/bindings/CSharp/bin/x64/$<CONFIG>'
#-- [openDAQ.bindings.dotnet] buildCmd                       = 'dotnet;build;openDAQ.NET.sln;--configuration;$<CONFIG>_Build;--framework;net6.0;--verbosity;minimal;--configfile;./.nuget/NuGet.Config;--ignore-failed-sources;-p:NoWarn=1591;-p:CMAKE_RUNTIME_OUTPUT_DIRECTORY=C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/x64/msvc-22/full/bin;-p:OPENDAQ_PACKAGE_VERSION=3.4.0'
#-- [openDAQ.bindings.dotnet] testCmd                        = 'dotnet;test;openDAQ.NET.sln;--no-build;--configuration;$<CONFIG>_Build;--logger;junit\;LogFilePath=C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/bindings/CSharp/test_results/test_DotNetBindings.xml'
#-- [openDAQ.bindings.dotnet] configEnabled                  = '$<CONFIG:Debug,Release>'
#-- [openDAQ.bindings.dotnet] Adding target dependencies: daq::coretypes;daq::coreobjects;daq::opendaq;opcua_client_module;ws_stream_cl_module;native_stream_cl_module;opcua_server_module;ws_stream_srv_module;native_stream_srv_module;empty_module;ref_device_module;ref_fb_module;audio_device_module
#-- [openDAQ.bindings.dotnet] === .NET Bindings end ===

#use all generated C# files as dependency (CONFIGURE_DEPENDS: list is re-evaluated on every build)
file(GLOB_RECURSE dependencyFiles LIST_DIRECTORIES true CONFIGURE_DEPENDS ${bindingsSrcDir}/*.cs)

# <CustomBuild>
add_custom_command(OUTPUT "$<${configEnabled}:${outFile}>"
                   COMMENT "$<${configEnabled}:[${targetName}] Building .NET Bindings (${platform}/$<CONFIG>)>"
                   COMMAND "$<${configEnabled}:${buildCmd}>"
                   WORKING_DIRECTORY "$<${configEnabled}:${fullProjectDir}>"
                   COMMAND_EXPAND_LISTS
                   VERBATIM
)
#                   COMMAND "$<${configEnabled}:${restoreCmd}>"

add_custom_target(${targetName} ALL
                  COMMENT "[${targetName}] target (${platform}/$<CONFIG>)"
                  DEPENDS "$<${configEnabled}:${outFile}>"
                          dependencyFiles
)

#use all modules as dependency
set(dependencyTargets ${SDK_TARGET_NAMESPACE}::coretypes
                      ${SDK_TARGET_NAMESPACE}::coreobjects
                      ${SDK_TARGET_NAMESPACE}::opendaq
)
add_cmake_targets(${CMAKE_SOURCE_DIR}/modules/. dependencyTargets)

message(DEBUG "Adding target dependencies: ${dependencyTargets}")

add_dependencies(${targetName} ${dependencyTargets}
                               ${SDK_TARGET_NAME}
)

# <PreBuildEvent>
add_custom_command(TARGET ${targetName}
                   # On Visual Studio Generators, run before any other rules are executed within the target. On other generators, run just before PRE_LINK commands
                   PRE_BUILD
                   COMMENT "[${targetName}] cleanup '${outputBinDir}' (${platform}/$<CONFIG>)"
                   COMMAND "$<${configEnabled}:${cleanCmd}>"
                   COMMAND_EXPAND_LISTS
                   VERBATIM
)

# <PostBuildEvent>
#message(DEBUG "Adding POST_BUILD (dotnet pack)")
#add_custom_command(TARGET ${targetName}
#                   # Run after all other rules within the target have been executed.
#                   POST_BUILD
#                   COMMENT "$<${configRelease}:[${targetName}] Building NuGet package (${platform}/$<CONFIG>)>"
#                   COMMAND "$<${configRelease}:${packCmd}>"
#                   WORKING_DIRECTORY "$<${configRelease}:${fullProjectDir}>"
#                   COMMAND_EXPAND_LISTS
#                   VERBATIM
#)

if(OPENDAQ_ENABLE_CSHARP_BINDINGS_TESTS)
    message(DEBUG "Adding 'test_${targetName}' (dotnet test)")
    add_test(NAME "test_${targetName}"
             COMMAND "$<${configEnabled}:${testCmd}>"
             WORKING_DIRECTORY "$<${configEnabled}:${fullProjectDir}>"
             COMMAND_EXPAND_LISTS
    )
endif()

# for packing (CPack)
install(DIRECTORY "${outputBinDir}/"
        DESTINATION bindings/dotnet
)

message(STATUS "=== .NET Bindings end ===")
