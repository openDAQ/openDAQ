#pragma once
#include <opendaq/device_impl.h>
#include <opendaq_module_template/component_template_base.h>
#include <opendaq_module_template/hooks_template_base.h>
#include <opendaq/io_folder_config_ptr.h>
#include <opendaq/log_file_info_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES

class DeviceTemplateHooks;

class DeviceTemplate : public ComponentTemplateBase<DeviceTemplateHooks>, public AddableComponentTemplateBase
{
public:
    template <class ChannelImpl, class ChannelTemplateImpl, class... Params>
    std::shared_ptr<ChannelTemplateImpl> createAndAddChannel(const ChannelParams& channelParams, Params&&... params) const;
    IoFolderConfigPtr createAndAddIOFolder(const std::string& folderId, const IoFolderConfigPtr& parent) const;

    void setDeviceDomain(const DeviceDomainPtr& deviceDomain) const;

protected:
    virtual void initIOFolder(const IoFolderConfigPtr& ioFolder);
    virtual void initDevices(const FolderConfigPtr& devicesFolder);
    virtual void initSyncComponent(const SyncComponentPrivatePtr& syncComponent);
    virtual void initCustomComponents();
    virtual DeviceDomainPtr initDeviceDomain();  // TODO: Pass builder as param when implemented
    
    virtual uint64_t getTicksSinceOrigin();
    virtual ListPtr<ILogFileInfo> getLogFileInfos();
    virtual StringPtr getLog(const StringPtr& id, Int size, Int offset);

    // Should this be handled differently?
    virtual bool allowAddDevicesFromModules();
    virtual bool allowAddFunctionBlocksFromModules();
    
private:
    friend class DeviceTemplateHooks;
};

template <class ChannelHooksImpl, class ChannelTemplateImpl, class ... Params>
std::shared_ptr<ChannelTemplateImpl> DeviceTemplate::createAndAddChannel(const ChannelParams& channelParams, Params&&... params) const
{
    LOG_T("Adding channel {}", channelId)
    ChannelPtr ch = createWithImplementation<IChannel, ChannelHooksImpl>(channelParams, std::forward<Params>(params)...);
    channelParams.parent.getRef().addItem(ch);
    
    auto implPtr = static_cast<ChannelHooksImpl*>(ch.getObject());
    return implPtr->template getChannelTemplate<ChannelTemplateImpl>();
}

class DeviceTemplateHooks : public TemplateHooksBase<DeviceTemplate>, public DeviceParamsValidation, public Device
{
public:

    DeviceTemplateHooks(const std::shared_ptr<DeviceTemplate>& device, const DeviceParams& params, const StringPtr& className = "")
        : TemplateHooksBase(device)
        , DeviceParamsValidation(params)
        , Device(params.context, params.parent.assigned() ? params.parent.getRef() : nullptr, params.localId, className, params.info.getName())
        , info(params.info)
    {
        if (!this->info.isFrozen())
            this->info.freeze();
            
        this->templateImpl->componentImpl = this;
        this->templateImpl->objPtr = this->thisPtr<PropertyObjectPtr>();
        this->templateImpl->loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
        this->templateImpl->context = this->context;

        auto lock = this->getRecursiveConfigLock();
        registerCallbacks(objPtr);
        
        this->templateImpl->initProperties();
        this->templateImpl->applyConfig(params.config);

        setDeviceDomain(this->templateImpl->initDeviceDomain());
        this->templateImpl->initIOFolder(ioFolder);
        this->templateImpl->initDevices(devices);
        this->templateImpl->initSignals(signals);
        this->templateImpl->initFunctionBlocks(functionBlocks);
        this->templateImpl->initCustomComponents();
        this->templateImpl->initSyncComponent(syncComponent);

        this->templateImpl->initTags(tags);
        this->templateImpl->initStatuses(statusContainer);

        this->templateImpl->start();
    }   

private:
    
    DeviceInfoPtr onGetInfo() override;
    uint64_t onGetTicksSinceOrigin() override;
    ListPtr<ILogFileInfo> onGetLogFileInfos() override;
    StringPtr onGetLog(const StringPtr& id, Int size, Int offset) override;

    bool allowAddDevicesFromModules() override;
    bool allowAddFunctionBlocksFromModules() override;
    void removed() override;
    
    friend class DeviceTemplate;
    friend class ComponentTemplateBase<DeviceTemplateHooks>;
    DeviceInfoPtr info;
};

END_NAMESPACE_OPENDAQ_TEMPLATES
