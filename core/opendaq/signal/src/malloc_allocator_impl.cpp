#include <opendaq/malloc_allocator_impl.h>
#include <coretypes/common.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

ErrCode MallocAllocatorImpl::allocate(
    const IDataDescriptor *descriptor,
    SizeT bytes,
    SizeT align,
    VoidPtr* address)
{
    *address = std::malloc(bytes);
    return OPENDAQ_SUCCESS;
}

ErrCode MallocAllocatorImpl::free(VoidPtr address)
{
    std::free(address);
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, MallocAllocator,
    IAllocator)

END_NAMESPACE_OPENDAQ
