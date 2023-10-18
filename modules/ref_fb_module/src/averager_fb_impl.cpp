#include <ref_fb_module/averager_fb_impl.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace Averager
{

AveragerFbImpl::AveragerFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , avgCtxIndex(0)
{
    initProperties();
    updateAveragerContexts();
}

FunctionBlockTypePtr AveragerFbImpl::CreateType()
{
    return FunctionBlockType(
        "ref_fb_module_averager",
        "Averager",
        "Calculates statistics"
    );
}

void AveragerFbImpl::initProperties()
{
    objPtr.addProperty(IntProperty("BlockSize", 10));
    objPtr.getOnPropertyValueWrite("BlockSize") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    objPtr.addProperty(SelectionProperty("DomainSignalType", List<IString>("Implicit", "Explicit", "ExplicitRange"), 0));
    objPtr.getOnPropertyValueWrite("DomainSignalType") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    readProperties();
}

void AveragerFbImpl::propertyChanged()
{
    std::scoped_lock lock(sync);
    readProperties();

    configureAveragerContexts();
}

void AveragerFbImpl::configureAveragerContexts()
{
    for (auto& avgContext : averagerContexts)
        configureAveragerContext(avgContext->getAveragerContext());
}

void AveragerFbImpl::configureAveragerContext(AveragerContext& avgContext)
{
    std::scoped_lock lock(avgContext.getSync());

    avgContext.blockSize = blockSize;
    avgContext.domainSignalType = domainSignalType;
    avgContext.configure();
}

void AveragerFbImpl::readProperties()
{
    blockSize = objPtr.getPropertyValue("BlockSize");
    domainSignalType = static_cast<DomainSignalType>(static_cast<Int>(objPtr.getPropertyValue("DomainSignalType")));
    LOG_I("Properties: BlockSize {}, DomainSignalType {}", blockSize, objPtr.getPropertySelectionValue("DomainSignalType").toString());
}

AveragerContext& AveragerFbImpl::getAveragerContext(const InputPortConfigPtr& inputPort)
{
    auto avgCtxPtr = inputPort.getCustomData().asPtr<IAveragerContext>(true);
    return avgCtxPtr->getAveragerContext();
}

void AveragerFbImpl::removed()
{
    for (auto& averagerContext : averagerContexts)
        averagerContext.remove();
    LOG_I("Removed");
}

void AveragerFbImpl::updateAveragerContexts()
{
    for (auto it = averagerContexts.begin(); it != averagerContexts.end();)
    {
        auto avgContext = *it;
        auto inputPort = avgContext->getAveragerContext().inputPort;
        if (!inputPort.getSignal().assigned())
        {
            avgContext.remove();
            it = averagerContexts.erase(it);
        }
        else
            ++it;
    }

    const auto newAveragerContext = createWithImplementation<IAveragerContext, AveragerContext>(
        thisPtr<InputPortNotificationsPtr>(),
        context,
        this->functionBlocks,
        StringPtr(fmt::format("avgctx{}", avgCtxIndex++)));

    configureAveragerContext(newAveragerContext->getAveragerContext());

    averagerContexts.push_back(newAveragerContext);
}

void AveragerFbImpl::onConnected(const InputPortPtr& inputPort)
{
    std::scoped_lock lock(sync);
    updateAveragerContexts();
    LOG_T("Connected to port {}", inputPort.getLocalId());
}

void AveragerFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    std::scoped_lock lock(sync);
    updateAveragerContexts();
    LOG_T("Disconnected from port {}", inputPort.getLocalId());
}

ErrCode AveragerFbImpl::getInputPorts(IList** inputPorts)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    auto inputPortList = List<IInputPort>();

    std::scoped_lock lock(sync);

    for (auto& avgContext: averagerContexts)
    {
        auto avgContextInputPorts = avgContext.getInputPorts();
        for (const auto& ip : avgContextInputPorts)
            inputPortList.pushBack(ip);
    }

    *inputPorts = inputPortList.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode AveragerFbImpl::getSignals(IList** signals)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    auto signalList = List<ISignal>();

    std::scoped_lock lock(sync);

    for (auto& avgContext : averagerContexts)
    {
        auto avgContextSignals = avgContext.getSignals();
        for (const auto& signal : avgContextSignals)
            signalList.pushBack(signal);
    }

    *signals = signalList.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode AveragerFbImpl::getSignalsRecursive(IList** signals)
{
    return getSignals(signals);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    AveragerFb,
    IFunctionBlock,
    IContext*, context,
    IString*, parentGlobalId,
    IString*, localId);

}

END_NAMESPACE_REF_FB_MODULE
