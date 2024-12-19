#pragma once
#include <opendaq/opendaq.h>
#include <set>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES
// Attribute definitions

struct AttributeTemplate
{
    AttributeTemplate(): readOnly(false){}
    AttributeTemplate(bool readOnly): readOnly(readOnly){}

    bool readOnly;
};

struct BoolAttribute : AttributeTemplate
{
    BoolAttribute(bool value) : value(value){}
    BoolAttribute(bool value, bool _readOnly) : AttributeTemplate(_readOnly){ this->value = value; }
    bool value;
};

struct IntAttribute : AttributeTemplate
{
    IntAttribute(int value) : value(value){}
    IntAttribute(int value, bool _readOnly) : AttributeTemplate(_readOnly){ this->value = value; }
    int value;
};

struct StringAttribute : AttributeTemplate
{
    StringAttribute(const std::string& value) : value(value){}
    StringAttribute(const std::string& value, bool _readOnly) : AttributeTemplate(_readOnly){ this->value = value; }
    std::string value;
};

struct SignalAttribute : AttributeTemplate
{
    SignalAttribute(const SignalPtr& value) : value(value){}
    SignalAttribute(const SignalPtr& value, bool _readOnly) : AttributeTemplate(_readOnly){ this->value = value; }
    SignalPtr value;
};

struct SignalListAttribute : AttributeTemplate
{
    SignalListAttribute(const ListPtr<ISignal>& value) : value(value){}
    SignalListAttribute(const ListPtr<ISignal>& value, bool _readOnly) : AttributeTemplate(_readOnly){ this->value = value; }
    ListPtr<ISignal> value;
};

struct ComponentAttributeConfig
{
    StringAttribute description{"", true};
    StringAttribute name {"", false};
    BoolAttribute active {true, false};
    BoolAttribute visible {true, true};
};

struct SignalAttributeConfig : ComponentAttributeConfig
{
    BoolAttribute isPublic {true, true};
    SignalAttribute domainSignal {nullptr, true};
    SignalListAttribute relatedSignals{{}, true};
};

// Device information

struct DeviceInfoParams
{
    DeviceInfoParams(){}

    StringAttribute address{""};
    StringAttribute typeId{""};
    StringAttribute name{""};
    StringAttribute manufacturer{""};
    StringAttribute manufacturerUri{""};
    StringAttribute model{""};
    StringAttribute productCode{""};
    StringAttribute deviceRevision{""};
    StringAttribute hardwareRevision{""};
    StringAttribute softwareRevision{""};
    StringAttribute deviceManual{""};
    StringAttribute deviceClass{""};
    StringAttribute serialNumber{""};
    StringAttribute productInstanceUri{""};
    IntAttribute revisionCounter{0};
    StringAttribute assetId{""};
    StringAttribute macAddress{""};
    StringAttribute parentMacAddress{""};
    StringAttribute platform{""};
    IntAttribute position{0};
    StringAttribute systemType{""};
    StringAttribute systemUuid{""};
    StringAttribute location{""};
    std::map<std::string, StringAttribute> other;
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

struct SignalParams
{
    std::string localId;
    std::string className;

    DataDescriptorPtr descriptor;
    SignalAttributeConfig attributes;
};

struct ChannelParams
{
    std::string logName;
    std::string localId;
    std::string className = "";

    FunctionBlockTypePtr type;
    ContextPtr context;
    WeakRefPtr<IIoFolderConfig> parent;
};

struct FunctionBlockParams
{
    std::string logName;
    std::string localId;
    std::string className = "";

    FunctionBlockTypePtr type;
    ContextPtr context;
    WeakRefPtr<IComponent> parent;

    PropertyObjectPtr config;
};

struct DeviceParams
{
    std::string logName;
    std::string localId;

    DeviceInfoPtr info;
    ContextPtr context;
    WeakRefPtr<IComponent> parent;

    PropertyObjectPtr config;

    std::string typeId;
    std::string address;
};

struct ModuleParams
{
    VersionInfoPtr version;
    std::string name;
    std::string id;
    std::string logName;
    DictPtr<IString, IBaseObject> defaultOptions;
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
        if (!params.parent.assigned())
            throw InvalidParameterException("Parent is not set");

        this->params = params; 
    }

    FunctionBlockParams params;
};

class ChannelParamsValidation   
{
public:
    ChannelParamsValidation(const ChannelParams& params)
    {
        if (params.localId.empty())
            throw InvalidParameterException("Local id is not set");
        if (!params.type.assigned())
            throw InvalidParameterException("Function block type is not set");
        if (params.logName.empty())
            throw InvalidParameterException("Log name is not set");
        if (!params.context.assigned())
            throw InvalidParameterException("Context is not set");
        if (!params.parent.assigned() || !params.parent.getRef().assigned())
            throw InvalidParameterException("Parent is not set");
        if (!params.parent.getRef().asPtrOrNull<IIoFolderConfig>().assigned())
            throw InvalidParameterException("Channel parent folder is not an IO folder");
        
        this->params = params; 
    }

    ChannelParams params;
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

// Internal Helpers


struct PropertyEventArgs
{
    PropertyObjectPtr owner;
    PropertyPtr property;
    StringPtr propertyName;
    BaseObjectPtr value;
    Bool isUpdating;
};

struct UpdateEndArgs
{
    PropertyObjectPtr owner;
    std::set<StringPtr> changedProperties;
    Bool isParentUpdating;
};

struct AcquisitionLoopParams
{
    bool enableLoop = false;
    std::chrono::milliseconds loopTime = std::chrono::milliseconds(100);
};


END_NAMESPACE_OPENDAQ_TEMPLATES
