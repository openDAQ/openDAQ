#include <opendaq/transport_option_impl.h>
#include <opendaq/transport_option_builder_ptr.h>
#include <coretypes/validation.h>
#include <coretypes/boolean_factory.h>
#include <coreobjects/property_object_protected_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

namespace transport_option
{
static const char* Id = "Id";
static const char* Port = "Port";
static const char* SecurityMode = "SecurityMode";
static const char* CertificateRequired = "CertificateRequired";
}

TransportOptionImpl::TransportOptionImpl()
    : Super()
{
    const auto securityModeNames = Dict<IInteger, IString>({{static_cast<Int>(SecurityMode::None), "None"},
                                                            {static_cast<Int>(SecurityMode::Signed), "Signed"},
                                                            {static_cast<Int>(SecurityMode::Encrypted), "Encrypted"}});

    Super::addProperty(StringPropertyBuilder(transport_option::Id, "").setReadOnly(true).build());
    Super::addProperty(IntPropertyBuilder(transport_option::Port, 0).setReadOnly(true).build());
    Super::addProperty(SparseSelectionPropertyBuilder(transport_option::SecurityMode, securityModeNames, static_cast<Int>(SecurityMode::None))
                           .setReadOnly(true)
                           .build());
    Super::addProperty(BoolPropertyBuilder(transport_option::CertificateRequired, False).setReadOnly(true).build());
}

TransportOptionImpl::TransportOptionImpl(ITransportOptionBuilder* builder)
    : TransportOptionImpl()
{
    const auto builderPtr = TransportOptionBuilderPtr::Borrow(builder);
    const StringPtr id = builderPtr.getId();
    const SecurityMode securityMode = builderPtr.getSecurityMode();

    if (!id.assigned() || id.getLength() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The transport option id must not be empty");
    if (securityMode != SecurityMode::None && securityMode != SecurityMode::Signed && securityMode != SecurityMode::Encrypted)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The security mode of transport option \"{}\" is not valid", id);

    const auto protectedObj = objPtr.asPtr<IPropertyObjectProtected>();
    protectedObj.setProtectedPropertyValue(transport_option::Id, id);
    protectedObj.setProtectedPropertyValue(transport_option::Port, Integer(builderPtr.getPort()));
    protectedObj.setProtectedPropertyValue(transport_option::SecurityMode, Integer(static_cast<Int>(securityMode)));
    protectedObj.setProtectedPropertyValue(transport_option::CertificateRequired, Boolean(builderPtr.getCertificateRequired()));
}

ErrCode TransportOptionImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    return daqTry([&] { *id = objPtr.getPropertyValue(transport_option::Id).asPtr<IString>().detach(); });
}

ErrCode TransportOptionImpl::getPort(Int* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    return daqTry([&] { *port = objPtr.getPropertyValue(transport_option::Port); });
}

ErrCode TransportOptionImpl::getSecurityMode(SecurityMode* securityMode)
{
    OPENDAQ_PARAM_NOT_NULL(securityMode);

    return daqTry(
        [&]
        {
            const Int value = objPtr.getPropertyValue(transport_option::SecurityMode);
            *securityMode = static_cast<SecurityMode>(value);
        });
}

ErrCode TransportOptionImpl::getCertificateRequired(Bool* required)
{
    OPENDAQ_PARAM_NOT_NULL(required);

    return daqTry([&] { *required = objPtr.getPropertyValue(transport_option::CertificateRequired); });
}

ErrCode TransportOptionImpl::getInterfaceIds(SizeT* idCount, IntfID** ids)
{
    OPENDAQ_PARAM_NOT_NULL(idCount);

    *idCount = InterfaceIds::Count() + 1;
    if (ids == nullptr)
        return OPENDAQ_SUCCESS;

    **ids = IPropertyObject::Id;
    (*ids)++;

    InterfaceIds::AddInterfaceIds(*ids);
    return OPENDAQ_SUCCESS;
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
                                         IBaseObject* context,
                                         IFunction* factoryCallback,
                                         IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry(
        [&]
        {
            *obj = Super::DeserializePropertyObject(
                       serialized,
                       context,
                       factoryCallback,
                       [](const SerializedObjectPtr& /*serialized*/, const BaseObjectPtr& /*context*/, const StringPtr& /*className*/)
                       { return createWithImplementation<ITransportOption, TransportOptionImpl>(); })
                       .detach();
        });
}

ErrCode TransportOptionImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    return daqTry(
        [&]
        {
            auto obj = createWithImplementation<ITransportOption, TransportOptionImpl>();
            dynamic_cast<TransportOptionImpl*>(obj.getObject())->configureClonedMembers(getCloneParameters());
            *cloned = obj.detach();
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, TransportOption,
    ITransportOption, createTransportOptionFromBuilder,
    ITransportOptionBuilder*, builder
)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(TransportOptionImpl)

END_NAMESPACE_OPENDAQ
