set(MODULE_NAME core_containers)

if (POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
endif()

set(SRC_PublicHeaders listobject.h
                      list_factory.h
                      listobject_factory.h
                      listptr.h
                      list_ptr.h

                      dictobject.h
                      dictobject_factory.h
                      dictptr.h
                      dict_ptr.h
)

set(SRC_PrivateHeaders dictobject_impl.h
                       dictobject_iterable_impl.h
                       dictobject_iterator_impl.h
                       listobject_impl.h
)

set(SRC_Cpp listobject_impl.cpp
            dictobject_impl.cpp
)

prepend_include(coretypes SRC_PublicHeaders)
prepend_include(coretypes SRC_PrivateHeaders)

add_library(${MODULE_NAME} INTERFACE)
add_library(${SDK_TARGET_NAMESPACE}::${MODULE_NAME} ALIAS ${MODULE_NAME})

target_include_directories(${MODULE_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                                    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
)

target_sources(${MODULE_NAME} INTERFACE ${SRC_PublicHeaders}
                                        ${SRC_PrivateHeaders}
                                        ${SRC_Cpp}
)

install(FILES ${SRC_PublicHeaders}
        DESTINATION include/coretypes
        COMPONENT ${SDK_NAME}_coretypes_Development
)
