#pragma once
#include <opendaq/device_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ

class DeviceTemplateHooks;

class DeviceTemplate : public ComponentTemplateBase<DeviceTemplateHooks>
{
public:
    DevicePtr getDevice() const;

    template <class ChannelImpl, class... Params>
    ChannelPtr createAndAddChannel(const IoFolderConfigPtr& parentFolder, const std::string& channelId, Params&&... params) const;

    IoFolderConfigPtr createAndAddIOFolder(const std::string& folderId, const IoFolderConfigPtr& parent) const;

    void setDeviceDomain(const DeviceDomainPtr& deviceDomain) const;

protected:

    virtual void handleConfig(const PropertyObjectPtr& config);
    virtual void handleOptions(const DictPtr<IString, IBaseObject>& options);
    virtual void initSignals(const FolderConfigPtr& signalsFolder);
    virtual void initDevices(const FolderConfigPtr& devicesFolder);
    virtual void initFunctionBlocks(const FolderConfigPtr& fbFolder);
    virtual void initIOFolder(const IoFolderConfigPtr& ioFolder);
    virtual void initCustomComponents();
    virtual DeviceDomainPtr initDeviceDomain();  // TODO: Pass builder as param when implemented

    virtual void start();

    //virtual void configureSyncComponent(); // TODO: Add once sync component is implemented

    virtual DeviceDomainPtr getDeviceDomain();
    virtual uint64_t getTicksSinceOrigin();

    virtual BaseObjectPtr onPropertyWrite(const StringPtr& propertyName, const PropertyPtr& property, const BaseObjectPtr& value);
    virtual BaseObjectPtr onPropertyRead(const StringPtr& propertyName, const PropertyPtr& property, const BaseObjectPtr& value);

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

    DeviceTemplateHooks(std::unique_ptr<DeviceTemplate> device, const DeviceParams& params, const StringPtr& className = "")
        : DeviceParamsValidation(params)
        , Device(params.context, params.parent, params.localId, className, params.info.getName())
        , device(std::move(device))
        , info(params.info)
        , initialized(false)
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
        registerCallbacks(objPtr);
        setDeviceDomain(this->device->initDeviceDomain());
        this->device->initIOFolder(ioFolder);
    }   

private:
    
    void onObjectReady() override;
    DeviceInfoPtr onGetInfo() override;
    uint64_t onGetTicksSinceOrigin() override;
    void registerCallbacks(const PropertyObjectPtr& obj);
    
    friend class DeviceTemplate;
    std::unique_ptr<DeviceTemplate> device;
    DeviceInfoPtr info;
    bool initialized;
};

END_NAMESPACE_OPENDAQ
