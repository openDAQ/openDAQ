set(LIB_NAME licensing_module)
set(MODULE_HEADERS_DIR ../include/${TARGET_FOLDER_NAME})


set(SRC_Include common.h
				module_dll.h
                licensing_module_impl.h
				license_checker_impl.h
				passthrough_fb_impl.h
)

set(SRC_Srcs module_dll.cpp
			 licensing_module_impl.cpp
			 license_checker_impl.cpp
			 passthrough_fb_impl.cpp
)

opendaq_prepend_include(${TARGET_FOLDER_NAME} SRC_Include)

source_group("module" FILES ${MODULE_HEADERS_DIR}/module_dll.h
                            ${MODULE_HEADERS_DIR}/licensing_module_impl.h
                            ${MODULE_HEADERS_DIR}/license_checker_impl.h
                            ${MODULE_HEADERS_DIR}/passthrough_fb_impl.h
                            module_dll.cpp
                            licensing_module_impl.cpp
                            license_checker_impl.cpp
                            passthrough_fb_impl.cpp
)

add_library(${LIB_NAME} SHARED ${SRC_Include}
                               ${SRC_Srcs}
)

add_library(${SDK_TARGET_NAMESPACE}::${LIB_NAME} ALIAS ${LIB_NAME})

if("${Boost_LIBRARIES}")
    set(BOOST_LIBS ${Boost_LIBRARIES})
endif()

if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE /bigobj)
endif()

target_link_libraries(${LIB_NAME}
        PUBLIC
            daq::opendaq
        PRIVATE
            ${BOOST_LIBS}
)

# Should link only what you need $<BUILD_INTERFACE:Boost::locale>
target_link_libraries(${LIB_NAME} PRIVATE $<BUILD_INTERFACE:Boost::locale>)

if (CMAKE_COMPILER_IS_GNUCXX AND MINGW)
    target_compile_options(${LIB_NAME} PRIVATE -Wno-error)
endif()

target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                                              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
                                              $<INSTALL_INTERFACE:include>
                                              ${Boost_INCLUDE_DIRS}
)

opendaq_set_module_properties(${LIB_NAME} ${PROJECT_VERSION_MAJOR})
opendaq_generate_version_header(${LIB_NAME})
