set(BASE_NAME security)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_CredentialProvider credential_provider.h)
    rtgen(SRC_CredentialRequest credential_request.h)
    rtgen(SRC_CredentialRequestBuilder credential_request_builder.h)
    rtgen(SRC_AuthenticationConfig authentication_config.h)
    rtgen(SRC_AuthenticationMethod authentication_method.h)
    rtgen(SRC_AuthenticationMethodInternal authentication_method_internal.h INTERNAL)
    rtgen(SRC_CredentialField credential_field.h)
    rtgen(SRC_LoginParameters login_parameters.h)
    rtgen(SRC_LoginParametersBuilder login_parameters_builder.h)

    set(SRC_PublicHeaders_Component_Generated
        ${SRC_CredentialProvider_PublicHeaders}
        ${SRC_CredentialRequest_PublicHeaders}
        ${SRC_CredentialRequestBuilder_PublicHeaders}
        ${SRC_AuthenticationConfig_PublicHeaders}
        ${SRC_AuthenticationMethod_PublicHeaders}
        ${SRC_AuthenticationMethodInternal_PublicHeaders}
        ${SRC_CredentialField_PublicHeaders}
        ${SRC_LoginParameters_PublicHeaders}
        ${SRC_LoginParametersBuilder_PublicHeaders}
        PARENT_SCOPE
    )

    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_CredentialProvider_PrivateHeaders}
        ${SRC_CredentialRequest_PrivateHeaders}
        ${SRC_CredentialRequestBuilder_PrivateHeaders}
        ${SRC_AuthenticationConfig_PrivateHeaders}
        ${SRC_AuthenticationMethod_PrivateHeaders}
        ${SRC_AuthenticationMethodInternal_PrivateHeaders}
        ${SRC_CredentialField_PrivateHeaders}
        ${SRC_LoginParameters_PrivateHeaders}
        ${SRC_LoginParametersBuilder_PrivateHeaders}
        PARENT_SCOPE
    )

    set(SRC_Cpp_Component_Generated
        ${SRC_CredentialProvider_Cpp}
        ${SRC_CredentialRequest_Cpp}
        ${SRC_CredentialRequestBuilder_Cpp}
        ${SRC_AuthenticationConfig_Cpp}
        ${SRC_AuthenticationMethod_Cpp}
        ${SRC_AuthenticationMethodInternal_Cpp}
        ${SRC_CredentialField_Cpp}
        ${SRC_LoginParameters_Cpp}
        ${SRC_LoginParametersBuilder_Cpp}
        PARENT_SCOPE
    )
endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "security/include/opendaq")
    set(SDK_SRC_DIR "security/src")

    source_group("security//credentials" FILES 
        ${SDK_HEADERS_DIR}/credential_provider.h
        ${SDK_HEADERS_DIR}/credential_provider_factory.h
        ${SDK_HEADERS_DIR}/credential_request.h
        ${SDK_HEADERS_DIR}/credential_request_factory.h
        ${SDK_HEADERS_DIR}/credential_request_impl.h
        ${SDK_HEADERS_DIR}/credential_request_builder.h
        ${SDK_HEADERS_DIR}/credential_request_builder_impl.h
        ${SDK_HEADERS_DIR}/cmd_line_credential_provider_impl.h
        ${SDK_HEADERS_DIR}/authentication_config.h
        ${SDK_HEADERS_DIR}/authentication_config_impl.h
        ${SDK_HEADERS_DIR}/authentication_method.h
        ${SDK_HEADERS_DIR}/authentication_method_internal.h
        ${SDK_HEADERS_DIR}/authentication_method_factory.h
        ${SDK_HEADERS_DIR}/authentication_method_impl.h
        ${SDK_HEADERS_DIR}/credential_field.h
        ${SDK_HEADERS_DIR}/credential_field_factory.h
        ${SDK_HEADERS_DIR}/credential_field_impl.h
        ${SDK_HEADERS_DIR}/login_parameters.h
        ${SDK_HEADERS_DIR}/login_parameters_factory.h
        ${SDK_HEADERS_DIR}/login_parameters_impl.h
        ${SDK_HEADERS_DIR}/login_parameters_builder.h
        ${SDK_HEADERS_DIR}/login_parameters_builder_impl.h
        ${SDK_SRC_DIR}/credential_request_impl.cpp
        ${SDK_SRC_DIR}/cmd_line_credential_provider_impl.cpp
        ${SDK_SRC_DIR}/credential_request_builder_impl.cpp
        ${SDK_SRC_DIR}/authentication_method_impl.cpp
        ${SDK_SRC_DIR}/authentication_config_impl.cpp
        ${SDK_SRC_DIR}/credential_field_impl.cpp
        ${SDK_SRC_DIR}/login_parameters_impl.cpp
        ${SDK_SRC_DIR}/login_parameters_builder_impl.cpp
    )

    source_group("security//errors" FILES
        ${SDK_HEADERS_DIR}/security_errors.h
        ${SDK_HEADERS_DIR}/security_exceptions.h
    )
endfunction()

set(SRC_PublicHeaders_Component
    credential_provider.h
    credential_provider_factory.h
    credential_request.h
    credential_request_builder.h
    credential_request_factory.h
    authentication_config.h
    authentication_method.h
    authentication_method_internal.h
    authentication_method_factory.h
    credential_field.h
    credential_field_factory.h
    login_parameters.h
    login_parameters_factory.h
    login_parameters_builder.h
    security_errors.h
    security_exceptions.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component
    credential_request_builder_impl.h
    credential_request_impl.h
    cmd_line_credential_provider_impl.h
    authentication_method_impl.h
    authentication_config_impl.h
    credential_field_impl.h
    login_parameters_impl.h
    login_parameters_builder_impl.h
    PARENT_SCOPE
)

set(SRC_Cpp_Component
    credential_request_impl.cpp
    cmd_line_credential_provider_impl.cpp
    credential_request_builder_impl.cpp
    authentication_method_impl.cpp
    authentication_config_impl.cpp
    credential_field_impl.cpp
    login_parameters_impl.cpp
    login_parameters_builder_impl.cpp
    PARENT_SCOPE
)
