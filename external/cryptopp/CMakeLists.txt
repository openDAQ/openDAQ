# cryptopp is not a cmake package and cannot be found with find_package();
# therefore we cannot use opendaq_depencency(). However we do it in the same way.

option(
    OPENDAQ_ALWAYS_FETCH_CRYPTOPP
    "Ignore any installed cryptopp and always build from source"
    ${OPENDAQ_ALWAYS_FETCH_DEPENDENCIES})

find_library(CRYPTOPP_LIB libcryptopp.a)
find_path(CRYPTOPP_INCLUDE_DIR cryptopp/sha.h)

if(CRYPTOPP_LIB AND CRYPTOPP_INCLUDE_DIR)

    add_library(cryptopp::cryptopp IMPORTED UNKNOWN GLOBAL)
    set_target_properties(cryptopp::cryptopp PROPERTIES IMPORTED_LOCATION ${CRYPTOPP_LIB})
    target_include_directories(cryptopp::cryptopp INTERFACE ${CRYPTOPP_INCLUDE_DIR})
    target_link_libraries(cryptopp::cryptopp INTERFACE ${CRYPTOPP_LIB})

else()

    set(CRYPTOPP_BUILD_TESTING OFF)
    FetchContent_Declare(cryptopp-cmake
        GIT_REPOSITORY  https://github.com/abdes/cryptopp-cmake
        GIT_TAG         2c384c28265a93358a2455e610e76393358794df)

    FetchContent_MakeAvailable(cryptopp-cmake)

endif()
