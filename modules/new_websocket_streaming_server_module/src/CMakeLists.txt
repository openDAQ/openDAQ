# Windows NSIS package manager limits lenght of variables
# "openDAQ_[MODULE_NAME]_[SUFFIX]" with 60 characters max
# The suffix with max lenght is [Development_was_installed]
# so the shorter module name [ws_stream_srv_module] should be used instead of full name:
# [ws_stream_srv_module]
set(LIB_NAME new_ws_stream_srv_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})

file(GLOB_RECURSE SRC_Include ../include/*.h ../include/*.hpp)
file(GLOB_RECURSE SRC_Srcs *.cpp)

source_group("module" FILES ${SRC_Include} ${SRC_Srcs})
add_library(${LIB_NAME} SHARED ${SRC_Include} ${SRC_Srcs})

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

target_link_libraries(${LIB_NAME}
    PUBLIC
        daq::opendaq
    PRIVATE
        Boost::asio
        Boost::headers
        Boost::uuid
        nlohmann_json::nlohmann_json
)

if (TARGET Boost::endian)
    target_link_libraries(${LIB_NAME} PRIVATE Boost::endian)
endif ()

if(MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /bigobj)
endif()

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/base64>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/streaming>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
create_version_header(${LIB_NAME})
