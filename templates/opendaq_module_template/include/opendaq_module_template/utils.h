#pragma once
#include <opendaq/opendaq.h>

BEGIN_NAMESPACE_OPENDAQ

// Attribute definitions

struct AttributeTemplate
{
    bool locked;
};

struct BoolAttribute : AttributeTemplate
{
    bool value;
};

struct StringAttribute : AttributeTemplate
{
    std::string value;
};

struct SignalAttribute : AttributeTemplate
{
    SignalPtr value;
};

struct SignalListAttribute : AttributeTemplate
{
    ListPtr<ISignal> value;
};

struct ComponentAttributeConfig
{
    StringAttribute description {true, ""};
    StringAttribute name {false, ""};
    BoolAttribute active {false, true};
    BoolAttribute visible {true, true};
};

struct SignalAttributeConfig : ComponentAttributeConfig
{
    BoolAttribute isPublic {true, true};
    SignalAttribute domainSignal {true, nullptr};
    SignalListAttribute relatedSignals{true, {}};
};

// Device information

struct DeviceInfoParams
{
    std::string address;
    std::string typeId;
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

struct DeviceTypeParams
{
    std::string id;
    std::string name;
    std::string description;
    std::string connectionStringPrefix;
    PropertyObjectPtr defaultConfiguration;
};

// Constructor parameter definitions

struct FunctionBlockParams
{
    FunctionBlockTypePtr type;
    ContextPtr context;
    ComponentPtr parent;

    PropertyObjectPtr config;
    DictPtr<IString, IBaseObject> options;

    std::string logName;
    std::string localId;
};

struct DeviceParams
{
    DeviceInfoPtr info;
    ContextPtr context;
    ComponentPtr parent;

    PropertyObjectPtr config;
    DictPtr<IString, IBaseObject> options;

    std::string typeId;
    std::string address;
    
    std::string logName;
    std::string localId;
};

struct ModuleParams
{
    VersionInfoPtr version;
    std::string name;
    std::string id;
    std::string logName;
};


// Validation classes

class FunctionBlockParamsValidation
{
public:
    FunctionBlockParamsValidation(const FunctionBlockParams& params)
    {
        if (params.localId.empty())
            throw InvalidParameterException("Local id is not set");
        if (!params.type.assigned())
            throw InvalidParameterException("Function block type is not set");
        if (params.logName.empty())
            throw InvalidParameterException("Log name is not set");
        if (!params.context.assigned())
            throw InvalidParameterException("Context is not set");

        this->params = params; 
    }

    FunctionBlockParams params;
};

class DeviceParamsValidation
{
public:
    DeviceParamsValidation(const DeviceParams& params)
    {
        if (params.localId.empty())
            throw InvalidParameterException("Local id is not set");
        if (!params.info.assigned())
            throw InvalidParameterException("Device info is not set");
        if (params.logName.empty())
            throw InvalidParameterException("Log name is not set");
        if (!params.context.assigned())
            throw InvalidParameterException("Context is not set");

        this->params = params; 
    }

    DeviceParams params;
};

class ModuleParamsValidation
{
public:
    ModuleParamsValidation(const ModuleParams& params)
    {
        if (!params.version.assigned())
            throw InvalidParameterException("Module version is not set");
        if (params.name.empty())
            throw InvalidParameterException("Module is not set");
        if (params.logName.empty())
            throw InvalidParameterException("Log name is not set");
        if (params.id.empty())
            throw InvalidParameterException("Module ID is not set");

        this->params = params; 
    }

    ModuleParams params;
};


END_NAMESPACE_OPENDAQ
