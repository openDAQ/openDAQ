#include <opendaq/binary_data_packet_impl.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, BinaryDataPacketImpl<false>,
    IDataPacket, createBinaryDataPacket,
    IDataPacket*, domainPacket,
    IDataDescriptor*, descriptor,
    SizeT, sampleMemSize
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, BinaryDataPacketImpl<true>,
    IDataPacket, createBinaryDataPacketWithExternalMemory,
    IDataPacket*, domainPacket,
    IDataDescriptor*, descriptor,
    SizeT, sampleMemSize,
    void*, data,
    IDeleter*, deleter
)

END_NAMESPACE_OPENDAQ
