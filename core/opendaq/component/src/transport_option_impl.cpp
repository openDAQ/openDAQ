#include <opendaq/transport_option_impl.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/serializer_ptr.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

namespace
{

ConstCharPtr SecurityModeToString(SecurityMode securityMode)
{
    switch (securityMode)
    {
        case SecurityMode::Signed:
            return "Signed";
        case SecurityMode::Encrypted:
            return "Encrypted";
        case SecurityMode::None:
        default:
            return "None";
    }
}

SecurityMode SecurityModeFromString(const StringPtr& securityMode)
{
    if (securityMode == "None")
        return SecurityMode::None;
    if (securityMode == "Signed")
        return SecurityMode::Signed;
    if (securityMode == "Encrypted")
        return SecurityMode::Encrypted;

    DAQ_THROW_EXCEPTION(InvalidParameterException, R"(Unknown security mode "{}")", securityMode);
}

}

TransportOptionImpl::TransportOptionImpl(const StringPtr& id,
                                         Int port,
                                         SecurityMode securityMode,
                                         Bool certificateRequired)
    : id(id)
    , port(port)
    , securityMode(securityMode)
    , certificateRequired(certificateRequired)
{
    if (!id.assigned() || id.getLength() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The transport option id must not be empty");
    if (securityMode != SecurityMode::None && securityMode != SecurityMode::Signed && securityMode != SecurityMode::Encrypted)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The security mode of transport option \"{}\" is not valid", id);
}

ErrCode TransportOptionImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->id.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode TransportOptionImpl::getPort(Int* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    *port = this->port;
    return OPENDAQ_SUCCESS;
}

ErrCode TransportOptionImpl::getSecurityMode(SecurityMode* securityMode)
{
    OPENDAQ_PARAM_NOT_NULL(securityMode);

    *securityMode = this->securityMode;
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
        serializer->writeInt(port);

        serializer->key("securityMode");
        const StringPtr securityModeStr = SecurityModeToString(securityMode);
        serializer->writeString(securityModeStr.getCharPtr(), securityModeStr.getLength());

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
        const SecurityMode securityMode = SecurityModeFromString(serializedObj.readString("securityMode"));
        const Bool certificateRequired = serializedObj.readBool("certificateRequired");

        *obj = TransportOption(id, port, securityMode, certificateRequired).detach();
        return OPENDAQ_SUCCESS;
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, TransportOption, ITransportOption,
    IString*, id,
    Int, port,
    SecurityMode, securityMode,
    Bool, certificateRequired
)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(TransportOptionImpl)

END_NAMESPACE_OPENDAQ
