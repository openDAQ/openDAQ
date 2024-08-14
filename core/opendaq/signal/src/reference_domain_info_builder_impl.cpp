#include <coretypes/validation.h>
#include <opendaq/reference_domain_info_builder_impl.h>
#include <opendaq/reference_domain_info_factory.h>
#include <opendaq/reference_domain_info_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

ReferenceDomainInfoBuilderImpl::ReferenceDomainInfoBuilderImpl()
    : referenceDomainId(nullptr)
    , referenceDomainOffset(nullptr)
    , referenceTimeSource(nullptr)
{
}

ReferenceDomainInfoBuilderImpl::ReferenceDomainInfoBuilderImpl(const ReferenceDomainInfoPtr& infoCopy)
    : referenceDomainId(infoCopy.getReferenceDomainId())
    , referenceDomainOffset(infoCopy.getReferenceDomainOffset())
    , referenceTimeSource(infoCopy.getReferenceTimeSource())
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

ErrCode ReferenceDomainInfoBuilderImpl::setReferenceTimeSource(IBoolean* referenceTimeSource)
{
    this->referenceTimeSource = referenceTimeSource;
    return OPENDAQ_SUCCESS;
}

ErrCode ReferenceDomainInfoBuilderImpl::getReferenceTimeSource(IBoolean** referenceTimeSource)
{
    OPENDAQ_PARAM_NOT_NULL(referenceTimeSource);
    *referenceTimeSource = this->referenceTimeSource.addRefAndReturn();
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
