SET(MODULE_NAME opcuaclient)

set(SOURCE_CPPS opcuaclient.cpp
                opcuatimertaskcontextcollection.cpp
                opcuatimertaskhelper.cpp
                opcuanodefactory.cpp
                opcuaasyncexecthread.cpp
                opcuareadvalueid.cpp
                opcuacallmethodrequest.cpp
                subscriptions.cpp
                monitored_item_create_request.cpp
                event_filter.cpp
                browse_request.cpp
                reference_utils.cpp
                request_handler.cpp
                attribute_reader.cpp
                cached_reference_browser.cpp
)

set(SOURCE_BROWSER_CPPS browser/opcuanodevisitor.cpp
                        browser/opcuanodefactorybrowser.cpp
                        browser/opcuabrowser.cpp)

set(SOURCE_TASKPROCESSOR_CPPS taskprocessor/opcuataskprocessor.cpp)

set(SOURCE_HEADERS opcuaclient.h
                   opcuatimertaskcontextcollection.h
                   opcuatimertaskhelper.h
                   opcuanodefactory.h
                   opcuaasyncexecthread.h
                   opcuareadvalueid.h
                   opcuacallmethodrequest.h
                   subscriptions.h
                   monitored_item_create_request.h
                   event_filter.h
                   browse_request.h
                   reference_utils.h
                   request_handler.h
                   attribute_reader.h
)

set(SOURCE_BROWSER_HEADERS
    browser/opcuanodevisitor.h
    browser/opcuanodefactorybrowser.h
    browser/opcuabrowser.h)

set(SOURCE_TASKPROCESSOR_HEADERS taskprocessor/opcuataskprocessor.h)

set(INCLUDE_DIR ../include/${MODULE_NAME})

prepend_include(${INCLUDE_DIR} SOURCE_HEADERS)
prepend_include(${INCLUDE_DIR} SOURCE_BROWSER_HEADERS)
prepend_include(${INCLUDE_DIR} SOURCE_TASKPROCESSOR_HEADERS)

set(ALL_SOURCES ${SOURCE_CPPS}
                ${SOURCE_BROWSER_CPPS}
                ${SOURCE_TASKPROCESSOR_CPPS}
                ${SOURCE_HEADERS}
                ${SOURCE_BROWSER_HEADERS}
                ${SOURCE_TASKPROCESSOR_HEADERS}
)

set(SOURCE_FILES ${ALL_SOURCES})
add_library(${MODULE_NAME} STATIC ${ALL_SOURCES})
add_library(${SDK_TARGET_NAMESPACE}::${MODULE_NAME} ALIAS ${MODULE_NAME})

target_include_directories(${MODULE_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>
                                                 $<INSTALL_INTERFACE:include/${MODULE_NAME}>
)

target_link_libraries(${MODULE_NAME} PUBLIC daq::opcuashared
                                            daq::opendaq_utils
                                            tsl::ordered_map
)

if(BUILD_64Bit)
    set_target_properties(${MODULE_NAME} PROPERTIES OUTPUT_NAME "${MODULE_NAME}64")
endif()

if(BUILD_64Bit OR BUILD_ARM)
    set_target_properties(${MODULE_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
    set_target_properties(${MODULE_NAME} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
endif()

if (MSVC)
    source_group("Header Files\\Browser" "browser/.+[.]h")
    source_group("Source Files\\Browser" "browser/.+[.]cpp")

    source_group("Header Files\\Taskprocessor" "taskprocessor/.+[.]h")
    source_group("Source Files\\Taskprocessor" "taskprocessor/.+[.]cpp")
endif()
