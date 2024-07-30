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
                                   StringPtr referenceDomainId,
                                   NumberPtr referenceDomainOffset,
                                   BoolPtr referenceDomainIsAbsolute)
    : GenericStructImpl<IDeviceDomain, IStruct>(
          detail::deviceDomainStructType,
          Dict<IString, IBaseObject>({{"TickResolution", std::move(tickResolution)},
                                      {"Origin", std::move(origin)},
                                      {"Unit", std::move(unit)},
                                      {"ReferenceDomainId", std::move(referenceDomainId)},
                                      {"ReferenceDomainOffset", std::move(referenceDomainOffset)},
                                      {"ReferenceDomainIsAbsolute", std::move(referenceDomainIsAbsolute)}}))
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

ErrCode DeviceDomainImpl::getReferenceDomainId(IString** referenceDomainId)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainId);

    auto ptr = this->fields.get("ReferenceDomainId");
    if (ptr.assigned())
        *referenceDomainId = ptr.asPtr<IString>().addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode DeviceDomainImpl::getReferenceDomainOffset(INumber** referenceDomainOffset)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainOffset);

    auto ptr = this->fields.get("ReferenceDomainOffset");
    if (ptr.assigned())
        *referenceDomainOffset = ptr.asPtr<INumber>().addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode DeviceDomainImpl::getReferenceDomainIsAbsolute(IBoolean** referenceDomainIsAbsolute)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainIsAbsolute);

    auto ptr = this->fields.get("ReferenceDomainIsAbsolute");
    if (ptr.assigned())
        *referenceDomainIsAbsolute = ptr.asPtr<IBoolean>().addRefAndReturn();

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

        const StringPtr referenceDomainId = this->fields.get("ReferenceDomainId");
        if (referenceDomainId.assigned()) // TODO: maybe check for empty string?
        {
            serializer->key("referenceDomainId");
            serializer->writeString(referenceDomainId.getCharPtr(), referenceDomainId.getLength());
        }

        const NumberPtr referenceDomainOffset = this->fields.get("ReferenceDomainOffset");
        if (referenceDomainOffset.assigned())
        {
            serializer->key("referenceDomainOffset");
            serializer->writeFloat(referenceDomainOffset);
        }

        const BoolPtr referenceDomainIsAbsolute = this->fields.get("ReferenceDomainIsAbsolute");
        if (referenceDomainIsAbsolute.assigned())
        {
            serializer->key("referenceDomainIsAbsolute");
            serializer->writeBool(referenceDomainIsAbsolute);
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
    StringPtr referenceDomainId;
    NumberPtr referenceDomainOffset;
    BoolPtr referenceDomainIsAbsolute;
    
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

    if (serializedObj.hasKey("referenceDomainId"))
    {
        referenceDomainId = serializedObj.readString("referenceDomainId");
    }

    if (serializedObj.hasKey("referenceDomainOffset"))
    {
        referenceDomainOffset = serializedObj.readFloat("referenceDomainOffset");
    }

    if (serializedObj.hasKey("referenceDomainIsAbsolute"))
    {
        referenceDomainIsAbsolute = serializedObj.readBool("referenceDomainIsAbsolute");
    }

    *obj = DeviceDomain(resolution, origin, unit, referenceDomainId, referenceDomainOffset, referenceDomainIsAbsolute).as<IBaseObject>();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, DeviceDomain,
    IRatio*, tickResolution,
    IString*, origin,
    IUnit*, unit,
    IString*, referenceDomainId,
    INumber*, referenceDomainOffset,
    IBoolean*, referenceDomainIsAbsolute
)

END_NAMESPACE_OPENDAQ
