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

class ModuleTemplate : public ModuleTemplateParamsValidation, public Module
{
public:
    ModuleTemplate(const ModuleTemplateParams& params);

protected:
    virtual std::vector<DeviceInfoFields> getDeviceInfoFields(const std::string& typeId, const DictPtr<IString, IBaseObject>& options);
    virtual std::vector<DeviceTypePtr> getDeviceTypes();
    virtual DevicePtr getDevice(const GetDeviceParams& params);
    virtual void deviceRemoved(const std::string& deviceLocalId);

    std::mutex sync;
    LoggerComponentPtr loggerComponent;

private:
    ListPtr<IDeviceInfo> onGetAvailableDevices() override;
    DictPtr<IString, IDeviceType> onGetAvailableDeviceTypes() override;
    DevicePtr onCreateDevice(const StringPtr& connectionString, const ComponentPtr& parent, const PropertyObjectPtr& config) override;
    static DeviceInfoPtr createDeviceInfo(const DeviceInfoFields& fields, const DeviceTypePtr& type);

};

END_NAMESPACE_OPENDAQ
