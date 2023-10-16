#include <opendaq/external_allocator_impl.h>
#include <coretypes/common.h>
#include <coretypes/impl.h>
#include <opendaq/signal_errors.h>

BEGIN_NAMESPACE_OPENDAQ

ExternalAllocatorImpl::ExternalAllocatorImpl(void* data, const DeleterPtr& deleter)
    : data(data)
    , deleter(deleter)
    , allocated(false)
    , deleted(false)
{
#ifdef OPENDAQ_ENABLE_PARAMETER_VALIDATION
    if (!this->data)
        throw InvalidParameterException("Data parameter must not be null.");

    if (!this->deleter.assigned())
        throw ArgumentNullException("Deleter parameter must not be null.");
#endif
}

ErrCode ExternalAllocatorImpl::allocate(
    const IDataDescriptor *descriptor,
    SizeT bytes,
    SizeT align,
    VoidPtr* address)
{
    if(allocated)
        return makeErrorInfo(OPENDAQ_ERR_PACKET_MEMORY_ALLOCATION, "Memory already in use");

    *address = data;
    allocated = true;

    return OPENDAQ_SUCCESS;
}

ErrCode ExternalAllocatorImpl::free(VoidPtr address)
{
    if(!address)
        return OPENDAQ_SUCCESS;

    if(deleted)
        return makeErrorInfo(OPENDAQ_ERR_PACKET_MEMORY_DEALLOCATION, "Memory already freed");

    if(address != data)
        return makeErrorInfo(OPENDAQ_ERR_PACKET_MEMORY_DEALLOCATION, "Memory address mismatch");

    deleter.deleteMemory(address);
    deleted = true;

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ExternalAllocator,
    IAllocator,
    void*, data,
    IDeleter*, deleter
)

END_NAMESPACE_OPENDAQ
