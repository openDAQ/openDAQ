#include <opendaq/transport_option_impl.h>
#include <coreobjects/property_factory.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

namespace
{

DictPtr<IInteger, IString> ProtectionNames()
{
    return Dict<IInteger, IString>({
        {static_cast<Int>(TransportProtection::None), "None"},
        {static_cast<Int>(TransportProtection::Signed), "Signed"},
        {static_cast<Int>(TransportProtection::Encrypted), "Encrypted"},
    });
}

}

TransportOptionImpl::TransportOptionImpl(const StringPtr& id,
                                         const IntegerPtr& port,
                                         TransportProtection protection,
                                         Bool certificateRequired)
    : Super()
{
    if (!id.assigned() || id.getLength() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The transport option id must not be empty");
    if (!port.assigned())
        DAQ_THROW_EXCEPTION(ArgumentNullException, "The port of transport option \"{}\" is not assigned", id);
    if (static_cast<Int>(port) < 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The port of transport option \"{}\" must not be negative", id);
    if (protection != TransportProtection::None && protection != TransportProtection::Signed && protection != TransportProtection::Encrypted)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The protection of transport option \"{}\" is not valid", id);

    Super::addProperty(StringProperty(IdPropertyName, id));
    Super::addProperty(IntProperty(PortPropertyName, port));
    Super::addProperty(SparseSelectionProperty(ProtectionPropertyName, ProtectionNames(), static_cast<Int>(protection)));
    Super::addProperty(BoolProperty(CertificateRequiredPropertyName, certificateRequired));

    Super::freeze();
}

ErrCode TransportOptionImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    return daqTry([&]
    {
        StringPtr value = objPtr.getPropertyValue(IdPropertyName);
        *id = value.detach();
    });
}

ErrCode TransportOptionImpl::getPort(IInteger** port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    return daqTry([&]
    {
        IntegerPtr value = objPtr.getPropertyValue(PortPropertyName);
        *port = value.detach();
    });
}

ErrCode TransportOptionImpl::getProtection(TransportProtection* protection)
{
    OPENDAQ_PARAM_NOT_NULL(protection);

    return daqTry([&]
    {
        const Int value = objPtr.getPropertyValue(ProtectionPropertyName);
        *protection = static_cast<TransportProtection>(value);
    });
}

ErrCode TransportOptionImpl::getCertificateRequired(Bool* required)
{
    OPENDAQ_PARAM_NOT_NULL(required);

    return daqTry([&]
    {
        *required = objPtr.getPropertyValue(CertificateRequiredPropertyName);
    });
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

ErrCode TransportOptionImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    return daqTry([&]
    {
        // Immutable, so a copy of the fields is a full clone
        const StringPtr id = objPtr.getPropertyValue(IdPropertyName);
        const Int port = objPtr.getPropertyValue(PortPropertyName);
        const Int protection = objPtr.getPropertyValue(ProtectionPropertyName);
        const Bool certificateRequired = objPtr.getPropertyValue(CertificateRequiredPropertyName);

        const auto copy = TransportOption(id, port, static_cast<TransportProtection>(protection), certificateRequired);
        *cloned = copy.asPtr<IPropertyObject>().detach();
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, TransportOption, ITransportOption,
    IString*, id,
    IInteger*, port,
    TransportProtection, protection,
    Bool, certificateRequired
)

END_NAMESPACE_OPENDAQ
