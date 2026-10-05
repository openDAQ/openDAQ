#include <coretypes/validation.h>
#include <opendaq/multi_reader2_status_impl.h>

BEGIN_NAMESPACE_OPENDAQ

MultiReader2InputStatusImpl::MultiReader2InputStatusImpl(const ComponentPtr& input,
                                                         bool used,
                                                         MultiReader2InputError error,
                                                         bool descriptorChanged,
                                                         const DataDescriptorPtr& descriptor)
    : input(input)
    , used(used)
    , error(error)
    , descriptorChanged(descriptorChanged)
    , descriptor(descriptor)
{
}

ErrCode MultiReader2InputStatusImpl::getInput(IComponent** input)
{
    OPENDAQ_PARAM_NOT_NULL(input);
    *input = this->input.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2InputStatusImpl::getUsed(Bool* used)
{
    OPENDAQ_PARAM_NOT_NULL(used);
    *used = this->used;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2InputStatusImpl::getError(MultiReader2InputError* error)
{
    OPENDAQ_PARAM_NOT_NULL(error);
    *error = this->error;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2InputStatusImpl::getDescriptorChanged(Bool* changed)
{
    OPENDAQ_PARAM_NOT_NULL(changed);
    *changed = descriptorChanged;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2InputStatusImpl::getDescriptor(IDataDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);
    *descriptor = this->descriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

MultiReader2StatusImpl::MultiReader2StatusImpl(bool hasChanges,
                                               bool valid,
                                               bool domainDescriptorChanged,
                                               const DataDescriptorPtr& domainDescriptor,
                                               bool resynchronized,
                                               const ListPtr<IMultiReader2InputStatus>& inputs)
    : hasChanges(hasChanges)
    , valid(valid)
    , domainDescriptorChanged(domainDescriptorChanged)
    , domainDescriptor(domainDescriptor)
    , resynchronized(resynchronized)
    , inputs(inputs)
{
}

ErrCode MultiReader2StatusImpl::getHasChanges(Bool* hasChanges)
{
    OPENDAQ_PARAM_NOT_NULL(hasChanges);
    *hasChanges = this->hasChanges;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getValid(Bool* valid)
{
    OPENDAQ_PARAM_NOT_NULL(valid);
    *valid = this->valid;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getDomainDescriptorChanged(Bool* changed)
{
    OPENDAQ_PARAM_NOT_NULL(changed);
    *changed = domainDescriptorChanged;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getDomainDescriptor(IDataDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);
    *descriptor = domainDescriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getResynchronized(Bool* resynchronized)
{
    OPENDAQ_PARAM_NOT_NULL(resynchronized);
    *resynchronized = this->resynchronized;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getInputs(IList** inputs)
{
    OPENDAQ_PARAM_NOT_NULL(inputs);
    *inputs = this->inputs.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getInputStatus(IComponent* input, IMultiReader2InputStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(input);
    OPENDAQ_PARAM_NOT_NULL(status);

    const auto id = ComponentPtr::Borrow(input).getGlobalId();
    for (const MultiReader2InputStatusPtr entry : inputs)
    {
        if (entry.getInput().getGlobalId() == id)
        {
            *status = entry.addRefAndReturn();
            return OPENDAQ_SUCCESS;
        }
    }
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, R"(Input "%s" is not an input of the reader)", id.getCharPtr());
}

END_NAMESPACE_OPENDAQ
