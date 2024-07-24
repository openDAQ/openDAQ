#include <opendaq_module_template/device_template.h>

BEGIN_NAMESPACE_OPENDAQ

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
