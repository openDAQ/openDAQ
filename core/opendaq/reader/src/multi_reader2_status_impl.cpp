#include <coretypes/validation.h>
#include <opendaq/multi_reader2_status_impl.h>

BEGIN_NAMESPACE_OPENDAQ

MultiReader2StatusImpl::MultiReader2StatusImpl(const DataDescriptorPtr& domainDescriptor,
                                               MultiReader2StatusType status,
                                               const DictPtr<IString, IDataDescriptor>& descriptors,
                                               const DictPtr<IString, IInteger>& errors,
                                               const DictPtr<IString, IInteger>& dividers)
    : domainDescriptor(domainDescriptor)
    , status(status)
    , descriptors(descriptors.assigned() ? descriptors : Dict<IString, IDataDescriptor>())
    , errors(errors.assigned() ? errors : Dict<IString, IInteger>())
    , dividers(dividers.assigned() ? dividers : Dict<IString, IInteger>())
{
}

ErrCode MultiReader2StatusImpl::getStatus(MultiReader2StatusType* status)
{
    OPENDAQ_PARAM_NOT_NULL(status);

    *status = this->status;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getDomainDescriptor(IDataDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    *descriptor = domainDescriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getDescriptors(IDict** descriptors)
{
    OPENDAQ_PARAM_NOT_NULL(descriptors);

    *descriptors = this->descriptors.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getDividers(IDict** dividers)
{
    OPENDAQ_PARAM_NOT_NULL(dividers);

    *dividers = this->dividers.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getErrors(IDict** errors)
{
    OPENDAQ_PARAM_NOT_NULL(errors);

    *errors = this->errors.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
