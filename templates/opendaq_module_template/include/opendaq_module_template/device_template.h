#pragma once
#include <opendaq/device_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ

class DeviceTemplate : public Device, ComponentTemplateBase
{
public:
    DeviceTemplate(const StringPtr& localId,
                   const DeviceInfoPtr& info,
                   const StringPtr& logName,
                   const ContextPtr& context,
                   const ComponentPtr& parent,
                   const StringPtr& className = nullptr);
protected:
    LoggerComponentPtr loggerComponent;

private:
    DeviceInfoPtr onGetInfo() override;
    DeviceInfoPtr info;
};

END_NAMESPACE_OPENDAQ
