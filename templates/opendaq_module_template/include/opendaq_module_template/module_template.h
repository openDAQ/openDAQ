#pragma once
#include <opendaq/module_impl.h>
#include <opendaq/device_info_config_ptr.h>
#include <opendaq_module_template/utils.h>

BEGIN_NAMESPACE_OPENDAQ

class ModuleTemplateHooks;

class ModuleTemplate
{
public:
    ModuleTemplate(const ContextPtr& context)
        : context(context)
    {
    }
    virtual ~ModuleTemplate() = default;

protected:
    
    virtual ModuleParams buildModuleParams();
    virtual std::vector<DeviceTypeParams> getAvailableDeviceTypes(const DictPtr<IString, IBaseObject>& options);
    virtual std::vector<DeviceInfoParams> getAvailableDeviceInfo(const DictPtr<IString, IBaseObject>& options);
    virtual DevicePtr createDevice(const DeviceParams& params);
    virtual void deviceRemoved(const std::string& deviceLocalId);

    friend class ModuleTemplateHooks;

    std::mutex sync;
    ContextPtr context;
    LoggerComponentPtr loggerComponent;
    ModuleTemplateHooks* moduleImpl;
    std::unordered_set<std::string> devices;
};

class ModuleTemplateHooks : public ModuleParamsValidation, public Module
{
public:

    ModuleTemplateHooks(std::unique_ptr<ModuleTemplate> module_)
        : ModuleParamsValidation(module_->buildModuleParams())
        , Module(params.name, params.version, module_->context, params.id)
        , module_(std::move(module_))
    {
        this->module_->moduleImpl = this;
        this->module_->loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
    }   

private:

    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config) override;
    static DeviceInfoPtr createDeviceInfo(const DeviceInfoParams& infoParams, const DeviceTypeParams& typeParams);
    
    friend class ModuleTemplate;
    std::unique_ptr<ModuleTemplate> module_;
};


END_NAMESPACE_OPENDAQ
