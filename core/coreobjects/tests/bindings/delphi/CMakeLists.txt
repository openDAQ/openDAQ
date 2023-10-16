set_cmake_folder(TARGET_FOLDER_NAME)

set(TargetName "CoreObjectsUT")
set(ProjectFileName "CoreObjectsUT.dproj")
set(DprFileName "CoreObjectsUT.dpr")
set(OutputFileName "CoreObjectsUT.exe")
set(DepName "CoreObjectsUT.res")

set(CoreObjectsDelphiDir ../../../Bindings/Delphi)

set(DelphiUnitTestsDir ${CMAKE_SOURCE_DIR}/external/dsunittest)
set(CoreObjectsDelphiDirRelativeToBuild ${CMAKE_CURRENT_SOURCE_DIR}/${CoreObjectsDelphiDir})
set(CoreObjectsDelphiBinDirRelativeToBuild ${CMAKE_CURRENT_BINARY_DIR}/${CoreObjectsDelphiDir})

set(CoreObjectsSources
)

set(TestSources Test.OpenDAQ.PropertyObject.pas
)

set(FullProjectFileName ${CMAKE_CURRENT_BINARY_DIR}/${ProjectFileName})


file(TO_NATIVE_PATH ${DelphiUnitTestsDir} DelphiUnitTestsDir)
file(TO_NATIVE_PATH ${CoreObjectsDelphiDirRelativeToBuild} CoreObjectsDelphiDirRelativeToBuild)
file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR} TestSourceDir)
file(RELATIVE_PATH DelphiUnitTestsDir ${CMAKE_CURRENT_BINARY_DIR} ${DelphiUnitTestsDir})
file(RELATIVE_PATH CoreObjectsDelphiDirRelativeToBuild ${CMAKE_CURRENT_BINARY_DIR} ${CoreObjectsDelphiDirRelativeToBuild})
file(RELATIVE_PATH CoreObjectsDelphiBinDirRelativeToBuild ${CMAKE_CURRENT_BINARY_DIR} ${CoreObjectsDelphiBinDirRelativeToBuild})
file(RELATIVE_PATH TestSourceDir ${CMAKE_CURRENT_BINARY_DIR} ${TestSourceDir})

if(MSVC OR GENERATOR_IS_MULTI_CONFIG)
    set(MSVC_CXX "IS_MSVC;")
    set(OutputBinDir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
else()
    set(OutputBinDir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

# Get include folders from CoreTypes
get_target_property(CoreObjectsDelphiDependentIncludes daq::coretypes DelphiBindingsIncludes)

set(CoreObjectsDelphiIncludes ${CoreObjectsDelphiDirRelativeToBuild}
                              ${CoreObjectsDelphiBinDirRelativeToBuild}
                              ${CoreObjectsDelphiDependentIncludes}
)

# Set include folders for CoreTypes
set_property(TARGET daqcoreobjects
             PROPERTY DelphiBindingsIncludes
             ${CoreObjectsDelphiIncludes}
)

configure_file(${ProjectFileName}.in ${ProjectFileName})
configure_file(${DprFileName}.in ${DprFileName})

check_if_files_exist(${CMAKE_CURRENT_SOURCE_DIR} CoreObjectsSources)
check_if_files_exist(${CMAKE_CURRENT_SOURCE_DIR} TestSources)

set(InputFiles ${TestSources} ${CoreObjectsSources} ${CMAKE_CURRENT_BINARY_DIR}/${DprFileName})
set(FullOutputFileName ${OutputBinDir}/${OutputFileName})

if(BUILD_64Bit)
    set(Platform Win64)
else()
    set(Platform Win32)
endif()

set(OUT_NAME ${CMAKE_CURRENT_BINARY_DIR}/${DepName})

add_custom_command(OUTPUT ${OUT_NAME} # here we should use FullOutputFileName, but generator expressions not supported for output parameter
                   COMMAND ${DELPHI_RSVARS} && msbuild /nologo /v:m /p:config=$<CONFIG> /p:platform=${Platform} /t:build /p:DCC_ExeOutput=${OutputBinDir} /p:DCC_DcuOutput=${CMAKE_CURRENT_BINARY_DIR} ${FullProjectFileName}
                   MAIN_DEPENDENCY ${FullProjectFileName}
                   DEPENDS ${InputFiles}
                   COMMENT "Building Delphi unit tests for DaqCoreObjects"
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   VERBATIM
)

add_custom_target(${TargetName} ALL DEPENDS ${OUT_NAME})
add_dependencies(${TargetName} ${SDK_TARGET_NAMESPACE}::coreobjects
                               CoreObjectsDelphiBindings
)

add_test(NAME ${TargetName} COMMAND ${FullOutputFileName})
