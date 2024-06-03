set(BASE_NAME reader)

set(SDK_HEADERS_DIR ../include/${MAIN_TARGET})
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_HEADERS_DIR})

set(RTGEN_HEADERS_DIR ${SDK_HEADERS_DIR})
set(RTGEN_OUTPUT_DIR ${GENERATED_HEADERS_DIR})
set(RTGEN_OUTPUT_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR})

rtgen(SRC_ReaderStatus reader_status.h)
rtgen(SRC_BlockReaderStatus block_reader_status.h)
rtgen(SRC_TailReaderStatus tail_reader_status.h)
rtgen(SRC_MultiReaderStatus multi_reader_status.h)
rtgen(SRC_Reader reader.h)
rtgen(SRC_SampleReader sample_reader.h)
rtgen(SRC_ReaderConfig reader_config.h)
rtgen(SRC_StreamReader stream_reader.h)
rtgen(SRC_BlockReaderBuilder block_reader_builder.h INTERNAL)
rtgen(SRC_BlockReader block_reader.h)
rtgen(SRC_TailReader tail_reader.h)
rtgen(SRC_PacketReader packet_reader.h)
rtgen(SRC_MultiReaderBuilder multi_reader_builder.h INTERNAL)
rtgen(SRC_MultiReader multi_reader.h)

source_group("reader" FILES ${SDK_HEADERS_DIR}/reader_status.h
                            ${SDK_HEADERS_DIR}/block_reader_status.h
                            ${SDK_HEADERS_DIR}/tail_reader_status.h
                            ${SDK_HEADERS_DIR}/multi_reader_status.h
                            ${SDK_HEADERS_DIR}/reader.h
                            ${SDK_HEADERS_DIR}/sample_reader.h
                            ${SDK_HEADERS_DIR}/reader_config.h
                            ${SDK_HEADERS_DIR}/reader_errors.h
                            ${SDK_HEADERS_DIR}/reader_factory.h
                            ${SDK_HEADERS_DIR}/time_reader.h
                            ${SDK_HEADERS_DIR}/read_info.h
                            ${SDK_HEADERS_DIR}/typed_reader.h
                            ${SDK_HEADERS_DIR}/reader_impl.h
                            reader_impl.cpp
                            ${SDK_HEADERS_DIR}/reader_status_impl.h
                            reader_status_impl.cpp
                            typed_reader.cpp
)

source_group("stream" FILES ${SDK_HEADERS_DIR}/stream_reader.h
                            ${SDK_HEADERS_DIR}/stream_reader_ptr.h
                            ${SDK_HEADERS_DIR}/stream_reader_impl.h
                            stream_reader_impl.cpp
)

source_group("block" FILES ${SDK_HEADERS_DIR}/block_reader_builder.h
                           ${SDK_HEADERS_DIR}/block_reader_builder_impl.h
                           ${SDK_HEADERS_DIR}/block_reader.h
                           ${SDK_HEADERS_DIR}/block_reader_impl.h
                           block_reader_impl.cpp
                           block_reader_builder_impl.cpp
)

source_group("tail" FILES ${SDK_HEADERS_DIR}/tail_reader.h
                          ${SDK_HEADERS_DIR}/tail_reader_impl.h
                          tail_reader_impl.cpp
)

source_group("packet" FILES ${SDK_HEADERS_DIR}/packet_reader.h
                            ${SDK_HEADERS_DIR}/packet_reader_impl.h
                            packet_reader_impl.cpp
)

source_group("multi" FILES ${SDK_HEADERS_DIR}/multi_reader_builder.h
                           ${SDK_HEADERS_DIR}/multi_reader_builder_impl.h
                           ${SDK_HEADERS_DIR}/multi_reader.h
                           ${SDK_HEADERS_DIR}/signal_reader.h
                           ${SDK_HEADERS_DIR}/multi_reader_impl.h
                           ${SDK_HEADERS_DIR}/multi_typed_reader.h
                           ${SDK_HEADERS_DIR}/reader_domain_info.h
                           multi_reader_impl.cpp
                           multi_reader_builder_impl.cpp
                           signal_reader.cpp
)

set(SRC_Cpp stream_reader_impl.cpp
            block_reader_impl.cpp
            block_reader_builder_impl.cpp
            tail_reader_impl.cpp
            packet_reader_impl.cpp
            reader_status_impl.cpp
            reader_impl.cpp
            typed_reader.cpp
            multi_reader_impl.cpp
            multi_reader_builder_impl.cpp
            signal_reader.cpp
)

set(SRC_PublicHeaders reader_factory.h
                      reader_errors.h
                      reader_exceptions.h
                      typed_reader.h
                      time_reader.h
                      read_info.h
                      reader_utils.h
                      reader_domain_info.h
)

set(SRC_PrivateHeaders stream_reader_impl.h
                       block_reader_impl.h
                       block_reader_builder_impl.h
                       tail_reader_impl.h
                       packet_reader_impl.h
                       multi_reader_impl.h
                       multi_reader_builder_impl.h
                       multi_typed_reader.h
                       signal_reader.h
                       reader_status_impl.h
                       reader_impl.h
)

prepend_include(${MAIN_TARGET} SRC_PrivateHeaders)
prepend_include(${MAIN_TARGET} SRC_PublicHeaders)

list(APPEND SRC_Cpp ${SRC_Reader_Cpp}
                    ${SRC_SampleReader_Cpp}
                    ${SRC_ReaderConfig_Cpp}
                    ${SRC_StreamReader_Cpp}
                    ${SRC_BlockReader_Cpp}
                    ${SRC_TailReader_Cpp}
                    ${SRC_PacketReader_Cpp}
                    ${SRC_MultiReader_Cpp}
)

list(APPEND SRC_PublicHeaders ${SRC_ReaderStatus_PublicHeaders}
                              ${SRC_BlockReaderStatus_PublicHeaders}
                              ${SRC_TailReaderStatus_PublicHeaders}
                              ${SRC_MultiReaderStatus_PublicHeaders}
                              ${SRC_Reader_PublicHeaders}
                              ${SRC_SampleReader_PublicHeaders}
                              ${SRC_ReaderConfig_PublicHeaders}
                              ${SRC_StreamReader_PublicHeaders}
                              ${SRC_BlockReader_PublicHeaders}
                              ${SRC_BlockReaderBuilder_PublicHeaders}
                              ${SRC_TailReader_PublicHeaders}
                              ${SRC_PacketReader_PublicHeaders}
                              ${SRC_MultiReader_PublicHeaders}
                              ${SRC_MultiReaderBuilder_PublicHeaders}
                              reader.natvis
)

list(APPEND SRC_PrivateHeaders ${SRC_ReaderStatus_PrivateHeaders}
                               ${SRC_BlockReaderStatus_PrivateHeaders}
                               ${SRC_TailReaderStatus_PrivateHeaders}
                               ${SRC_MultiReaderStatus_PrivateHeaders}
                               ${SRC_Reader_PrivateHeaders}
                               ${SRC_SampleReader_PrivateHeaders}
                               ${SRC_ReaderConfig_PrivateHeaders}
                               ${SRC_StreamReader_PrivateHeaders}
                               ${SRC_BlockReader_PrivateHeaders}
                               ${SRC_BlockReaderBuilder_PrivateHeaders}
                               ${SRC_TailReader_PrivateHeaders}
                               ${SRC_PacketReader_PrivateHeaders}
                               ${SRC_MultiReader_PrivateHeaders}
                               ${SRC_MultiReaderBuilder_PrivateHeaders}
)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    add_compile_options(-UOPENDAQ_LOG_LEVEL -DOPENDAQ_LOG_LEVEL=$<IF:$<CONFIG:Debug>,0,${OPENDAQ_LOG_LEVEL_RELEASE_INT}>)
else()
    add_compile_definitions(OPENDAQ_LOG_LEVEL=$<IF:$<CONFIG:Debug>,0,${OPENDAQ_LOG_LEVEL_RELEASE_INT}>)
endif()

opendaq_add_library(${BASE_NAME} STATIC
    ${SRC_Cpp}
    ${SRC_PrivateHeaders}
    ${SRC_PublicHeaders}
    ${ConfigHeaderSource}
)

opendaq_target_link_libraries(${BASE_NAME}
    PUBLIC
        daq::coretypes
        daq::signal
        daq::module_manager
        daq::logger
        date::date
)

opendaq_target_include_directories(${BASE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include/>

        $<INSTALL_INTERFACE:include>
)
