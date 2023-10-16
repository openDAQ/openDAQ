#include <coretypes/binarydata_impl.h>
#include <coretypes/errors.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

BinaryDataImpl::BinaryDataImpl(SizeT size)
{
    if (size == 0)
        throw InvalidParameterException();

    this->data = new (std::nothrow) char[size];
    this->size = size;
    if (this->data == nullptr)
        throw NoMemoryException();
}

ErrCode BinaryDataImpl::getAddress(void** data)
{
    if (data == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *data = this->data;
    return OPENDAQ_SUCCESS;
}

ErrCode BinaryDataImpl::getSize(SizeT* size)
{
    if (size == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *size = this->size;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC BinaryDataImpl::getCoreType(CoreType* coreType)
{
    if (coreType == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *coreType = ctBinaryData;
    return OPENDAQ_SUCCESS;
}

void BinaryDataImpl::internalDispose(bool)
{
    delete[] data;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, BinaryData, const SizeT, size)

END_NAMESPACE_OPENDAQ
