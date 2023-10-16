set(MODULE_NAME signal_generator)
list(APPEND CMAKE_MESSAGE_CONTEXT ${MODULE_NAME})

set(SOURCE_CPPS signal_generator.cpp
)

set(SOURCE_HEADERS signal_generator.h
)

prepend_include(${MODULE_NAME} SOURCE_HEADERS)

add_library(${MODULE_NAME} STATIC ${SOURCE_CPPS} ${SOURCE_HEADERS})
add_library(${SDK_TARGET_NAMESPACE}::${MODULE_NAME} ALIAS ${MODULE_NAME})

target_include_directories(${MODULE_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>

                                              $<INSTALL_INTERFACE:include>
)

target_link_libraries(${MODULE_NAME} PUBLIC daq::signal
                                            daq::coretypes
)

