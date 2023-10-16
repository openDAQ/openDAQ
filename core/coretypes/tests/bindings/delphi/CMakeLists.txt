set_cmake_folder_context(TARGET_FOLDER_NAME)

set(TargetName "CoreTypesUT")
set(ProjectFileName "CoreTypesUT.dproj")
set(DprFileName "CoreTypesUT.dpr")
set(OutputFileName "CoreTypesUT.exe")
set(DepName "CoreTypesUT.res")

set(CoreTypesDelphiDir ../../../Bindings/Delphi)

set(DelphiUnitTestsDir ${CMAKE_SOURCE_DIR}/external/dsunittest)
set(CoreTypesDelphiDirRelativeToBuild ${CMAKE_CURRENT_SOURCE_DIR}/${CoreTypesDelphiDir})
set(CoreTypesDelphiBinDirRelativeToBuild ${CMAKE_CURRENT_BINARY_DIR}/${CoreTypesDelphiDir})

set(CoreTypesDelphiIncludes ${CoreTypesDelphiDirRelativeToBuild}
                            ${CoreTypesDelphiBinDirRelativeToBuild}
)

set(CoreTypesSources ${CoreTypesDelphiDir}/OpenDAQ.CoreTypes.pas
                     ${CoreTypesDelphiDir}/OpenDAQ.BaseObjectImpl.pas
                     ${CoreTypesDelphiDir}/OpenDAQ.ProcedureImpl.pas
                     ${CoreTypesDelphiDir}/OpenDAQ.FunctionImpl.pas
                     ${CoreTypesDelphiDir}/OpenDAQ.CoreTypes.Errors.pas
)
set(TestSources Test.OpenDAQ.BaseObject.pas
                Test.OpenDAQ.BaseObjectImpl.pas
                Test.OpenDAQ.ProcedureImpl.pas
                Test.OpenDAQ.FunctionImpl.pas
                Test.OpenDAQ.BinaryData.pas
                Test.OpenDAQ.BinaryDataPtr.pas
                Test.OpenDAQ.Integer.pas
                Test.OpenDAQ.Float.pas
                Test.OpenDAQ.Boolean.pas
                Test.OpenDAQ.ConvertibleObject.pas
                Test.OpenDAQ.ConvertiblePtr.pas
                Test.OpenDAQ.StringObject.pas
                Test.OpenDAQ.StringPtr.pas
                Test.OpenDAQ.ListObject.pas
                Test.OpenDAQ.ListPtr.pas
                Test.OpenDAQ.DictObject.pas
                Test.OpenDAQ.DictPtr.pas
                Test.OpenDAQ.JsonSerializer.pas
                Test.OpenDAQ.SerializerPtr.pas
                Test.OpenDAQ.JsonDeserializer.pas
                Test.OpenDAQ.DeserializerPtr.pas
                Test.OpenDAQ.JsonSerializedList.pas
                Test.OpenDAQ.JsonSerializedListPtr.pas
                Test.OpenDAQ.JsonSerializedObject.pas
                Test.OpenDAQ.JsonSerializedObjectPtr.pas
                Test.OpenDAQ.ProxyValue.pas
                Test.OpenDAQ.Ratio.pas
                Test.OpenDAQ.RatioPtr.pas
)

set(FullProjectFileName ${CMAKE_CURRENT_BINARY_DIR}/${ProjectFileName})


file(TO_NATIVE_PATH ${DelphiUnitTestsDir} DelphiUnitTestsDir)
file(TO_NATIVE_PATH ${CoreTypesDelphiDirRelativeToBuild} CoreTypesDelphiDirRelativeToBuild)
file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR} TestSourceDir)
file(RELATIVE_PATH DelphiUnitTestsDir ${CMAKE_CURRENT_BINARY_DIR} ${DelphiUnitTestsDir})
file(RELATIVE_PATH CoreTypesDelphiDirRelativeToBuild ${CMAKE_CURRENT_BINARY_DIR} ${CoreTypesDelphiDirRelativeToBuild})
file(RELATIVE_PATH CoreTypesDelphiBinDirRelativeToBuild ${CMAKE_CURRENT_BINARY_DIR} ${CoreTypesDelphiBinDirRelativeToBuild})
file(RELATIVE_PATH TestSourceDir ${CMAKE_CURRENT_BINARY_DIR} ${TestSourceDir})

if(MSVC OR GENERATOR_IS_MULTI_CONFIG)
    set(MSVC_CXX "IS_MSVC;")
    set(OutputBinDir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
else()
    set(OutputBinDir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

set_property(TARGET ${SDK_TARGET_NAMESPACE}coretypes
             PROPERTY DelphiBindingsIncludes
             ${CoreTypesDelphiIncludes}
)

configure_file(${ProjectFileName}.in ${ProjectFileName})
configure_file(${DprFileName}.in ${DprFileName})

check_if_files_exist(${CMAKE_CURRENT_SOURCE_DIR} CoreTypesSources)
check_if_files_exist(${CMAKE_CURRENT_SOURCE_DIR} TestSources)

set(InputFiles ${TestSources} ${CoreTypesSources} ${CMAKE_CURRENT_BINARY_DIR}/${DprFileName})
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
                   COMMENT "Building Delphi unit tests for CoreTypes"
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   VERBATIM
)

add_custom_target(${TargetName} ALL DEPENDS ${OUT_NAME})
add_dependencies(${TargetName} ${SDK_TARGET_NAMESPACE}::coretypes
                               DelphiBindings
)

add_test(NAME ${TargetName} COMMAND ${FullOutputFileName})
