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

// Constructor parameter definitions

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

struct CreateDeviceParams
{
    std::string typeId;
    std::string address;
    DeviceInfoPtr info;
    FolderPtr parent;
    PropertyObjectPtr config;
    DictPtr<IString, IBaseObject> options;
};

// Validation classes

class DeviceTemplateParamsValidation
{
public:
    DeviceTemplateParamsValidation(const DeviceTemplateParams& params)
    {
        if (params.localId.empty())
            throw InvalidParameterException("Local id is not set");
        if (!params.info.assigned())
            throw InvalidParameterException("Device info is not set");
        if (params.logName.empty())
            throw InvalidParameterException("Log name is not set");
        if (!params.context.assigned())
            throw InvalidParameterException("Context is not set");
    }
};

class ModuleTemplateParamsValidation
{
public:
    virtual ~ModuleTemplateParamsValidation() = default;

    ModuleTemplateParamsValidation(const ModuleTemplateParams& params)
    {
        if (!params.version.assigned())
            throw InvalidParameterException("Module version is not set");
        if (!params.context.assigned())
            throw InvalidParameterException("Context is not set");
        if (params.name.empty())
            throw InvalidParameterException("Module is not set");
        if (params.logName.empty())
            throw InvalidParameterException("Log name is not set");
        if (params.id.empty())
            throw InvalidParameterException("Module ID is not set");

        this->params = params; 
    }

    ModuleTemplateParams params;
};

class FunctionBlockTemplateParamsValidation
{

};

END_NAMESPACE_OPENDAQ
