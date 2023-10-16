set_cmake_folder_context(TARGET_FOLDER_NAME)

get_target_property(LIB_MAJOR_VERSION ${SDK_TARGET_NAMESPACE}::coretypes LIB_MAJOR_VERSION)

configure_file(OpenDAQ.CoreTypes.Config.pas.in OpenDAQ.CoreTypes.Config.pas)

add_custom_target(DelphiBindings ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/OpenDAQ.CoreTypes.Config.pas)

set(DELPHI_BINDINGS_SRC
    OpenDAQ.BinaryData.pas
    OpenDAQ.Convertible.pas
    OpenDAQ.Deserializer.pas
    OpenDAQ.Dict.pas
    OpenDAQ.Freezable.pas
    OpenDAQ.TFunction.pas
    OpenDAQ.Iterator.pas
    OpenDAQ.Iterable.pas
    OpenDAQ.List.pas
    OpenDAQ.Number.pas
    OpenDAQ.Boolean.pas
    OpenDAQ.Float.pas
    OpenDAQ.Integer.pas
    OpenDAQ.ObjectPtr.pas
    OpenDAQ.TProcedure.pas
    OpenDAQ.ProxyValue.pas
    OpenDAQ.Ratio.pas
    OpenDAQ.Serializable.pas
    OpenDAQ.SerializedList.pas
    OpenDAQ.SerializedObject.pas
    OpenDAQ.Serializer.pas
    OpenDAQ.SmartPtrRegistry.pas
    OpenDAQ.TString.pas
    OpenDAQ.Event.pas
    OpenDAQ.EventArgs.pas
    OpenDAQ.EventHandler.pas
    OpenDAQ.BaseObjectImpl.pas
    OpenDAQ.ProcedureImpl.pas
    OpenDAQ.FunctionImpl.pas
    OpenDAQ.CoreTypes.Errors.pas
    OpenDAQ.CoreTypes.pas
    OpenDAQ.Exceptions.pas
    OpenDAQ.VersionInfo.pas
    OpenDAQ.Updatable.pas
    ${CMAKE_CURRENT_BINARY_DIR}/OpenDAQ.CoreTypes.Config.pas
)

install(FILES ${DELPHI_BINDINGS_SRC}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bindings/delphi
)
