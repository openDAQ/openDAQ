#include <opendaq/address_info_builder_impl.h>
#include <opendaq/address_info_impl.h>
#include <opendaq/address_info_builder_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

AddressInfoBuilderImpl::AddressInfoBuilderImpl()
    : reachability(AddressReachabilityStatus::Unknown)
{
}

ErrCode AddressInfoBuilderImpl::build(IAddressInfo** address)
{
    OPENDAQ_PARAM_NOT_NULL(address);

    const auto builderPtr = this->borrowPtr<AddressInfoBuilderPtr>();

    return daqTry([&]()
    {
        *address = createWithImplementation<IAddressInfo, AddressInfoImpl>(builderPtr).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AddressInfoBuilderImpl::setAddress(IString* address)
{
    this->address = address;
    return OPENDAQ_SUCCESS;
}

ErrCode AddressInfoBuilderImpl::getAddress(IString** address)
{
    OPENDAQ_PARAM_NOT_NULL(address);
    *address = this->address.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AddressInfoBuilderImpl::setConnectionString(IString* connectionString)
{
    this->connectionString = connectionString;
    return OPENDAQ_SUCCESS;
}

ErrCode AddressInfoBuilderImpl::getConnectionString(IString** connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(address);
    *connectionString = this->connectionString.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AddressInfoBuilderImpl::setType(IString* type)
{
    this->type = type;
    return OPENDAQ_SUCCESS;
}

ErrCode AddressInfoBuilderImpl::getType(IString** type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    *type = this->type.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AddressInfoBuilderImpl::setReachabilityStatus(AddressReachabilityStatus addressReachability)
{
    this->reachability = addressReachability;
    return OPENDAQ_SUCCESS;
}

ErrCode AddressInfoBuilderImpl::getReachabilityStatus(AddressReachabilityStatus* addressReachability)
{
    OPENDAQ_PARAM_NOT_NULL(addressReachability);
    *addressReachability = this->reachability;
    return OPENDAQ_SUCCESS;
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createAddressInfoBuilder(IAddressInfoBuilder** objTmp)
{
    return daq::createObject<IAddressInfoBuilder, AddressInfoBuilderImpl>(objTmp);
}

#endif

END_NAMESPACE_OPENDAQ
