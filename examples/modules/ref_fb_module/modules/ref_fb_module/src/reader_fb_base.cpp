#include <ref_fb_module/reader_fb_base.h>
#include <opendaq/custom_log.h>

#include <sstream>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace
{
    const char* errorName(MultiReader2InputError error)
    {
        switch (error)
        {
            case MultiReader2InputError::Disconnected: return "not connected";
            case MultiReader2InputError::ValueDescriptorInvalid: return "value descriptor not readable";
            case MultiReader2InputError::DomainDescriptorInvalid: return "domain descriptor not compatible";
            case MultiReader2InputError::SyncFailed: return "not synchronized";
            default: return "ok";
        }
    }
}

ReaderFbBase::ReaderFbBase(const FunctionBlockTypePtr& type, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(type, ctx, parent, localId)
    , params(MultiReader2Params())
{
    initComponentStatus();
}

void ReaderFbBase::createReader()
{
    reader = MultiReader2(params);
    reader.getOnDataAvailable() += [this, thisWeakRef = this->getWeakRefInternal<IFunctionBlock>()](InputPortPtr&, EventArgsPtr<>&)
    {
        const auto thisFb = thisWeakRef.getRef();
        if (thisFb.assigned())
            drain();
    };
}

void ReaderFbBase::removed()
{
    reader.release();
    FunctionBlock::removed();
}

bool ReaderFbBase::contributes(SizeT slot) const
{
    return slot < active.size() && active[slot];
}

void ReaderFbBase::drain()
{
    auto lock = this->getAcquisitionLock2();
    if (!reader.assigned())
        return;

    for (;;)
    {
        SizeT count = reader.getAvailableCount();
        const SizeT slots = params.getInputs().getCount();
        storage.resize(slots);
        buffers.resize(slots);
        for (SizeT i = 0; i < slots; i++)
        {
            storage[i].resize(count);
            buffers[i] = storage[i].data();
        }

        SizeT offset = 0;
        const MultiReader2StatusPtr status = reader.read(count > 0 ? buffers.data() : nullptr, &count, offset);
        if (count > 0)
            processAndSend(count, offset);

        if (status.getHasChanges())
        {
            if (onChanges(status))
                return;  // reconfigured; the next wake starts over
        }
        else if (count == 0)
        {
            return;
        }
    }
}

bool ReaderFbBase::onChanges(const MultiReader2StatusPtr& status)
{
    if (applyRejections(status))
        return true;

    // The slot order and the contributing set, for the data that follows this status
    const auto inputs = status.getInputs();
    slotInputs.clear();
    active.assign(inputs.getCount(), false);
    for (SizeT i = 0; i < inputs.getCount(); i++)
    {
        const MultiReader2InputStatusPtr in = inputs[i];
        slotInputs.push_back(in.getInput());
        active[i] = status.getValid() && in.getUsed() && in.getError() == MultiReader2InputError::None;
    }

    // The output descriptors are in place before the status says Ok, so a reader on the output sees them first
    if (status.getValid())
        cacheDescriptors(status);
    reportStatus(status);
    return false;
}

bool ReaderFbBase::applyRejections(const MultiReader2StatusPtr& status)
{
    if (!status.getValid())
        return false;

    const auto mainInput = params.getMainInput();
    bool changed = false;
    for (const MultiReader2InputStatusPtr in : status.getInputs())
    {
        if (!in.getDescriptorChanged())
            continue;
        const auto input = in.getInput();
        if (mainInput.assigned() && mainInput.getGlobalId() == input.getGlobalId())
            continue;  // a pinned main input is left alone

        const bool wantUsed = accepts(in.getDescriptor());
        const auto id = input.getGlobalId().toStdString();
        if (wantUsed)
            rejected.erase(id);
        else
            rejected.insert(id);
        if (static_cast<bool>(in.getUsed()) != wantUsed)
        {
            params.setInputUsed(input, wantUsed);
            changed = true;
        }
    }
    if (changed)
        reader.configure(params);
    return changed;
}

void ReaderFbBase::cacheDescriptors(const MultiReader2StatusPtr& status)
{
    if (status.getDomainDescriptorChanged())
        outputDomain = status.getDomainDescriptor();
    for (const MultiReader2InputStatusPtr in : status.getInputs())
    {
        if (in.getDescriptorChanged())
            cached[in.getInput().getGlobalId().toStdString()] = in.getDescriptor();
    }
    rebuildOutputDescriptor();
}

bool ReaderFbBase::accepts(const DataDescriptorPtr& /*descriptor*/)
{
    return true;
}

void ReaderFbBase::reportStatus(const MultiReader2StatusPtr& status)
{
    std::ostringstream message;
    bool anyActive = false;
    for (const MultiReader2InputStatusPtr in : status.getInputs())
    {
        const auto id = in.getInput().getGlobalId().toStdString();
        anyActive = anyActive || (status.getValid() && in.getUsed() && in.getError() == MultiReader2InputError::None);
        if (!status.getValid() && in.getUsed() && in.getError() != MultiReader2InputError::None)
            message << (message.tellp() > 0 ? "; " : "") << in.getInput().getLocalId() << " " << errorName(in.getError());
        else if (rejected.count(id) > 0)
            message << (message.tellp() > 0 ? "; " : "") << in.getInput().getLocalId() << " rejected";
    }

    if (!status.getValid())
        setComponentStatusWithMessage(ComponentStatus::Warning, "Waiting for inputs: " + message.str());
    else if (!anyActive)
        setComponentStatusWithMessage(ComponentStatus::Warning, "No signals connected!");
    else if (message.tellp() > 0)
        setComponentStatusWithMessage(ComponentStatus::Warning, "Inputs not accepted: " + message.str());
    else if (!outputError.empty())
        setComponentStatusWithMessage(ComponentStatus::Warning, outputError);
    else
        setComponentStatus(ComponentStatus::Ok);
}

END_NAMESPACE_REF_FB_MODULE
