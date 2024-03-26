#include <opendaq/data_packet_impl.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, DataPacketImpl<IDataPacket>,
    IDataPacket, createDataPacket,
    IDataDescriptor*, descriptor,
    SizeT, sampleCount,
    INumber*, offset
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, DataPacketImpl<IDataPacket>,
    IDataPacket, createDataPacketWithDomain,
    IDataPacket*, domainPacket,
    IDataDescriptor*, descriptor,
    SizeT, sampleCount,
    INumber*, offset
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, DataPacketImpl<IDataPacket>,
    IDataPacket, createDataPacketWithExternalMemory,
    IDataPacket*, domainPacket,
    IDataDescriptor*, descriptor,
    SizeT, sampleCount,
    INumber*, offset,
    void*, externalMemory,
    IDeleter*, deleter,
    SizeT, bufferSize
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, DataPacketImpl<IDataPacket>,
    IDataPacket, createConstantDataPacketWithDomain,
    IDataPacket*, domainPacket,
    IDataDescriptor*, descriptor,
    SizeT, sampleCount,
    void*, initialValue,
    void*, otherValues,
    SizeT, otherValueCount
)

END_NAMESPACE_OPENDAQ
