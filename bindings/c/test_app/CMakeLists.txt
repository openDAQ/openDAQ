enable_language(C)

add_executable(test_app test_app.c)

target_link_libraries(test_app PRIVATE daq::copendaq)

target_include_directories(test_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_dependencies(test_app daq::copendaq)