#pragma once
#include <opendaq/module_impl.h>
#include <opendaq/device_info_config_ptr.h>
#include <map>

BEGIN_NAMESPACE_OPENDAQ

struct DeviceInfoFields
{
    std::string connectionAddress;
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

class ModuleTemplate : public Module
{
public:
    ModuleTemplate(const StringPtr& name,
                   const VersionInfoPtr& version,
                   const ContextPtr& context,
                   const StringPtr& id);

protected:
    virtual std::vector<DeviceInfoFields> getDeviceInfoFields(const std::string& typeId, const DictPtr<IString, IBaseObject>& options);
    virtual std::vector<DeviceTypePtr> getDeviceTypes();
    virtual DevicePtr getDevice(const std::string& typeId,
                                const std::string& connectionAddress,
                                const DeviceInfoPtr& info,
                                const FolderPtr& parent,
                                const PropertyObjectPtr& config,
                                const DictPtr<IString, IBaseObject>& options);
    virtual void deviceRemoved(const std::string& deviceLocalId);

    std::mutex sync;

private:
    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config) override;
    static DeviceInfoPtr createDeviceInfo(const DeviceInfoFields& fields, const DeviceTypePtr& type);

};

END_NAMESPACE_OPENDAQ
