set_cmake_folder_context(TARGET_FOLDER_NAME)

message(STATUS "=== .NET Bindings ===")
message(STATUS "CMAKE_SOURCE_DIR               = '${CMAKE_SOURCE_DIR}'")
message(STATUS "CMAKE_CURRENT_BINARY_DIR       = '${CMAKE_CURRENT_BINARY_DIR}'")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR       = '${CMAKE_CURRENT_SOURCE_DIR}'")
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY = '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}'")
message(STATUS "---")
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
set(outFile ${outputBinDir}/${outputFileName})
set(dotnet_config --configuration $<CONFIG>_Build)
set(msbuild_parameters -p:NoWarn=1591 -p:CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY} -p:OPENDAQ_PACKAGE_VERSION=${OPENDAQ_PACKAGE_VERSION}) # suppress: Warning CS1591 Missing XML comment for publicly visible type or member xy
set(version_suffix --version-suffix "beta-\$\(SHORT_SHA\)")
set(dotnet_cmd_args ${projectFileName} ${dotnet_config} --framework net6.0 --verbosity normal ${msbuild_parameters} ${version_suffix})
set(dotnet_pack_args ${projectFileName} --no-build ${dotnet_config} ${msbuild_parameters} ${version_suffix})
set(cleanCmd ${CMAKE_COMMAND} -E rm -r -f ${outputBinDir}) # clear outputBinDir (no warning if it doesn't exist)
set(cmd dotnet build ${dotnet_cmd_args})        # https://learn.microsoft.com/en-us/dotnet/core/tools/dotnet-build
set(packCmd dotnet pack ${dotnet_pack_args})    # https://learn.microsoft.com/en-us/dotnet/core/tools/dotnet-pack

set(configEnabled "$<CONFIG:Debug,Release>")
set(configRelease "$<CONFIG:Release>")

message(STATUS "targetName                     = '${targetName}'")
message(STATUS "projectName                    = '${projectName}'")
message(STATUS "projectFileName                = '${projectFileName}'")
message(STATUS "outputFileName                 = '${outputFileName}'")
message(STATUS "fullProjectDir                 = '${fullProjectDir}'")
message(STATUS "platform                       = '${platform}'")
message(STATUS "bindingsSrcDir                 = '${bindingsSrcDir}'")
message(STATUS "outputBinDir                   = '${outputBinDir}'")
message(STATUS "outFile                        = '${outFile}'")
message(STATUS "cleanCmd                       = '${cleanCmd}'")
message(STATUS "cmd                            = '${cmd}'")
message(STATUS "packCmd                        = '${packCmd}'")
message(STATUS "configEnabled                  = '${configEnabled}'")
#-- [openDAQ.bindings.dotnet] targetName                     = 'DotNetBindings'
#-- [openDAQ.bindings.dotnet] projectName                    = 'openDAQ.NET'
#-- [openDAQ.bindings.dotnet] projectFileName                = 'openDAQ.NET.sln'
#-- [openDAQ.bindings.dotnet] outputFileName                 = 'openDAQ.NET.dll'
#-- [openDAQ.bindings.dotnet] fullProjectDir                 = 'C:/VS_NETProjects/C++/BlueBerry/openDAQ/bindings/dotnet/openDAQ.NET'
#-- [openDAQ.bindings.dotnet] platform                       = 'x64'
#-- [openDAQ.bindings.dotnet] bindingsSrcDir                 = 'C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/bindings/CSharp/core'
#-- [openDAQ.bindings.dotnet] outputBinDir                   = 'C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/bindings/CSharp/bin/x64/$<CONFIG>'
#-- [openDAQ.bindings.dotnet] outFile                        = 'C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/bindings/CSharp/bin/x64/$<CONFIG>/openDAQ.NET.dll'
#-- [openDAQ.bindings.dotnet] cleanCmd                       = 'C:/Program Files/Microsoft Visual Studio/2022/Professional/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe;-E;rm;-r;-f;C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/bindings/CSharp/bin/x64/$<CONFIG>'
#-- [openDAQ.bindings.dotnet] cmd                            = 'dotnet;build;openDAQ.NET.sln;--configuration;$<CONFIG>_Build;--framework;net6.0;--verbosity;normal;-p:NoWarn=1591;-p:CMAKE_RUNTIME_OUTPUT_DIRECTORY=C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/x64/msvc-22/full/bin;-p:OPENDAQ_PACKAGE_VERSION=2.1.0;--version-suffix;beta-$(SHORT_SHA)'
#-- [openDAQ.bindings.dotnet] packCmd                        = 'dotnet;pack;openDAQ.NET.sln;--no-build;--configuration;$<CONFIG>_Build;-p:NoWarn=1591;-p:CMAKE_RUNTIME_OUTPUT_DIRECTORY=C:/VS_NETProjects/C++/BlueBerry/openDAQ/build/x64/msvc-22/full/bin;-p:OPENDAQ_PACKAGE_VERSION=2.1.0;--version-suffix;beta-$(SHORT_SHA)'
#-- [openDAQ.bindings.dotnet] configEnabled                  = '$<CONFIG:Debug,Release>'
#-- [openDAQ.bindings.dotnet] === .NET Bindings end ===

#use all generated C# files as dependency (CONFIGURE_DEPENDS: list is re-evaluated on every build)
file(GLOB_RECURSE dependencyFiles LIST_DIRECTORIES true CONFIGURE_DEPENDS ${bindingsSrcDir}/*.cs)

# <CustomBuild>
add_custom_command(OUTPUT "$<${configEnabled}:${outFile}>"
                   COMMENT "$<${configEnabled}:[${targetName}] Building .NET Bindings (${platform}/$<CONFIG>)>"
                   COMMAND "$<${configEnabled}:${cmd}>"
                   WORKING_DIRECTORY "$<${configEnabled}:${fullProjectDir}>"
                   COMMAND_EXPAND_LISTS
                   VERBATIM
)

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

message(STATUS "Adding target dependencies: ${dependencyTargets}")

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
add_custom_command(TARGET ${targetName}
                   # Run after all other rules within the target have been executed.
                   POST_BUILD
                   COMMENT "$<${configRelease}:[${targetName}] Building NuGet package (${platform}/$<CONFIG>)>"
                   COMMAND "$<${configRelease}:${packCmd}>"
                   WORKING_DIRECTORY "$<${configRelease}:${fullProjectDir}>"
                   COMMAND_EXPAND_LISTS
                   VERBATIM
)

# this is somehow doing nothing when packing (CPack)
#install(DIRECTORY "${outputBinDir}/"
#        DESTINATION ${CMAKE_INSTALL_PREFIX}/bindings/dotnet
#        COMPONENT openDAQ_opendaq_Development
#)

message(STATUS "=== .NET Bindings end ===")
