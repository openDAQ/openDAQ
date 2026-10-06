#include <ref_fb_module/reader_fb_base.h>
#include <coretypes/function_factory.h>
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
    // The judgement runs inside read and configure, both called by this block under its own lock
    params.setAcceptsDescriptor(Function([this, thisWeakRef = this->getWeakRefInternal<IFunctionBlock>()](ComponentPtr input, DataDescriptorPtr value, DataDescriptorPtr) -> bool
    {
        const auto thisFb = thisWeakRef.getRef();
        return !thisFb.assigned() || accepts(input, value);
    }));
    reader = MultiReader2(params);
    reader.getOnDataAvailable() += [this, thisWeakRef = this->getWeakRefInternal<IFunctionBlock>()](InputPortPtr&, EventArgsPtr<>&)
    {
        const auto thisFb = thisWeakRef.getRef();
        if (thisFb.assigned())
            drain();
    };
}

// ---------------------------------------------------------------- the free port

void ReaderFbBase::addFreePort(const std::string& prefix)
{
    freePortPrefix = prefix;
    freePort = createAndAddInputPort(fmt::format("{}_{}", prefix, nextPortId++), PacketReadyNotification::Scheduler);
}

ListPtr<IComponent> ReaderFbBase::readerPorts() const
{
    auto list = List<IComponent>();
    for (const auto& port : inputPorts.getItems())
    {
        if (port != freePort)
            list.pushBack(port);
    }
    return list;
}

// A signal connected to the free port: the port joins the reader, which judges the descriptor, and the next
// free port is offered. Under the block's lock, since drain reconfigures and reads under the same lock
void ReaderFbBase::onConnected(const InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock2();
    if (!reader.assigned() || !freePort.assigned() || port != freePort)
        return;
    addFreePort(freePortPrefix);
    params.setInputs(readerPorts());
    reader.configure(params);
}

void ReaderFbBase::removed()
{
    reader.release();
    FunctionBlock::removed();
}

void ReaderFbBase::drain()
{
    auto lock = this->getAcquisitionLock2();
    if (!reader.assigned())
        return;

    for (;;)
    {
        SizeT count = reader.getAvailableCount();
        const auto inputs = params.getInputs();
        storage.resize(inputs.getCount());
        buffers.resize(inputs.getCount());
        for (SizeT i = 0; i < inputs.getCount(); i++)
        {
            // Only contributing inputs are written; the others get no buffer
            if (cached.count(inputs[i].getGlobalId().toStdString()) == 0)
            {
                buffers[i] = nullptr;
                continue;
            }
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
    // The output descriptors are in place before the status says Ok, so a reader on the output sees them first
    if (status.getValid())
        cacheDescriptors(status);
    reportStatus(status);
    return false;
}

void ReaderFbBase::cacheDescriptors(const MultiReader2StatusPtr& status)
{
    if (status.getDomainDescriptorChanged())
        outputDomain = status.getDomainDescriptor();
    for (const MultiReader2InputStatusPtr in : status.getInputs())
    {
        const auto id = in.getInput().getGlobalId().toStdString();
        if (in.getError() != MultiReader2InputError::None)
            cached.erase(id);
        else if (in.getDescriptorChanged())
            cached[id] = in.getDescriptor();
    }
    rebuildOutputDescriptor();
}

bool ReaderFbBase::accepts(const ComponentPtr& /*input*/, const DataDescriptorPtr& /*descriptor*/)
{
    return true;
}

void ReaderFbBase::reportStatus(const MultiReader2StatusPtr& status)
{
    std::ostringstream message;
    bool anyActive = false;
    for (const MultiReader2InputStatusPtr in : status.getInputs())
    {
        anyActive = anyActive || (status.getValid() && in.getError() == MultiReader2InputError::None);
        if (in.getError() != MultiReader2InputError::None)
            message << (message.tellp() > 0 ? "; " : "") << in.getInput().getLocalId() << " " << errorName(in.getError());
    }

    if (!status.getValid())
        setComponentStatusWithMessage(ComponentStatus::Warning, "Waiting for inputs: " + message.str());
    else if (!anyActive)
        setComponentStatusWithMessage(ComponentStatus::Warning, "No signals connected!");
    else if (message.tellp() > 0)
        setComponentStatusWithMessage(ComponentStatus::Warning, "Inputs set aside: " + message.str());
    else if (!outputError.empty())
        setComponentStatusWithMessage(ComponentStatus::Warning, outputError);
    else
        setComponentStatus(ComponentStatus::Ok);
}

END_NAMESPACE_REF_FB_MODULE
