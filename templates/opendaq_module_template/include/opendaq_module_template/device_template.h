#pragma once
#include <opendaq/device_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES

class DeviceTemplateHooks;

class DeviceTemplate : public ComponentTemplateBase<DeviceTemplateHooks>, public AddableComponentTemplateBase
{
public:

    DevicePtr getDevice() const;

    template <class ChannelImpl, class... Params>
    ChannelPtr createAndAddChannel(const IoFolderConfigPtr& parentFolder, const std::string& channelId, Params&&... params) const;
    IoFolderConfigPtr createAndAddIOFolder(const std::string& folderId, const IoFolderConfigPtr& parent) const;

    void setDeviceDomain(const DeviceDomainPtr& deviceDomain) const;

protected:
    
    virtual void initIOFolder(const IoFolderConfigPtr& ioFolder);
    virtual void initDevices(const FolderConfigPtr& devicesFolder);
    virtual void initSyncComponent(const SyncComponentPrivatePtr& syncComponent);
    virtual void initCustomComponents();
    virtual DeviceDomainPtr initDeviceDomain();  // TODO: Pass builder as param when implemented

    virtual uint64_t getTicksSinceOrigin();

    // Should this be handled differently?
    virtual bool allowAddDevicesFromModules();
    virtual bool allowAddFunctionBlocksFromModules();
    
private:

    friend class DeviceTemplateHooks;
};

template <class ChannelImpl, class ... Params>
ChannelPtr DeviceTemplate::createAndAddChannel(const IoFolderConfigPtr& parentFolder, const std::string& channelId, Params&&... params) const
{
    LOG_T("Adding channel {}", channelId)
    return componentImpl->createAndAddChannel<ChannelImpl, Params...>(parentFolder, channelId, std::forward<Params>(params)...);
}

class DeviceTemplateHooks : public DeviceParamsValidation, public Device
{
public:

    DeviceTemplateHooks(std::shared_ptr<DeviceTemplate> device, const DeviceParams& params, const StringPtr& className = "")
        : DeviceParamsValidation(params)
        , Device(params.context, params.parent, params.localId, className, params.info.getName())
        , device(std::move(device))
        , info(params.info)
    {
        if (!this->info.isFrozen())
            this->info.freeze();
            
        this->device->componentImpl = this; // TODO: Figure out safe ptr operations for this
        this->device->loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
        this->device->context = this->context;

        std::scoped_lock lock(sync);

        this->device->handleConfig(params.config);
        this->device->handleOptions(params.options);
        this->device->initProperties();

        registerCallbacks<DeviceTemplate>(objPtr, this->device);
        setDeviceDomain(this->device->initDeviceDomain());
        this->device->initIOFolder(ioFolder);
        this->device->initDevices(devices);
        this->device->initSignals(signals);
        this->device->initFunctionBlocks(functionBlocks);
        this->device->initCustomComponents();
        this->device->initSyncComponent(syncComponent);

        this->device->initTags(tags);
        this->device->initStatuses(statusContainer);

        this->device->start();
    }   

private:
    
    DeviceInfoPtr onGetInfo() override;
    uint64_t onGetTicksSinceOrigin() override;

    bool allowAddDevicesFromModules() override;
    bool allowAddFunctionBlocksFromModules() override;
    
    friend class DeviceTemplate;
    std::shared_ptr<DeviceTemplate> device;
    DeviceInfoPtr info;
};

END_NAMESPACE_OPENDAQ_TEMPLATES
