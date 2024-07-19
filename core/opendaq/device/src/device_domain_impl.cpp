#include <coretypes/validation.h>
#include <opendaq/device_domain_impl.h>
#include <opendaq/device_domain_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr unitStructType = UnitStructType();
}

DeviceDomainImpl::DeviceDomainImpl(
    RatioPtr tickResolution, StringPtr origin, UnitPtr unit, StringPtr domainId, IntegerPtr grandmasterOffset)
    : GenericStructImpl<IDeviceDomain, IStruct>(detail::unitStructType,
                                                Dict<IString, IBaseObject>({{"TickResolution", std::move(tickResolution)},
                                                                            {"Origin", std::move(origin)},
                                                                            {"Unit", std::move(unit)},
                                                                            {"domainId", std::move(domainId)},
                                                                            {"grandmasterOffset", std::move(grandmasterOffset)}}))
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

ErrCode DeviceDomainImpl::getDomainId(IString** domainId)
{
    OPENDAQ_PARAM_NOT_NULL(domainId);

    *domainId = this->fields.get("domainId").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceDomainImpl::getGrandmasterOffset(IInteger** grandmasterOffset)
{
    OPENDAQ_PARAM_NOT_NULL(grandmasterOffset);

    *grandmasterOffset = this->fields.get("grandmasterOffset").asPtr<IInteger>().addRefAndReturn();
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

        const StringPtr domainId = this->fields.get("domainId");
        if (domainId.assigned()) // TODO: maybe check for empty string?
        {
            serializer->key("domainId");
            serializer->writeString(domainId.getCharPtr(), domainId.getLength());
        }

        const IntegerPtr grandmasterOffset = this->fields.get("grandmasterOffset");
        if (grandmasterOffset.assigned())
        {
            serializer->key("grandmasterOffset");
            serializer->writeInt(grandmasterOffset);
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
    StringPtr domainId;
    IntegerPtr grandmasterOffset;
    
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

    if (serializedObj.hasKey("domainId"))
    {
        domainId = serializedObj.readString("domainId");
    }

    if (serializedObj.hasKey("grandmasterOffset"))
    {
        grandmasterOffset = serializedObj.readInt("grandmasterOffset");
    }

    *obj = DeviceDomain(resolution, origin, unit, domainId, grandmasterOffset).as<IBaseObject>();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, DeviceDomain,
    IRatio*, tickResolution,
    IString*, origin,
    IUnit*, unit,
    IString*, domainId,
    IInteger*, grandmasterOffset
)

END_NAMESPACE_OPENDAQ
