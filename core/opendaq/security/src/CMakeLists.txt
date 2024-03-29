set(BASE_NAME security)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_User user.h)
rtgen(SRC_UserInternal user_internal.h)
rtgen(SRC_AuthenticationProvider authentication_provider.h)
rtgen(SRC_PermissionManager permission_manager.h)
rtgen(SRC_PermissionManagerInternal permission_manager_internal.h)
rtgen(SRC_PermisisonsBuilder permissions_builder.h)
rtgen(SRC_Permisisons permissions.h)

set(SRC_Cpp user_impl.cpp
            authentication_provider_impl.cpp
            permission_manager_impl.cpp
            permissions_builder_impl.cpp
            permissions_impl.cpp
)

set(SRC_PublicHeaders security_errors.h
                      security_exceptions.h
                      user.h
                      user_factory.h
                      user_internal.h
                      authentication_provider.h
                      authentication_provider_factory.h
                      permission_manager.h
                      permission_manager_factory.h
                      permission_manager_internal.h
                      permissions_builder.h
                      permissions_builder_factory.h
                      permissions.h
)

set(SRC_PrivateHeaders user_impl.h
                       authentication_provider_impl.h
                       permission_manager_impl.h
                       permissions_builder_impl.h
                       permissions_impl.h
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_Security_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_Security_PublicHeaders}
)

list(APPEND SRC_PrivateHeaders ${SRC_Security_PrivateHeaders}
)

opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PrivateHeaders}
    ${SRC_PublicHeaders}
    ${ConfigHeaderSource}
)

opendaq_target_link_libraries(${BASE_NAME}
    PUBLIC
        daq::coretypes
    PRIVATE
        $<BUILD_INTERFACE:Boost::algorithm>
        $<BUILD_INTERFACE:Boost::filesystem>
)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>
)
