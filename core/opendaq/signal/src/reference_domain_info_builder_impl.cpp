#include <coretypes/validation.h>
#include <opendaq/reference_domain_info_builder_impl.h>
#include <opendaq/reference_domain_info_factory.h>
#include <opendaq/reference_domain_info_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

ReferenceDomainInfoBuilderImpl::ReferenceDomainInfoBuilderImpl()
    : referenceDomainId(nullptr)
    , referenceDomainOffset(nullptr)
    , referenceTimeProtocol(TimeProtocol::Unknown)
    , usesOffset(UsesOffset::Unknown)
{
}

ReferenceDomainInfoBuilderImpl::ReferenceDomainInfoBuilderImpl(const ReferenceDomainInfoPtr& infoCopy)
    : referenceDomainId(infoCopy.getReferenceDomainId())
    , referenceDomainOffset(infoCopy.getReferenceDomainOffset())
    , referenceTimeProtocol(infoCopy.getReferenceTimeProtocol())
    , usesOffset(infoCopy.getUsesOffset())
{
}

ErrCode INTERFACE_FUNC ReferenceDomainInfoBuilderImpl::build(IReferenceDomainInfo** info)
{
    OPENDAQ_PARAM_NOT_NULL(info);

    const auto builder = this->borrowPtr<ReferenceDomainInfoBuilderPtr>();

    return daqTry(
        [&]()
        {
            *info = ReferenceDomainInfoFromBuilder(builder).detach();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode ReferenceDomainInfoBuilderImpl::setReferenceDomainId(IString* referenceDomainId)
{
    this->referenceDomainId = referenceDomainId;
    return OPENDAQ_SUCCESS;
}

ErrCode ReferenceDomainInfoBuilderImpl::getReferenceDomainId(IString** referenceDomainId)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainId);
    *referenceDomainId = this->referenceDomainId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ReferenceDomainInfoBuilderImpl::setReferenceDomainOffset(IInteger* referenceDomainOffset)
{
    this->referenceDomainOffset = referenceDomainOffset;
    return OPENDAQ_SUCCESS;
}

ErrCode ReferenceDomainInfoBuilderImpl::getReferenceDomainOffset(IInteger** referenceDomainOffset)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainOffset);
    *referenceDomainOffset = this->referenceDomainOffset.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ReferenceDomainInfoBuilderImpl::setReferenceTimeProtocol(TimeProtocol referenceTimeProtocol)
{
    this->referenceTimeProtocol = referenceTimeProtocol;
    return OPENDAQ_SUCCESS;
}

ErrCode ReferenceDomainInfoBuilderImpl::getReferenceTimeProtocol(TimeProtocol* referenceTimeProtocol)
{
    OPENDAQ_PARAM_NOT_NULL(referenceTimeProtocol);
    *referenceTimeProtocol = this->referenceTimeProtocol;
    return OPENDAQ_SUCCESS;
}

ErrCode ReferenceDomainInfoBuilderImpl::setUsesOffset(UsesOffset usesOffset)
{
    this->usesOffset = usesOffset;
    return OPENDAQ_SUCCESS;
}

ErrCode ReferenceDomainInfoBuilderImpl::getUsesOffset(UsesOffset* usesOffset)
{
    OPENDAQ_PARAM_NOT_NULL(usesOffset);
    *usesOffset = this->usesOffset;
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
                                                           ReferenceDomainInfoBuilder,
                                                           IReferenceDomainInfoBuilder,
                                                           createReferenceDomainInfoBuilder, )

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
                                                           ReferenceDomainInfoBuilder,
                                                           IReferenceDomainInfoBuilder,
                                                           createReferenceDomainInfoBuilderFromExisting,
                                                           IReferenceDomainInfo*,
                                                           infoToCopy)

END_NAMESPACE_OPENDAQ
