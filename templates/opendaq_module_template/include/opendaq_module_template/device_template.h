#pragma once
#include <opendaq/device_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ

class DeviceTemplateHooks;

class DeviceTemplate : ComponentTemplateBase 
{
public:
    DeviceTemplate(const DeviceParams& params)
        : params(params)
    {
    }

protected:
    
    virtual void configureSignals(const FolderConfigPtr& signalsFolder);
    virtual void configureDevices(const FolderConfigPtr& devicesFolder);
    virtual void configureFunctionBlocks(const FolderConfigPtr& fbFolder);
    virtual void configureIOComponents(const IoFolderConfigPtr& ioFolder);
    virtual void configureCustomComponents();

    
    //virtual void configureSyncComponent(); // TODO: Add once sync component is implemented

    virtual DeviceDomainPtr getDeviceDomain();
    virtual uint64_t getTicksSinceOrigin();

    virtual bool allowAddDevicesFromModules();
    virtual bool allowAddFunctionBlocksFromModules();

    
    std::weak_ptr<DeviceTemplateHooks> deviceImpl;
    LoggerComponentPtr loggerComponent;
    std::mutex sync;

private:
    friend class DeviceTemplateHooks;
    DeviceParams params;
};

class DeviceTemplateHooks : public DeviceParamsValidation, public Device, std::enable_shared_from_this<DeviceTemplateHooks>
{
public:

    DeviceTemplateHooks(std::unique_ptr<DeviceTemplate> device, const StringPtr& className = "")
        : DeviceParamsValidation(device->params)
        , Device(params.context, params.parent, params.localId, className, params.info.getName())
        , device(std::move(device))
        , info(params.info)
        , initialized(false)
    {
        if (!this->info.isFrozen())
            this->info.freeze();

        this->device->deviceImpl = weak_from_this();
        this->device->loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
    }   

private:
    
    void onObjectReady() override;
    DeviceInfoPtr onGetInfo() override;
    uint64_t onGetTicksSinceOrigin() override;
    
    friend class DeviceTemplate;
    std::unique_ptr<DeviceTemplate> device;
    DeviceInfoPtr info;
    bool initialized;
};

END_NAMESPACE_OPENDAQ
