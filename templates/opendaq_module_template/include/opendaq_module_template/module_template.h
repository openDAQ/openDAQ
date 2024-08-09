#pragma once
#include <opendaq/module_impl.h>
#include <opendaq/device_info_config_ptr.h>
#include <opendaq_module_template/utils.h>
#include <map>

BEGIN_NAMESPACE_OPENDAQ

struct DeviceInfoFields
{
    std::string address;
    std::string name;
    std::string manufacturer;
    std::string manufacturerUri;
    std::string model;
    std::string productCode;
    std::string deviceRevision;
    std::string hardwareRevision;
    std::string softwareRevision;
    std::string deviceManual;
    std::string deviceClass;
    std::string serialNumber;
    std::string productInstanceUri;
    int revisionCounter;
    std::string assetId;
    std::string macAddress;
    std::string parentMacAddress;
    std::string platform;
    int position;
    std::string systemType;
    std::string systemUuid;
    std::string location;
    std::map<std::string, std::string> other;
};

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

    virtual std::vector<DeviceInfoFields> getDeviceInfoFields(const std::string& typeId, const DictPtr<IString, IBaseObject>& options);
    virtual std::vector<DeviceTypePtr> getAvailableDeviceTypes();
    virtual DevicePtr createDevice(const CreateDeviceParams& params);
    virtual void deviceRemoved(const std::string& deviceLocalId);
    virtual ModuleTemplateParams buildModuleTemplateParams(const ContextPtr& context);

    friend class ModuleTemplateHooks;

    std::mutex sync;
    ContextPtr context;
    LoggerComponentPtr loggerComponent;
    std::weak_ptr<ModuleTemplateHooks> moduleImpl;

private:

    static DeviceInfoPtr createDeviceInfo(const DeviceInfoFields& fields, const DeviceTypePtr& type);
};

class ModuleTemplateHooks : public ModuleTemplateParamsValidation, public Module, std::enable_shared_from_this<ModuleTemplateHooks>
{
public:

    ModuleTemplateHooks(std::unique_ptr<ModuleTemplate> module_, const ContextPtr& context)
        : ModuleTemplateParamsValidation(module_->buildModuleTemplateParams(context))
        , Module(params.name, params.version, params.context, params.id)
        , module_(std::move(module_))
    {
        this->module_->moduleImpl = weak_from_this();
        this->module_->loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
    }   

private:

    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config) override;
    
    friend class ModuleTemplate;
    std::unique_ptr<ModuleTemplate> module_;
};


END_NAMESPACE_OPENDAQ
