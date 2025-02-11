#pragma once
#include <opendaq/module_impl.h>
#include <opendaq/device_info_config_ptr.h>
#include <opendaq_module_template/utils.h>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES

/* 
 * Module template TODO:
 *  - Test changeable device info fields
 */

class ModuleTemplateHooks;

class ModuleTemplate
{
public:
    ModuleTemplate(const ContextPtr& context)
        : context(context), moduleImpl(nullptr)
    {
    }

    virtual ~ModuleTemplate() = default;

protected:
    
    virtual ModuleParams buildModuleParams();
    virtual std::vector<DeviceTypeParams> getAvailableDeviceTypes(const DictPtr<IString, IBaseObject>& options);
    virtual std::vector<DeviceInfoParams> getAvailableDeviceInfo(const DictPtr<IString, IBaseObject>& options);
    virtual PropertyObjectPtr createDefaultConfiguration(const StringPtr& typeId);
    virtual DevicePtr createDevice(const DeviceParams& params);
    virtual void deviceRemoved(const std::string& deviceLocalId);

    friend class ModuleTemplateHooks;

    std::mutex sync;
    ContextPtr context;
    LoggerComponentPtr loggerComponent;
    ModuleTemplateHooks* moduleImpl;
    std::unordered_map<std::string, WeakRefPtr<IDevice>> devices;
};

class ModuleTemplateHooks : public ModuleParamsValidation, public Module
{
public:

    ModuleTemplateHooks(std::shared_ptr<ModuleTemplate> module_)
        : ModuleParamsValidation(module_->buildModuleParams())
        , Module(params.name, params.version, module_->context, params.id)
        , module_(std::move(module_))
    {
        this->module_->moduleImpl = this;
        this->module_->loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
        const auto userOptions = this->context.getModuleOptions(params.id);
        const auto defaultOptions = params.defaultOptions.assigned() ? params.defaultOptions : Dict<IString, IBaseObject>();
        this->options = mergeModuleOptions(userOptions, defaultOptions);
    }   

private:

    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config) override;

    static DeviceInfoPtr createDeviceInfo(const DeviceInfoParams& infoParams, const DeviceTypeParams& typeParams);

    static void populateModuleOptions(const DictPtr<IString, IBaseObject>& userOptions, const DictPtr<IString, IBaseObject>& defaultOptions);
    DictPtr<IString, IBaseObject> mergeModuleOptions(const DictPtr<IString, IBaseObject>& userOptions, const DictPtr<IString, IBaseObject>& defaultOptions) const;
    
    static void populateDefaultConfig(const PropertyObjectPtr& userConfig, const PropertyObjectPtr& defaultConfig);
    PropertyObjectPtr mergeConfig(const PropertyObjectPtr& userConfig, const PropertyObjectPtr& defaultConfig) const;

    void clearRemovedDevices() const;

    friend class ModuleTemplate;
    std::shared_ptr<ModuleTemplate> module_;
    DictPtr<IString, IBaseObject> options;
};


END_NAMESPACE_OPENDAQ_TEMPLATES
