#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_impl.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/data_descriptor.h>
#include <opendaq/event_packet_ptr.h>

#include <utility>

BEGIN_NAMESPACE_OPENDAQ
    EventPacketImpl::EventPacketImpl(StringPtr eventId, DictPtr<IString, IBaseObject> parameters)
    : eventId(std::move(eventId))
    , parameters(std::move(parameters))
{
    this->type = PacketType::Event;
    if (this->parameters.asPtrOrNull<IFreezable>().assigned() && !this->parameters.isFrozen())
        this->parameters.freeze();
}

ErrCode EventPacketImpl::getEventId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->eventId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode EventPacketImpl::getParameters(IDict** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    *parameters = this->parameters.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC EventPacketImpl::equals(IBaseObject* other, Bool* equals) const
{
    if (equals == nullptr)
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

    *equals = false;
    if (other == nullptr)
        return OPENDAQ_SUCCESS;

    return daqTry(
        [this, &other, &equals]()
        {
            const ErrCode errCode = Super::equals(other, equals);
            checkErrorInfo(errCode);

            if (!(*equals))
                return errCode;

            *equals = false;
            const EventPacketPtr packetOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IEventPacket>();
            if (packetOther == nullptr)
                return errCode;

            if (!BaseObjectPtr::Equals(this->eventId, packetOther.getEventId()))
                return errCode;
            if (!BaseObjectPtr::Equals(this->parameters, packetOther.getParameters()))
                return errCode;

            *equals = true;
            return errCode;
        });
}

ErrCode EventPacketImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        serializer->key("id");
        serializer->writeString(eventId.getCharPtr(), eventId.getLength());

        serializer->key("params");
        parameters.serialize(serializer);
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode EventPacketImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr EventPacketImpl::SerializeId()
{
    return "EventPacket";
}

ErrCode EventPacketImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);
    auto eventId = serializedObj.readString("id");
    DictPtr<IString, IBaseObject> parameters = serializedObj.readObject("params");

    EventPacketPtr eventPacket;
    auto errCode = createObject<IEventPacket, EventPacketImpl>(&eventPacket, eventId, parameters);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    *obj = eventPacket.as<IBaseObject>();

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, EventPacket, IString*, id, IDict*, params)

using DataDescriptorChangedEventPacketImpl = EventPacketImpl;
using PropertyChangedEventPacketImpl = EventPacketImpl;

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C" daq::ErrCode PUBLIC_EXPORT createDataDescriptorChangedEventPacket(IEventPacket** objTmp,
                                                                             IDataDescriptor* dataDescriptor,
                                                                             IDataDescriptor* domainDataDescriptor)
{
    DictPtr<IString, IDataDescriptor> parameters = Dict<IString, IDataDescriptor>(
        {{event_packet_param::DATA_DESCRIPTOR, dataDescriptor}, {event_packet_param::DOMAIN_DATA_DESCRIPTOR, domainDataDescriptor}});

    return daq::createObject<IEventPacket, DataDescriptorChangedEventPacketImpl>(
        objTmp, event_packet_id::DATA_DESCRIPTOR_CHANGED, parameters);
}

extern "C" daq::ErrCode PUBLIC_EXPORT createPropertyChangedEventPacket(IEventPacket** objTmp,
                                                                       IString* name,
                                                                       IBaseObject* value)
{
    const auto parameters = Dict<IString, IBaseObject>(
        {{event_packet_param::NAME, name}, {event_packet_param::VALUE, value }});

    return daq::createObject<IEventPacket, PropertyChangedEventPacketImpl>(objTmp, event_packet_id::PROPERTY_CHANGED, parameters);
}

#endif

END_NAMESPACE_OPENDAQ
