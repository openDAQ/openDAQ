#include <coretypes/string_ptr.h>
#include <opendaq/multi_reader2_status_impl.h>

BEGIN_NAMESPACE_OPENDAQ

MultiReader2InputStatusImpl::MultiReader2InputStatusImpl(
    ComponentPtr input, MultiReader2InputError error, bool descriptorChanged, DataDescriptorPtr descriptor, SizeT sampleSize)
    : input(std::move(input))
    , error(error)
    , descriptorChanged(descriptorChanged)
    , descriptor(std::move(descriptor))
    , sampleSize(sampleSize)
{
}

ErrCode MultiReader2InputStatusImpl::getInput(IComponent** input)
{
    OPENDAQ_PARAM_NOT_NULL(input);
    *input = this->input.addRefAndReturn();
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

ErrCode MultiReader2InputStatusImpl::getSampleSize(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);
    *size = sampleSize;
    return OPENDAQ_SUCCESS;
}

MultiReader2StatusImpl::MultiReader2StatusImpl(Fields fields)
    : fields(std::move(fields))
{
    // A read without changes hands the same status out again
    this->fields.inputs.freeze();
}

ErrCode MultiReader2StatusImpl::getHasChanges(Bool* hasChanges)
{
    OPENDAQ_PARAM_NOT_NULL(hasChanges);
    *hasChanges = fields.hasChanges;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getStatus(ComponentStatus* status)
{
    OPENDAQ_PARAM_NOT_NULL(status);
    *status = fields.status;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getStatusMessage(IString** message)
{
    OPENDAQ_PARAM_NOT_NULL(message);
    *message = String(fields.message).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getDomainDescriptorChanged(Bool* changed)
{
    OPENDAQ_PARAM_NOT_NULL(changed);
    *changed = fields.domainDescriptorChanged;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getDomainDescriptor(IDataDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);
    *descriptor = fields.domainDescriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getMainInput(IComponent** input)
{
    OPENDAQ_PARAM_NOT_NULL(input);
    *input = fields.mainInput.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getResynchronized(Bool* resynchronized)
{
    OPENDAQ_PARAM_NOT_NULL(resynchronized);
    *resynchronized = fields.resynchronized;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getSynchronizing(Bool* synchronizing)
{
    OPENDAQ_PARAM_NOT_NULL(synchronizing);
    *synchronizing = fields.synchronizing;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getInputs(IList** inputs)
{
    OPENDAQ_PARAM_NOT_NULL(inputs);
    *inputs = fields.inputs.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2StatusImpl::getInputStatus(IComponent* input, IMultiReader2InputStatus** status)
{
    OPENDAQ_PARAM_NOT_NULL(input);
    OPENDAQ_PARAM_NOT_NULL(status);
    for (const MultiReader2InputStatusPtr entry : fields.inputs)
    {
        // By identity: several inputs may carry the same name
        if (entry.getInput().getObject() == input)
        {
            *status = entry.addRefAndReturn();
            return OPENDAQ_SUCCESS;
        }
    }
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The input is not on this status");
}

END_NAMESPACE_OPENDAQ
