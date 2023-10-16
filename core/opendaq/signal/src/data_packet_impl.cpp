#include <opendaq/data_packet_impl.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, DataPacketImpl<IDataPacket>,
    IDataPacket, createDataPacket,
    IDataDescriptor*, descriptor,
    SizeT, sampleCount,
    INumber*, offset,
    IAllocator*, allocator
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, DataPacketImpl<IDataPacket>,
    IDataPacket, createDataPacketWithDomain,
    IDataPacket*, domainPacket,
    IDataDescriptor*, descriptor,
    SizeT, sampleCount,
    INumber*, offset,
    IAllocator*, allocator
)

END_NAMESPACE_OPENDAQ
