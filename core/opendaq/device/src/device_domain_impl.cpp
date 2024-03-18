#include <coretypes/validation.h>
#include <opendaq/device_domain_impl.h>
#include <opendaq/device_domain_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr unitStructType = UnitStructType();
}

DeviceDomainImpl::DeviceDomainImpl(RatioPtr tickResolution, StringPtr origin, UnitPtr unit)
    : GenericStructImpl<IDeviceDomain, IStruct>(
        detail::unitStructType,
        Dict<IString, IBaseObject>(
            {{"tickResolution", std::move(tickResolution)}, {"origin", std::move(origin)}, {"unit", std::move(unit)}}))
{
}

ErrCode DeviceDomainImpl::getTickResolution(IRatio** tickResolution)
{
    OPENDAQ_PARAM_NOT_NULL(tickResolution);

    *tickResolution = this->fields.get("tickResolution").asPtr<IRatio>().addRefAndReturn();;
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceDomainImpl::getOrigin(IString** origin)
{
    OPENDAQ_PARAM_NOT_NULL(origin);

    *origin = this->fields.get("origin").asPtr<IString>().addRefAndReturn();;
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceDomainImpl::getUnit(IUnit** unit)
{
    OPENDAQ_PARAM_NOT_NULL(unit);

    *unit = this->fields.get("unit").asPtr<IUnit>().addRefAndReturn();;
    return OPENDAQ_SUCCESS;
}

ErrCode DeviceDomainImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        const RatioPtr resolution = this->fields.get("tickResolution");
        if (resolution.assigned())
        {
            serializer->key("tickResolution");
            resolution.serialize(serializer);
        }
        
        const StringPtr origin = this->fields.get("origin");
        if (origin.assigned() && origin != "")
        {
            serializer->key("origin");
            serializer->writeString(origin.getCharPtr(), origin.getLength());
        }
        
        const UnitPtr unit = this->fields.get("unit");
        if (unit.assigned())
        {
            serializer->key("unit");
            unit.serialize(serializer);
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

    *obj = DeviceDomain(resolution, origin, unit).as<IBaseObject>();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, DeviceDomain,
    IRatio*, tickResolution,
    IString*, origin,
    IUnit*, unit
)

END_NAMESPACE_OPENDAQ
