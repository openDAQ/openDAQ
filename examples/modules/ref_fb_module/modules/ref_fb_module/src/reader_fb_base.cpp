#include <coretypes/function_factory.h>
#include <ref_fb_module/reader_fb_base.h>

BEGIN_NAMESPACE_REF_FB_MODULE

ReaderFbBase::ReaderFbBase(const FunctionBlockTypePtr& type, const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(type, ctx, parent, localId)
    , params(MultiReader2Params())
{
    initComponentStatus();
}

void ReaderFbBase::createReader()
{
    params.setAcceptsDescriptor(Function(
        [this, thisWeakRef = this->getWeakRefInternal<IFunctionBlock>()](
            ComponentPtr input, DataDescriptorPtr value, DataDescriptorPtr, ListPtr<IDataDescriptor> values) -> bool
        {
            const auto thisFb = thisWeakRef.getRef();
            return !thisFb.assigned() || accepts(input, value, values);
        }));
    reader = MultiReader2(params);
    reader.getOnDataAvailable() += [this, thisWeakRef = this->getWeakRefInternal<IFunctionBlock>()](BaseObjectPtr&, EventArgsPtr<>&)
    {
        const auto thisFb = thisWeakRef.getRef();
        if (thisFb.assigned())
            drain();
    };
    // The first status says what the configuration reads; it is owed until a read takes it
    SizeT count = 0;
    SizeT offset = 0;
    onChanges(reader.read(nullptr, &count, 0, offset));
}

void ReaderFbBase::drain()
{
    auto lock = getAcquisitionLock2();
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
            const auto it = cached.find(inputs[i].getGlobalId().toStdString());
            if (it == cached.end())
            {
                buffers[i] = nullptr;
                continue;
            }
            storage[i].resize(count * sampleSizes[it->first] / sizeof(double));  // a dimensioned input takes several per sample
            buffers[i] = storage[i].data();
        }

        SizeT offset = 0;
        const MultiReader2StatusPtr status = reader.read(count > 0 ? buffers.data() : nullptr, &count, 0, offset);
        if (count > 0)
            processAndSend(count, offset);  // under the cached descriptors

        if (status.getHasChanges())
            onChanges(status);  // may reconfigure; the next read then finds nothing
        else if (count == 0)
            return;
    }
}

// The output descriptors are in place before the component status says Ok, so a reader on the output sees them first
void ReaderFbBase::onChanges(const MultiReader2StatusPtr& status)
{
    if (status.getStatus() != ComponentStatus::Error)
        cacheDescriptors(status);
    setComponentStatusWithMessage(blockStatus(status), status.getStatusMessage());
}

ComponentStatus ReaderFbBase::blockStatus(const MultiReader2StatusPtr& status)
{
    if (status.getStatus() != ComponentStatus::Error)
        return status.getStatus();
    for (const MultiReader2InputStatusPtr in : status.getInputs())
    {
        const auto error = in.getError();
        if (error != MultiReader2InputError::None && error != MultiReader2InputError::Disconnected)
            return ComponentStatus::Error;
    }
    return ComponentStatus::Warning;
}

bool ReaderFbBase::scalarNumeric(const DataDescriptorPtr& descriptor)
{
    if (!descriptor.assigned())
        return false;
    const int sampleType = static_cast<int>(descriptor.getSampleType());
    if (sampleType == 0 || sampleType > static_cast<int>(SampleType::Int64))
        return false;
    const auto dimensions = descriptor.getDimensions();
    return !dimensions.assigned() || dimensions.getCount() == 0;
}

void ReaderFbBase::cacheDescriptors(const MultiReader2StatusPtr& status)
{
    if (status.getDomainDescriptorChanged())
        outputDomain = status.getDomainDescriptor();
    for (const MultiReader2InputStatusPtr in : status.getInputs())
    {
        const auto id = in.getInput().getGlobalId().toStdString();
        if (in.getError() != MultiReader2InputError::None)
        {
            cached.erase(id);
            sampleSizes.erase(id);
        }
        else if (in.getDescriptorChanged())
        {
            cached[id] = in.getDescriptor();
            sampleSizes[id] = in.getSampleSize();
        }
    }
    rebuildOutputDescriptor();
}

bool ReaderFbBase::accepts(const ComponentPtr&, const DataDescriptorPtr&, const ListPtr<IDataDescriptor>&)
{
    return true;
}

void ReaderFbBase::setInputs(const ListPtr<IComponent>& inputs)
{
    params.setInputs(inputs);
}

void ReaderFbBase::addFreePort(const std::string& prefix)
{
    freePortPrefix = prefix;
    freePort = createAndAddInputPort(fmt::format("{}_{}", prefix, nextPortId++), PacketReadyNotification::Scheduler);
}

ListPtr<IComponent> ReaderFbBase::readerPorts() const
{
    auto list = List<IComponent>();
    for (const auto& port : inputPorts.getItems())
        if (port != freePort)
            list.pushBack(port);
    return list;
}

// A signal connected to the free port, so the port joins the reader, which judges its descriptor, and the next free port
// is offered
void ReaderFbBase::onConnected(const InputPortPtr& port)
{
    auto lock = getAcquisitionLock2();
    if (!reader.assigned() || !freePort.assigned() || port != freePort)
        return;
    addFreePort(freePortPrefix);
    setInputs(readerPorts());
    onChanges(reader.configure(params));
}

std::vector<InputPortConfigPtr> ReaderFbBase::removeDisconnectedPorts(const MultiReader2StatusPtr& status, MultiReader2StatusPtr& next)
{
    std::vector<InputPortConfigPtr> disconnected;
    for (const MultiReader2InputStatusPtr in : status.getInputs())
        if (in.getError() == MultiReader2InputError::Disconnected && !in.getInput().asPtr<IInputPort>(true).getSignal().assigned())
            disconnected.push_back(in.getInput().asPtr<IInputPortConfig>(true));  // a port connected again since stays
    if (disconnected.empty())
        return disconnected;

    auto inputs = List<IComponent>();
    for (const auto& input : params.getInputs())
        if (std::find(disconnected.begin(), disconnected.end(), input) == disconnected.end())
            inputs.pushBack(input);
    setInputs(inputs);  // new params: every input is judged afresh
    next = reader.configure(params);
    for (const auto& port : disconnected)
        removeInputPort(port);
    return disconnected;
}

void ReaderFbBase::removed()
{
    reader.release();
    FunctionBlock::removed();
}

END_NAMESPACE_REF_FB_MODULE
