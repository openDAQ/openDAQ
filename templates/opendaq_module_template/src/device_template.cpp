#include <opendaq_module_template/device_template.h>

BEGIN_NAMESPACE_OPENDAQ

DeviceTemplate::DeviceTemplate(const DeviceTemplateParams& params)
    : DeviceTemplateParamsValidation(params)
    , Device(params.context, params.parent, params.localId, params.className, params.info.getName())
    , info(params.info)
    , allowAddDevices(params.allowAddDevices)
    , allowAddFunctionBlocks(params.allowAddFunctionBlocks)
{
    this->info = params.info;
    if (!this->info.isFrozen())
        this->info.freeze();

    loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
}

DeviceInfoPtr DeviceTemplate::onGetInfo()
{
    return info;
}

bool DeviceTemplate::allowAddDevicesFromModules()
{
    return allowAddDevices;
}

bool DeviceTemplate::allowAddFunctionBlocksFromModules()
{
    return allowAddFunctionBlocks;
}

END_NAMESPACE_OPENDAQ
