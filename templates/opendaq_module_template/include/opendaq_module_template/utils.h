/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <set>
#include <opendaq/opendaq.h>

/*
 * Utils TODO:
 *  - Implement circular buffer class for packet with pre-allocated memory creation
 */

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
    DeviceInfoParams() = default;

    StringAttribute address{"", true};
    StringAttribute typeId{"", true};
    StringAttribute name{"", false};
    StringAttribute manufacturer{"", true};
    StringAttribute manufacturerUri{"", true};
    StringAttribute model{"", true};
    StringAttribute productCode{"", true};
    StringAttribute deviceRevision{"", true};
    StringAttribute hardwareRevision{"", true};
    StringAttribute softwareRevision{"", true};
    StringAttribute deviceManual{"", true};
    StringAttribute deviceClass{"", true};
    StringAttribute serialNumber{"", true};
    StringAttribute productInstanceUri{"", true};
    IntAttribute revisionCounter{0, true};
    StringAttribute assetId{"", true};
    StringAttribute macAddress{"", true};
    StringAttribute parentMacAddress{"", true};
    StringAttribute platform{"", false};
    IntAttribute position{0, false};
    StringAttribute systemType{"", true};
    StringAttribute systemUuid{"", true};
    StringAttribute location{"", false};
    std::map<std::string, StringAttribute> other;

    std::map<std::string, AttributeTemplate*> attributes{
        {"address", &address},
        {"typeId", &typeId},
        {"name", &name},
        {"manufacturer", &manufacturer},
        {"manufacturerUri", &manufacturerUri},
        {"model", &model},
        {"productCode", &productCode},
        {"deviceRevision", &deviceRevision},
        {"hardwareRevision", &hardwareRevision},
        {"softwareRevision", &softwareRevision},
        {"deviceManual", &deviceManual},
        {"deviceClass", &deviceClass},
        {"serialNumber", &serialNumber},
        {"productInstanceUri", &productInstanceUri},
        {"revisionCounter", &revisionCounter},
        {"assetId", &assetId},
        {"macAddress", &macAddress},
        {"parentMacAddress", &parentMacAddress},
        {"platform", &platform},
        {"position", &position},
        {"systemType", &systemType},
        {"systemUuid", &systemUuid},
        {"location", &location}
    };
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
    std::string className;

    FunctionBlockTypePtr type;
    ContextPtr context;
    WeakRefPtr<IIoFolderConfig> parent;
};

struct FunctionBlockParams
{
    std::string logName;
    std::string localId;
    std::string className;

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
    std::chrono::milliseconds loopTime = std::chrono::milliseconds(50);
};


END_NAMESPACE_OPENDAQ_TEMPLATES
