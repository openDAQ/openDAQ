#pragma once
#include <opendaq/opendaq.h>

BEGIN_NAMESPACE_OPENDAQ

struct DeviceTemplateParams
{
    DeviceInfoPtr info;
    ContextPtr context;
    ComponentPtr parent;
    PropertyObjectPtr config;

    std::string logName;
    std::string localId;
    std::string className;

    bool allowAddDevices = false;
    bool allowAddFunctionBlocks = false;
};

struct ModuleTemplateParams
{
    VersionInfoPtr version;
    ContextPtr context;
    std::string name;
    std::string id;
    std::string logName;
};

struct GetDeviceParams
{
    std::string typeId;
    std::string address;
    DeviceInfoPtr info;
    FolderPtr parent;
    PropertyObjectPtr config;
    DictPtr<IString, IBaseObject> options;
};

class DeviceTemplateParamsValidation
{
public:
    DeviceTemplateParamsValidation(const DeviceTemplateParams& params)
    {
        if (params.localId.empty())
            throw InvalidParametersException("Local id is not set");
        if (!params.info.assigned())
            throw InvalidParametersException("Device info is not set");
        if (params.logName.empty())
            throw InvalidParametersException("Log name is not set");
        if (!params.context.assigned())
            throw InvalidParametersException("Context is not set");
    }
};

class ModuleTemplateParamsValidation
{
public:
    ModuleTemplateParamsValidation(const ModuleTemplateParams& config)
    {
        if (!config.version.assigned())
            throw InvalidParametersException("Module version is not set");
        if (!config.context.assigned())
            throw InvalidParametersException("Context is not set");
        if (config.name.empty())
            throw InvalidParametersException("Module is not set");
        if (config.logName.empty())
            throw InvalidParametersException("Log name is not set");
        if (config.id.empty())
            throw InvalidParametersException("Module ID is not set");
    }
};

class FuntionBlockTemplateParamsValidation
{

};

END_NAMESPACE_OPENDAQ
