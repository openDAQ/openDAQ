#pragma once
#include <opendaq/device_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ


class DeviceTemplate : public DeviceTemplateParamsValidation, public Device, ComponentTemplateBase 
{
public:
    DeviceTemplate(const DeviceTemplateParams& params);

protected:
    LoggerComponentPtr loggerComponent;

private:
    DeviceInfoPtr onGetInfo() override;

    bool allowAddDevicesFromModules() override;
    bool allowAddFunctionBlocksFromModules() override;

    DeviceInfoPtr info;
    bool allowAddDevices;
    bool allowAddFunctionBlocks;
};

END_NAMESPACE_OPENDAQ
