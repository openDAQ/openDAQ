#include <coretypes/validation.h>
#include <opendaq/device_domain_impl.h>
#include <opendaq/device_domain_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr deviceDomainStructType = DeviceDomainStructType();
}

DeviceDomainImpl::DeviceDomainImpl(RatioPtr tickResolution,
                                   StringPtr origin,
                                   UnitPtr unit,
                                   ReferenceDomainInfoPtr referenceDomainInfo)
    : GenericStructImpl<IDeviceDomain, IStruct>(
          detail::deviceDomainStructType,
          Dict<IString, IBaseObject>({{"TickResolution", std::move(tickResolution)},
                                      {"Origin", std::move(origin)},
                                      {"Unit", std::move(unit)},
                                      {"ReferenceDomainInfo", std::move(referenceDomainInfo)}}))
{
}

ErrCode DeviceDomainImpl::getTickResolution(IRatio** tickResolution)
{
    OPENDAQ_PARAM_NOT_NULL(tickResolution);

    *tickResolution = this->fields.get("TickResolution").asPtr<IRatio>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceDomainImpl::getOrigin(IString** origin)
{
    OPENDAQ_PARAM_NOT_NULL(origin);

    *origin = this->fields.get("Origin").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceDomainImpl::getUnit(IUnit** unit)
{
    OPENDAQ_PARAM_NOT_NULL(unit);

    *unit = this->fields.get("Unit").asPtr<IUnit>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceDomainImpl::getReferenceDomainInfo(IReferenceDomainInfo** referenceDomainInfo)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainInfo);

    auto ptr = this->fields.get("ReferenceDomainInfo");
    if (ptr.assigned())
        *referenceDomainInfo = ptr.asPtr<IReferenceDomainInfo>().addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode DeviceDomainImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        const RatioPtr resolution = this->fields.get("TickResolution");
        if (resolution.assigned())
        {
            serializer->key("tickResolution");
            resolution.serialize(serializer);
        }
        
        const StringPtr origin = this->fields.get("Origin");
        if (origin.assigned() && origin != "")
        {
            serializer->key("origin");
            serializer->writeString(origin.getCharPtr(), origin.getLength());
        }
        
        const UnitPtr unit = this->fields.get("Unit");
        if (unit.assigned())
        {
            serializer->key("unit");
            unit.serialize(serializer);
        }

        const ReferenceDomainInfoPtr referenceDomainInfo = this->fields.get("ReferenceDomainInfo");
        if (referenceDomainInfo.assigned())
        {
            serializer->key("referenceDomainInfo");
            referenceDomainInfo.serialize(serializer);
        }
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceDomainImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr DeviceDomainImpl::SerializeId()
{
    return "DeviceDomain";
}

ErrCode DeviceDomainImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);

    RatioPtr resolution;
    StringPtr origin;
    UnitPtr unit;
    ReferenceDomainInfoPtr referenceDomainInfo;
    
    if (serializedObj.hasKey("tickResolution"))
    {
        resolution = serializedObj.readObject("tickResolution");
    }
    
    if (serializedObj.hasKey("origin"))
    {
        origin = serializedObj.readString("origin");
    }

    if (serializedObj.hasKey("unit"))
    {
        unit = serializedObj.readObject("unit");
    }

    if (serializedObj.hasKey("referenceDomainInfo"))
    {
        referenceDomainInfo = serializedObj.readObject("referenceDomainInfo");
    }

    *obj = DeviceDomain(resolution, origin, unit, referenceDomainInfo).as<IBaseObject>();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, DeviceDomain,
    IRatio*, tickResolution,
    IString*, origin,
    IUnit*, unit
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, DeviceDomain, IDeviceDomain, createDeviceDomainWithReferenceDomainInfo,
    IRatio*, tickResolution,
    IString*, origin,
    IUnit*, unit,
    IReferenceDomainInfo*, referenceDomainInfo
)

END_NAMESPACE_OPENDAQ
