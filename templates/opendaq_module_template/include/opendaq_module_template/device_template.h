#pragma once
#include <opendaq/device_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ

class DeviceTemplate : public DeviceParamsValidation, public Device, ComponentTemplateBase 
{
public:
    DeviceTemplate(const DeviceParams& params, const std::string& className = "")
        : DeviceParamsValidation(params)
        , Device(params.context, params.parent, params.localId, className, params.info.getName())
        , info(params.info)
        , initialized(false)
    {
        this->info = params.info;
        if (!this->info.isFrozen())
            this->info.freeze();

        loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
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

    LoggerComponentPtr loggerComponent;

private:
    void onObjectReady() override;
    DeviceInfoPtr onGetInfo() override;

    uint64_t onGetTicksSinceOrigin() override;

    DeviceInfoPtr info;
    bool initialized;
};

END_NAMESPACE_OPENDAQ
