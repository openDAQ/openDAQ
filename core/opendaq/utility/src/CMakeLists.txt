set(BASE_NAME utility)

function(rtgen_component_${BASE_NAME})
    rtgen(SRC_PacketBuffer packet_buffer.h
    )

    rtgen(SRC_PacketBufferBuilder packet_buffer_builder.h
    )

    set(SRC_PublicHeader_PacketBuffer_Generated
	${SRC_PacketBuffer_PublicHeaders}
	${SRC_PacketBufferBuilder_PublicHeaders}
	PARENT_SCOPE
    )

    set(SRC_PrivateHeader_PacketBuffer_Generated
	${SRC_PacketBuffer_PrivateHeaders}
	${SRC_PacketBufferBuilder_PrivateHeaders}
	PARENT_SCOPE
    )

    set(SRC_Cpp_Packet_BufferGenerated
	${SRC_PacketBuffer_Cpp}
	${SRC_PacketBufferBuilder_Cpp}
	PARENT_SCOPE
    )

endfunction()

function(create_component_source_groups_${BASE_NAME})
    set(SDK_HEADERS_DIR "utility/include/opendaq")
    set(SDK_SRC_DIR "utility/src")
    
    source_group("utility//ids_parser" FILES 
        ${SDK_HEADERS_DIR}/ids_parser.h
        ${SDK_SRC_DIR}/ids_parser.cpp
    )

    source_group("utility//circular_packet" FILES
        ${SDK_HEADERS_DIR}/circular_packet.h
        ${SDK_SRC_DIR}/circular_packet.cpp
    )

    source_group("utility//packet_buffer" FILES
	#${SDK_HEADERS_DIR}/circular_packet.h
	#${SDK_SRC_DIR}/circular_packet.cpp
	${SDK_HEADERS_DIR}/packet_buffer.h
	${SDK_HEADERS_DIR}/packet_buffer_impl.h
	${SDK_HEADERS_DIR}/packet_buffer_factory.h
	${SDK_HEADERS_DIR}/packet_buffer_builder.h
	${SDK_HEADERS_DIR}/packet_buffer_builder_impl.h
	${SDK_SRC_DIR}/packet_buffer_impl.cpp
	#${SDK_SRC_DIR}/packet_buffer.cpp
    )
    
    source_group("utility" FILES 
        ${SDK_HEADERS_DIR}/thread_name.h
        ${SDK_SRC_DIR}/thread_name.cpp
        ${SDK_HEADERS_DIR}/utility_errors.h
        ${SDK_HEADERS_DIR}/utility_exceptions.h
        ${SDK_SRC_DIR}/utility.natvis
    )
endfunction()

set(SRC_Cpp_Component 
    ids_parser.cpp
    #packet_buffer.cpp
    packet_buffer_impl.cpp
    packet_buffer_builder_impl.cpp
    circular_packet.cpp
    thread_name.cpp
    utility.natvis
    PARENT_SCOPE
)

set(SRC_PublicHeaders_Component 
    utility_errors.h
    utility_exceptions.h
    ids_parser.h
    circular_packet.h
    packet_buffer.h
    packet_buffer_impl.h
    packet_buffer_factory.h
    packet_buffer_builder.h
    packet_buffer_builder_impl.h
    mem_pool_allocator.h
    thread_name.h
    PARENT_SCOPE
)

set(SRC_PrivateLinkLibraries_Component
    $<BUILD_INTERFACE:Boost::algorithm>
    $<BUILD_INTERFACE:daq::opendaq_utils>
    PARENT_SCOPE
)