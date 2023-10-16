set(EXTERNAL_LIB rapidjson)

add_library(${EXTERNAL_LIB} INTERFACE)

target_compile_features(${EXTERNAL_LIB} INTERFACE cxx_std_17)
target_compile_definitions(${EXTERNAL_LIB} INTERFACE RAPIDJSON_HAS_STDSTRING
                                                     RAPIDJSON_PARSE_DEFAULT_FLAGS=256
                                                     RAPIDJSON_WRITE_DEFAULT_FLAGS=2
)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
    option(RAPIDJSON_SSE2 "Enable SSE2 support" OFF)
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
    option(RAPIDJSON_SSE42 "Enable SSE4.2 support" OFF)

    set(RAPIDJSON_SSE2 ON CACHE BOOL "" FORCE)
endif()

if (RAPIDJSON_SSE42)
    message(STATUS "RapidJSON: Enabling SSE4.2")
    target_compile_definitions(${EXTERNAL_LIB} INTERFACE RAPIDJSON_SSE42)
elseif (RAPIDJSON_SSE2)
    message(STATUS "RapidJSON: Enabling SSE2")
    target_compile_definitions(${EXTERNAL_LIB} INTERFACE RAPIDJSON_SSE2)
endif()

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm*")
    option(RAPIDJSON_NEON "Enable ARM NEON support" OFF)
endif()

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR RAPIDJSON_NEON)
    target_compile_definitions(${EXTERNAL_LIB} INTERFACE RAPIDJSON_NEON)
    message(STATUS "RapidJSON: Enabling NEON")
endif()

target_include_directories(${EXTERNAL_LIB} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# type-limits (comparison out of range) warning on aarch64 (causes build error with -Werror)
if(BUILD_ARM)
    target_compile_options(${EXTERNAL_LIB} INTERFACE -Wno-error=type-limits)
endif()
