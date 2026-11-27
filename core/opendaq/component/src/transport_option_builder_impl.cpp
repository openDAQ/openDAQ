#include <opendaq/transport_option_builder_impl.h>
#include <opendaq/transport_option_factory.h>

BEGIN_NAMESPACE_OPENDAQ

TransportOptionBuilderImpl::TransportOptionBuilderImpl(IString* id)
    : id(id)
    , port(0)
    , securityMode(SecurityMode::None)
    , certificateRequired(False)
{
}

TransportOptionBuilderImpl::TransportOptionBuilderImpl(ITransportOption* transportOptionToCopy)
{
    const auto option = TransportOptionPtr::Borrow(transportOptionToCopy);
    id = option.getId();
    port = option.getPort();
    securityMode = option.getSecurityMode();
    certificateRequired = option.getCertificateRequired();
}

ErrCode TransportOptionBuilderImpl::build(ITransportOption** transportOption)
{
    OPENDAQ_PARAM_NOT_NULL(transportOption);

    const auto builderPtr = this->borrowPtr<TransportOptionBuilderPtr>();

    return daqTry([&]
    {
        *transportOption = TransportOptionFromBuilder(builderPtr).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode TransportOptionBuilderImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->id.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode TransportOptionBuilderImpl::setPort(Int port)
{
    this->port = port;
    return OPENDAQ_SUCCESS;
}

ErrCode TransportOptionBuilderImpl::getPort(Int* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    *port = this->port;
    return OPENDAQ_SUCCESS;
}

ErrCode TransportOptionBuilderImpl::setSecurityMode(SecurityMode securityMode)
{
    this->securityMode = securityMode;
    return OPENDAQ_SUCCESS;
}

ErrCode TransportOptionBuilderImpl::getSecurityMode(SecurityMode* securityMode)
{
    OPENDAQ_PARAM_NOT_NULL(securityMode);

    *securityMode = this->securityMode;
    return OPENDAQ_SUCCESS;
}

ErrCode TransportOptionBuilderImpl::setCertificateRequired(Bool required)
{
    this->certificateRequired = required;
    return OPENDAQ_SUCCESS;
}

ErrCode TransportOptionBuilderImpl::getCertificateRequired(Bool* required)
{
    OPENDAQ_PARAM_NOT_NULL(required);

    *required = this->certificateRequired;
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, TransportOptionBuilder, ITransportOptionBuilder,
    IString*, id
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, TransportOptionBuilder, ITransportOptionBuilder, createTransportOptionBuilderFromExisting,
    ITransportOption*, transportOptionToCopy
)

END_NAMESPACE_OPENDAQ
