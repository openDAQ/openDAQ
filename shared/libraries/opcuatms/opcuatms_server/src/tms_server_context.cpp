#include <opcuatms_server/tms_server_context.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

TmsServerContext::TmsServerContext(const ContextPtr& context)
    : context(context)
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

void TmsServerContext::coreEventCallback(ComponentPtr& component, CoreEventArgsPtr& eventArgs)
{
    if (const auto it = idToObjMap.find(component.getGlobalId()); it != idToObjMap.end())
        if (const std::shared_ptr<tms::TmsServerObject> spt = it->second.lock())
            spt->onCoreEvent(eventArgs);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
