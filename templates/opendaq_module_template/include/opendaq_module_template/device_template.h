#pragma once
#include <opendaq/device_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ

class DeviceTemplate : public DeviceTemplateParamsValidation, public Device, ComponentTemplateBase 
{
public:
    DeviceTemplate(const DeviceTemplateParams& params)
        : DeviceTemplateParamsValidation(params)
        , Device(params.context, params.parent, params.localId, params.className, params.info.getName())
        , info(params.info)
        , allowAddDevices(params.allowAddDevices)
        , allowAddFunctionBlocks(params.allowAddFunctionBlocks)
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
    virtual Int getTicksSinceOrigin();

    LoggerComponentPtr loggerComponent;

private:
    void onObjectReady() override;
    DeviceInfoPtr onGetInfo() override;

    bool allowAddDevicesFromModules() override;
    bool allowAddFunctionBlocksFromModules() override;

    DeviceInfoPtr info;
    bool allowAddDevices;
    bool allowAddFunctionBlocks;
    bool initialized;
};

END_NAMESPACE_OPENDAQ
