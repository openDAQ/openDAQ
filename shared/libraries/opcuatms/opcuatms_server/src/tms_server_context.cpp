#include <opcuatms_server/tms_server_context.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/instance_ptr.h>
#include <string>

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

ComponentPtr TmsServerContext::findComponent(const std::string& globalId)
{
    std::string relativeGlobalId = toRelativeGlobalId(globalId);
    return rootDevice.findComponent(relativeGlobalId);
}

void TmsServerContext::coreEventCallback(ComponentPtr& component, CoreEventArgsPtr& eventArgs)
{
    if (const auto it = idToObjMap.find(component.getGlobalId()); it != idToObjMap.end())
        if (const std::shared_ptr<tms::TmsServerObject> spt = it->second.lock())
            spt->onCoreEvent(eventArgs);
}

std::string TmsServerContext::toRelativeGlobalId(const std::string& globalId)
{
    const std::string rootDeviceId = rootDevice.getGlobalId().toStdString();
    std::string relativeGlobalId = globalId;

    if (relativeGlobalId.rfind(rootDeviceId, 0) == 0)
        relativeGlobalId = relativeGlobalId.substr(rootDeviceId.size());
    if (relativeGlobalId.rfind("/", 0) == 0)
        relativeGlobalId = relativeGlobalId.substr(1);

    return relativeGlobalId;
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
