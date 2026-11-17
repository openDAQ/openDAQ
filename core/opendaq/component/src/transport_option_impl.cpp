#include <opendaq/transport_option_impl.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/serializer_ptr.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

namespace
{

ConstCharPtr ProtectionToString(TransportProtection protection)
{
    switch (protection)
    {
        case TransportProtection::Signed:
            return "Signed";
        case TransportProtection::Encrypted:
            return "Encrypted";
        case TransportProtection::None:
        default:
            return "None";
    }
}

TransportProtection ProtectionFromString(const StringPtr& protection)
{
    if (protection == "None")
        return TransportProtection::None;
    if (protection == "Signed")
        return TransportProtection::Signed;
    if (protection == "Encrypted")
        return TransportProtection::Encrypted;

    DAQ_THROW_EXCEPTION(InvalidParameterException, R"(Unknown transport protection "{}")", protection);
}

}

TransportOptionImpl::TransportOptionImpl(const StringPtr& id,
                                         const IntegerPtr& port,
                                         TransportProtection protection,
                                         Bool certificateRequired)
    : id(id)
    , port(port)
    , protection(protection)
    , certificateRequired(certificateRequired)
{
    if (!id.assigned() || id.getLength() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The transport option id must not be empty");
    if (!port.assigned())
        DAQ_THROW_EXCEPTION(ArgumentNullException, "The port of transport option \"{}\" is not assigned", id);
    if (static_cast<Int>(port) < 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The port of transport option \"{}\" must not be negative", id);
    if (protection != TransportProtection::None && protection != TransportProtection::Signed && protection != TransportProtection::Encrypted)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The protection of transport option \"{}\" is not valid", id);
}

ErrCode TransportOptionImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->id.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode TransportOptionImpl::getPort(IInteger** port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    *port = this->port.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode TransportOptionImpl::getProtection(TransportProtection* protection)
{
    OPENDAQ_PARAM_NOT_NULL(protection);

    *protection = this->protection;
    return OPENDAQ_SUCCESS;
}

ErrCode TransportOptionImpl::getCertificateRequired(Bool* required)
{
    OPENDAQ_PARAM_NOT_NULL(required);

    *required = this->certificateRequired;
    return OPENDAQ_SUCCESS;
}

ErrCode TransportOptionImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    return daqTry([&]
    {
        serializer->startTaggedObject(this);

        serializer->key("id");
        serializer->writeString(id.getCharPtr(), id.getLength());

        serializer->key("port");
        serializer->writeInt(static_cast<Int>(port));

        serializer->key("protection");
        const StringPtr protectionStr = ProtectionToString(protection);
        serializer->writeString(protectionStr.getCharPtr(), protectionStr.getLength());

        serializer->key("certificateRequired");
        serializer->writeBool(certificateRequired);

        serializer->endObject();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode TransportOptionImpl::getSerializeId(ConstCharPtr* serializeId) const
{
    OPENDAQ_PARAM_NOT_NULL(serializeId);

    *serializeId = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr TransportOptionImpl::SerializeId()
{
    return "TransportOption";
}

ErrCode TransportOptionImpl::Deserialize(ISerializedObject* serialized,
                                         IBaseObject* /*context*/,
                                         IFunction* /*factoryCallback*/,
                                         IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&]
    {
        const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
        const StringPtr id = serializedObj.readString("id");
        const Int port = serializedObj.readInt("port");
        const TransportProtection protection = ProtectionFromString(serializedObj.readString("protection"));
        const Bool certificateRequired = serializedObj.readBool("certificateRequired");

        *obj = TransportOption(id, port, protection, certificateRequired).detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, TransportOption, ITransportOption,
    IString*, id,
    IInteger*, port,
    TransportProtection, protection,
    Bool, certificateRequired
)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(TransportOptionImpl)

END_NAMESPACE_OPENDAQ
