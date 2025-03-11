#include <coretypes/binarydata_impl.h>
#include <coretypes/errors.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

BinaryDataImpl::BinaryDataImpl(SizeT size)
{
    if (size == 0)
        THROW_OPENDAQ_EXCEPTION(InvalidParameterException());

    this->data = new (std::nothrow) char[size];
    this->size = size;
    if (this->data == nullptr)
        THROW_OPENDAQ_EXCEPTION(NoMemoryException());
}

ErrCode BinaryDataImpl::getAddress(void** data)
{
    OPENDAQ_PARAM_NOT_NULL(data);

    *data = this->data;
    return OPENDAQ_SUCCESS;
}

ErrCode BinaryDataImpl::getSize(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);

    *size = this->size;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC BinaryDataImpl::getCoreType(CoreType* coreType)
{
    OPENDAQ_PARAM_NOT_NULL(coreType);

    *coreType = ctBinaryData;
    return OPENDAQ_SUCCESS;
}

void BinaryDataImpl::internalDispose(bool)
{
    delete[] data;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, BinaryData, const SizeT, size)

END_NAMESPACE_OPENDAQ
