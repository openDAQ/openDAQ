#include <opendaq/address_info_impl.h>
#include <coretypes/enumeration_factory.h>


BEGIN_NAMESPACE_OPENDAQ

const char* EnumerationName = "ProtocolType_v1";

const char* Address = "Address";
const char* Type = "Type";
const char* ReachabilityStatus = "ReachabilityStatus";
const char* ConnectionString = "ConnectionString";

template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr AddressInfoImpl::getTypedProperty(const StringPtr& name)
{
    return objPtr.getPropertyValue(name).template asPtr<T>();
}

AddressInfoImpl::AddressInfoImpl()
    : Super()
{
    Super::addProperty(StringProperty(Address, ""));
    Super::addProperty(StringProperty(Type, ""));
    Super::addProperty(IntProperty(ReachabilityStatus, static_cast<int>(AddressReachabilityStatus::Unknown)));
    Super::addProperty(StringProperty(ConnectionString, ""));
}

AddressInfoImpl::AddressInfoImpl(const AddressInfoBuilderPtr& builder)
    : AddressInfoImpl()
{
    objPtr.setPropertyValue(Address, builder.getAddress());
    objPtr.setPropertyValue(Type, builder.getType());
    objPtr.setPropertyValue(ReachabilityStatus, static_cast<int>(builder.getReachabilityStatus()));
    objPtr.setPropertyValue(ConnectionString, builder.getConnectionString());
}

ErrCode AddressInfoImpl::getAddress(IString** address)
{
    OPENDAQ_PARAM_NOT_NULL(address);

    return daqTry([&]()
    {
        *address = getTypedProperty<IString>(Address).detach();
    });
}

ErrCode AddressInfoImpl::getConnectionString(IString** connectionString)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);

    return daqTry([&]()
    {
        *connectionString = getTypedProperty<IString>(ConnectionString).detach();
    });
}

ErrCode AddressInfoImpl::getType(IString** type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    return daqTry([&]()
    {
        *type = getTypedProperty<IString>(Type).detach();
    });
}

ErrCode AddressInfoImpl::getReachabilityStatus(AddressReachabilityStatus* addressReachability)
{
    OPENDAQ_PARAM_NOT_NULL(addressReachability);

    return daqTry([&]()
    {
        *addressReachability = static_cast<AddressReachabilityStatus>(getTypedProperty<IInteger>(ReachabilityStatus));
    });
}

ErrCode AddressInfoImpl::setReachabilityStatusPrivate(AddressReachabilityStatus addressReachability)
{
    const bool frozenCache = isFrozen();
    if (frozenCache)
        unfreeze();

    ErrCode err = Super::setPropertyValue(String(ReachabilityStatus), Integer(static_cast<Int>(addressReachability)));

    if (frozenCache)
        freeze();

    return err;
}

ErrCode AddressInfoImpl::getInterfaceIds(SizeT* idCount, IntfID** ids)
{
    OPENDAQ_PARAM_NOT_NULL(idCount);

    *idCount = InterfaceIds::Count() + 1;
    if (ids == nullptr)
    {
        return OPENDAQ_SUCCESS;
    }

    **ids = IPropertyObject::Id;
    (*ids)++;

    InterfaceIds::AddInterfaceIds(*ids);
    return OPENDAQ_SUCCESS;
}

ErrCode AddressInfoImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr AddressInfoImpl::SerializeId()
{
    return "AddressInfo";
}

ErrCode AddressInfoImpl::Deserialize(ISerializedObject* serialized,
                                     IBaseObject* context,
                                     IFunction* factoryCallback,
                                     IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::DeserializePropertyObject(
                    serialized,
                    context,
                    factoryCallback,
                       [](const SerializedObjectPtr& /*serialized*/, const BaseObjectPtr& /*context*/, const StringPtr& /*className*/)
                       {
                           const auto addressInfo = createWithImplementation<IAddressInfo, AddressInfoImpl>();
                           return addressInfo;
                       }).detach();
        });
}

ErrCode AddressInfoImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);
        
    auto obj = createWithImplementation<IAddressInfo, AddressInfoImpl>();

    return daqTry([this, &obj, &cloned]()
    {
        auto implPtr = static_cast<AddressInfoImpl*>(obj.getObject());
        implPtr->configureClonedMembers(valueWriteEvents,
                                        valueReadEvents,
                                        endUpdateEvent,
                                        triggerCoreEvent,
                                        localProperties,
                                        propValues,
                                        customOrder,
                                        permissionManager);

        *cloned = obj.detach();
        return OPENDAQ_SUCCESS;
    });
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createAddressInfo(IAddressInfo** objTmp)
{
    return daq::createObject<IAddressInfo, AddressInfoImpl>(objTmp);
}

extern "C"
ErrCode PUBLIC_EXPORT createAddressInfoFromBuilder(IAddressInfo** objTmp, IAddressInfoBuilder* builder)
{
    return daq::createObject<IAddressInfo, AddressInfoImpl>(objTmp, builder);
}

#endif

END_NAMESPACE_OPENDAQ
