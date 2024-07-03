#include <opendaq_module_template/device_template.h>

BEGIN_NAMESPACE_OPENDAQ

DeviceTemplate::DeviceTemplate(const StringPtr& localId,
                               const DeviceInfoPtr& info,
                               const StringPtr& logName,
                               const ContextPtr& context,
                               const ComponentPtr& parent,
                               const StringPtr& className)
    : Device(context, parent, localId, className, info.getName())
    , info(info)
{
    if (!localId.assigned() || localId == "")
        throw InvalidParametersException("Local id is not set");
    if (!info.assigned())
        throw InvalidParametersException("Device info is not set");
    if (!logName.assigned() || logName == "")
        throw InvalidParametersException("Log name is not set");
    if (!context.assigned())
        throw InvalidParametersException("Context is not set");

    this->info = info;
    if (!this->info.isFrozen())
        this->info.freeze();

    loggerComponent = this->context.getLogger().getOrAddComponent(logName);
}

DeviceInfoPtr DeviceTemplate::onGetInfo()
{
    return info;
}

END_NAMESPACE_OPENDAQ
