include(FetchContent)

find_package(${OPENDAQ_SDK_NAME} GLOBAL)
if (NOT ${OPENDAQ_SDK_NAME}_FOUND)
    if (NOT DEFINED OPENDAQ_REF)
        message(FATAL_ERROR "Building ${REPO_NAME} standalone without ${OPENDAQ_SDK_NAME} project git reference specified; tip: use '-DOPENDAQ_REF=' to set it")
    endif()

    set(OPENDAQ_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        ${OPENDAQ_SDK_NAME}
        GIT_REPOSITORY https://github.com/openDAQ/openDAQ.git
        GIT_TAG ${OPENDAQ_REF}
        GIT_PROGRESS   ON
        SOURCE_DIR ${FETCHCONTENT_EXTERNALS_DIR}/src/${OPENDAQ_SDK_NAME}
    )
    FetchContent_MakeAvailable(${OPENDAQ_SDK_NAME})
else()
    message(STATUS "Found installed ${OPENDAQ_SDK_NAME} version: ${${OPENDAQ_SDK_NAME}_VERSION}")
endif()
