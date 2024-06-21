set(BASE_NAME reader)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_ReaderStatus reader_status.h)
    rtgen(SRC_BlockReaderStatus block_reader_status.h)
    rtgen(SRC_Reader reader.h)
    rtgen(SRC_SampleReader sample_reader.h)
    rtgen(SRC_ReaderConfig reader_config.h)
    rtgen(SRC_StreamReader stream_reader.h)
    rtgen(SRC_StreamReaderBuilder stream_reader_builder.h INTERNAL)
    rtgen(SRC_BlockReader block_reader.h)
    rtgen(SRC_BlockReaderBuilder block_reader_builder.h INTERNAL)
    rtgen(SRC_TailReader tail_reader.h)
    rtgen(SRC_PacketReader packet_reader.h)
    rtgen(SRC_MultiReaderBuilder multi_reader_builder.h INTERNAL)
    rtgen(SRC_MultiReader multi_reader.h)
    rtgen(SRC_TailReaderStatus tail_reader_status.h)
    rtgen(SRC_MultiReaderStatus multi_reader_status.h)
    
    set(SRC_PublicHeaders_Component_Generated
        ${SRC_ReaderStatus_PublicHeaders}
        ${SRC_BlockReaderStatus_PublicHeaders}
        ${SRC_Reader_PublicHeaders}
        ${SRC_SampleReader_PublicHeaders}
        ${SRC_ReaderConfig_PublicHeaders}
        ${SRC_StreamReader_PublicHeaders}
        ${SRC_StreamReaderBuilder_PublicHeaders}
        ${SRC_BlockReader_PublicHeaders}
        ${SRC_BlockReaderBuilder_PublicHeaders}
        ${SRC_TailReader_PublicHeaders}
        ${SRC_TailReaderBuilder_PublicHeaders}
        ${SRC_PacketReader_PublicHeaders}
        ${SRC_MultiReader_PublicHeaders}
        ${SRC_MultiReaderBuilder_PublicHeaders}
        ${SRC_TailReaderStatus_PublicHeaders}
        ${SRC_MultiReaderStatus_PublicHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_PrivateHeaders_Component_Generated
        ${SRC_ReaderStatus_PrivateHeaders}
        ${SRC_BlockReaderStatus_PrivateHeaders}
        ${SRC_Reader_PrivateHeaders}
        ${SRC_SampleReader_PrivateHeaders}
        ${SRC_ReaderConfig_PrivateHeaders}
        ${SRC_StreamReader_PrivateHeaders}
        ${SRC_StreamReaderBuilder_PrivateHeaders}
        ${SRC_BlockReader_PrivateHeaders}
        ${SRC_BlockReaderBuilder_PrivateHeaders}
        ${SRC_TailReader_PrivateHeaders}
        ${SRC_PacketReader_PrivateHeaders}
        ${SRC_MultiReader_PrivateHeaders}
        ${SRC_MultiReaderBuilder_PrivateHeaders}
        PARENT_SCOPE
    )
    
    set(SRC_Cpp_Component_Generated
        ${SRC_Reader_Cpp}
        ${SRC_SampleReader_Cpp}
        ${SRC_ReaderConfig_Cpp}
        ${SRC_StreamReader_Cpp}
        ${SRC_BlockReader_Cpp}
        ${SRC_TailReader_Cpp}
        ${SRC_PacketReader_Cpp}
        ${SRC_MultiReader_Cpp}
        PARENT_SCOPE
    )
endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "reader/include/opendaq")
    set(SDK_SRC_DIR "reader/src")
    
    source_group("reader//reader" FILES 
        ${SDK_HEADERS_DIR}/reader_status.h
        ${SDK_HEADERS_DIR}/reader.h
        ${SDK_HEADERS_DIR}/sample_reader.h
        ${SDK_HEADERS_DIR}/reader_config.h
        ${SDK_HEADERS_DIR}/reader_errors.h
        ${SDK_HEADERS_DIR}/reader_factory.h
        ${SDK_HEADERS_DIR}/time_reader.h
        ${SDK_HEADERS_DIR}/read_info.h
        ${SDK_HEADERS_DIR}/typed_reader.h
        ${SDK_HEADERS_DIR}/reader_impl.h
        ${SDK_HEADERS_DIR}/reader_status_impl.h
        ${SDK_SRC_DIR}/reader_impl.cpp
        ${SDK_SRC_DIR}/reader_status_impl.cpp
        ${SDK_SRC_DIR}/typed_reader.cpp
    )
    
    source_group("reader//stream" FILES 
        ${SDK_HEADERS_DIR}/stream_reader.h
        ${SDK_HEADERS_DIR}/stream_reader_builder.h
        ${SDK_HEADERS_DIR}/stream_reader_ptr.h
        ${SDK_HEADERS_DIR}/stream_reader_impl.h
        ${SDK_HEADERS_DIR}/stream_reader_builder_impl.h
        ${SDK_SRC_DIR}/stream_reader_impl.cpp
        ${SDK_SRC_DIR}/stream_reader_builder_impl.cpp
    )
    
    source_group("reader//block" FILES 
        ${SDK_HEADERS_DIR}/block_reader.h
        ${SDK_HEADERS_DIR}/block_reader_builder.h
        ${SDK_HEADERS_DIR}/block_reader_impl.h
        ${SDK_HEADERS_DIR}/block_reader_builder_impl.h
		${SDK_HEADERS_DIR}/block_reader_status.h
        ${SDK_SRC_DIR}/block_reader_impl.cpp
        ${SDK_SRC_DIR}/block_reader_builder_impl.cpp
    )
    
    source_group("reader//tail" FILES 
        ${SDK_HEADERS_DIR}/tail_reader.h
        ${SDK_HEADERS_DIR}/tail_reader_impl.h
		${SDK_HEADERS_DIR}/tail_reader_status.h
        ${SDK_SRC_DIR}/tail_reader_impl.cpp
    )
    
    source_group("reader//packet" FILES 
        ${SDK_HEADERS_DIR}/packet_reader.h
        ${SDK_HEADERS_DIR}/packet_reader_impl.h
        ${SDK_SRC_DIR}/packet_reader_impl.cpp
    )
    
    source_group("reader//multi" FILES 
        ${SDK_HEADERS_DIR}/multi_reader_builder.h
        ${SDK_HEADERS_DIR}/multi_reader_builder_impl.h
        ${SDK_HEADERS_DIR}/multi_reader.h
        ${SDK_HEADERS_DIR}/signal_reader.h
        ${SDK_HEADERS_DIR}/multi_reader_impl.h
        ${SDK_HEADERS_DIR}/multi_typed_reader.h
        ${SDK_HEADERS_DIR}/reader_domain_info.h
		${SDK_HEADERS_DIR}/multi_reader_status.h
        ${SDK_SRC_DIR}/multi_reader_impl.cpp
        ${SDK_SRC_DIR}/multi_reader_builder_impl.cpp
        ${SDK_SRC_DIR}/signal_reader.cpp
    )
endfunction()

set(SRC_PublicHeaders_Component 
    reader_factory.h
    reader_errors.h
    reader_exceptions.h
    typed_reader.h
    time_reader.h
    read_info.h
    reader_utils.h
    reader_domain_info.h
    PARENT_SCOPE
)

set(SRC_PrivateHeaders_Component
    stream_reader_impl.h
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
    PARENT_SCOPE
)

set(SRC_Cpp_Component 
    stream_reader_impl.cpp
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
    reader.natvis
    PARENT_SCOPE
)

set(SRC_PublicLinkLibraries_Component
    date::date
    PARENT_SCOPE
)
