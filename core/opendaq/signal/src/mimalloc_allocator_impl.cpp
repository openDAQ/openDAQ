#include <opendaq/mimalloc_allocator_impl.h>
#include <coretypes/common.h>
#include <coretypes/impl.h>

#include <mimalloc.h>

BEGIN_NAMESPACE_OPENDAQ

ErrCode MiMallocAllocatorImpl::allocate(
    const ISignalDescriptor *descriptor,
    SizeT bytes,
    SizeT align,
    VoidPtr* address)
{
    if (align <= 1)
        *address = ::mi_malloc(bytes);
    else
        *address = ::mi_malloc_aligned(bytes, align);

    return OPENDAQ_SUCCESS;
}

ErrCode MiMallocAllocatorImpl::free(
    VoidPtr address)
{
    ::mi_free(address);

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, MiMallocAllocator,
    IAllocator)

END_NAMESPACE_OPENDAQ
