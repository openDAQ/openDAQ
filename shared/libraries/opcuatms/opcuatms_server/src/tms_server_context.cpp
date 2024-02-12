#include <opcuatms_server/tms_server_context.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/instance_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

TmsServerContext::TmsServerContext(const ContextPtr& context, const DevicePtr& rootDevice)
    : context(context)
    , rootDevice(rootDevice)
{
    this->context.getOnCoreEvent() += event(this, &TmsServerContext::coreEventCallback);
}

TmsServerContext::~TmsServerContext()
{
    this->context.getOnCoreEvent() -= event(this, &TmsServerContext::coreEventCallback);
}

void TmsServerContext::registerComponent(const ComponentPtr& component, tms::TmsServerObject& obj)
{
    idToObjMap.insert(std::make_pair(component.getGlobalId(), obj.weak_from_this()));
}

DevicePtr TmsServerContext::getRootDevice()
{
    return rootDevice;
}

SignalPtr TmsServerContext::findSingal(const StringPtr& globalId)
{
    const auto& signals = rootDevice.getSignals(search::Recursive(search::Any()));

    for (const auto& signal : signals)
    {
        if (signal.getGlobalId() == globalId)
            return signal;
    }

    return nullptr;
}

void TmsServerContext::coreEventCallback(ComponentPtr& component, CoreEventArgsPtr& eventArgs)
{
    if (const auto it = idToObjMap.find(component.getGlobalId()); it != idToObjMap.end())
        if (const std::shared_ptr<tms::TmsServerObject> spt = it->second.lock())
            spt->onCoreEvent(eventArgs);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
