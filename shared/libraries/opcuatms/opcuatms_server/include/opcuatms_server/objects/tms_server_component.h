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
#include <opendaq/component_ptr.h>
#include <opendaq/component_private_ptr.h>
#include <opendaq/tags_ptr.h>
#include <opendaq/custom_log.h>
#include <coreobjects/core_event_args_ids.h>
#include <opcuatms_server/objects/tms_server_property_object.h>
#include <opcuatms/converters/variant_converter.h>
#include <opcuatms_server/tms_server_context.h>
#include <open62541/daqdevice_nodeids.h>


BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

template <typename Ptr = ComponentPtr>
class TmsServerComponent;
using TmsServerComponentPtr = std::shared_ptr<TmsServerComponent<ComponentPtr>>;

template <typename Ptr>
class TmsServerComponent : public TmsServerObjectBaseImpl<Ptr>
{
public:
    using Super = TmsServerObjectBaseImpl<Ptr>;

    TmsServerComponent(const ComponentPtr& object, const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext);

    bool createOptionalNode(const opcua::OpcUaNodeId& nodeId) override;

    std::string getBrowseName() override;
    std::string getDisplayName() override;
    std::string getDescription() override;
    opcua::OpcUaNodeId getReferenceType() override;
    void bindCallbacks() override;
    void registerToTmsServerContext() override;
    void addChildNodes() override;
    void onCoreEvent(const CoreEventArgsPtr& args) override;

protected:
    opcua::OpcUaNodeId getTmsTypeId() override;
    void configureNodeAttributes(opcua::OpcUaObject<UA_ObjectAttributes>& attr) override;

    std::unique_ptr<TmsServerPropertyObject> tmsPropertyObject;
    std::unique_ptr<TmsServerPropertyObject> tmsComponentConfig;

private:
    bool selfChange;
};

using namespace opcua;

template <typename Ptr>
TmsServerComponent<Ptr>::TmsServerComponent(const ComponentPtr& object, const OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext)
    : Super(object, server, context, tmsContext)
    , selfChange(false)
{
    tmsPropertyObject = std::make_unique<TmsServerPropertyObject>(this->object, this->server, this->daqContext, this->tmsContext, std::unordered_set<std::string>{"Name", "Description"});
    if (auto componentPrivate = this->object.template asPtrOrNull<IComponentPrivate>(true); componentPrivate.assigned())
    {
        auto componentConfig = componentPrivate.getComponentConfig();
        if (componentConfig.assigned())
            tmsComponentConfig = std::make_unique<TmsServerPropertyObject>(componentConfig, this->server, this->daqContext, this->tmsContext, "ComponentConfig");
    }
    
    this->loggerComponent = this->daqContext.getLogger().getOrAddComponent("OPCUAServerComponent");
}

template <typename Ptr>
std::string TmsServerComponent<Ptr>::getBrowseName()
{
    return this->object.getLocalId();
}

template <typename Ptr>
std::string TmsServerComponent<Ptr>::getDisplayName()
{
    return this->object.getName();
}

template <typename Ptr>
std::string TmsServerComponent<Ptr>::getDescription()
{
    return this->object.getDescription();
}

template <typename Ptr>
OpcUaNodeId TmsServerComponent<Ptr>::getReferenceType()
{
    return OpcUaNodeId(0, UA_NS0ID_HASCOMPONENT);
}

template <typename Ptr>
OpcUaNodeId TmsServerComponent<Ptr>::getTmsTypeId()
{
    return OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_DAQCOMPONENTTYPE);
}

template <typename Ptr>
void TmsServerComponent<Ptr>::configureNodeAttributes(opcua::OpcUaObject<UA_ObjectAttributes>& attr)
{
    TmsServerObject::configureNodeAttributes(attr);
    std::unordered_set<std::string> lockedAttrs;
    for (const auto& str : this->object.getLockedAttributes())
        lockedAttrs.insert(str);

    if (!lockedAttrs.count("Name"))
        attr->writeMask |= UA_WRITEMASK_DISPLAYNAME;
    if (!lockedAttrs.count("Description"))
        attr->writeMask |= UA_WRITEMASK_DESCRIPTION;
}

template <typename Ptr>
bool TmsServerComponent<Ptr>::createOptionalNode(const OpcUaNodeId& nodeId)
{
    return false;
}

template <typename Ptr>
void TmsServerComponent<Ptr>::bindCallbacks()
{
    this->addReadCallback("Tags", [this]
    {
        const TagsPtr tags = this->object.getTags();
        if (tags.assigned())
            return VariantConverter<IString>::ToArrayVariant(tags.getList());
        return VariantConverter<IString>::ToArrayVariant(List<IString>());
    });

    this->addReadCallback("Active", [this] { return VariantConverter<IBoolean>::ToVariant( this->object.getActive()); });
    if (!this->object.template supportsInterface<IFreezable>() || !this->object.isFrozen())
    {
        this->addWriteCallback("Active", [this] (const OpcUaVariant& variant)
        {
            this->object.setActive(VariantConverter<IBoolean>::ToDaqObject(variant));
            return UA_STATUSCODE_GOOD;
        });
    }

	this->addReadCallback("Visible", [this] { return VariantConverter<IBoolean>::ToVariant( this->object.getVisible()); });

    if (!this->object.template supportsInterface<IFreezable>() || !this->object.isFrozen())
    {
        this->addWriteCallback("Visible", [this] (const OpcUaVariant& variant) 
        {
            this->object.setVisible(VariantConverter<IBoolean>::ToDaqObject(variant));
            return UA_STATUSCODE_GOOD;
        });
    }

    DisplayNameChangedCallback nameChangedCallback =
        [this](const OpcUaNodeId& /*nodeId*/, const OpcUaObject<UA_LocalizedText>& name, void* /*context*/)
        {
            if (selfChange)
                return;

            try
            {
                selfChange = true;
                this->object.setName(utils::ToStdString(name->text));
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                const auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpenDAQOPCUAServerModule");
                LOG_D("OPC UA Component {} failed to set component name: {}", this->object.getGlobalId(), e.what());
            }
            catch (...)
            {
                const auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpenDAQOPCUAServerModule");
                LOG_D("OPC UA Component {} failed to set component name.", this->object.getGlobalId());
            }

            selfChange = false;
        };
    this->server->getEventManager()->onDisplayNameChanged(this->nodeId, nameChangedCallback);

    DisplayNameChangedCallback descriptionChangedCallback =
        [this](const OpcUaNodeId& /*nodeId*/, const OpcUaObject<UA_LocalizedText>& description, void* /*context*/)
        {
            if (selfChange)
                return;

            try
            {
                selfChange = true;
                this->object.setDescription(utils::ToStdString(description->text));
            }
            catch ([[maybe_unused]] const std::exception& e)
            {
                const auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpenDAQOPCUAServerModule");
                LOG_D("OPC UA Component {} failed to set component description: {}", this->object.getGlobalId(), e.what());
            }
            catch (...)
            {
                const auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpenDAQOPCUAServerModule");
                LOG_D("OPC UA Component {} failed to set component description.", this->object.getGlobalId());
            }

            selfChange = false;
        };
    this->server->getEventManager()->onDescriptionChanged(this->nodeId, descriptionChangedCallback);
}

template <typename Ptr>
void TmsServerComponent<Ptr>::registerToTmsServerContext()
{
    Super::registerToTmsServerContext();
    this->tmsContext->registerComponent(this->object, *this);
}

template <typename Ptr>
void TmsServerComponent<Ptr>::addChildNodes()
{
	OpcUaNodeId newNodeId(0);
	AddVariableNodeParams params(newNodeId, this->nodeId);
	params.setBrowseName("Visible");
	params.setDataType(OpcUaNodeId(UA_TYPES[UA_TYPES_BOOLEAN].typeId));
    params.typeDefinition = OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    OpcUaObject<UA_VariableAttributes> attr = UA_VariableAttributes_default;
    attr->accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    params.attr = attr;
    
    this->server->addVariableNode(params);

    tmsPropertyObject->registerToExistingOpcUaNode(this->nodeId);
    if (tmsComponentConfig)
        tmsComponentConfig->registerOpcUaNode(this->nodeId);
}

template <typename Ptr>
void TmsServerComponent<Ptr>::onCoreEvent(const CoreEventArgsPtr& args)
{
    Super::onCoreEvent(args);

    if (!selfChange && args.getEventId() == static_cast<int>(CoreEventId::AttributeChanged))
    {
        const StringPtr attrName = args.getParameters().get("AttributeName");

        try
        {
            selfChange = true;
            if (attrName == "Name")
                this->server->setDisplayName(this->nodeId, args.getParameters().get("Name"));
            else if (attrName == "Description")
                this->server->setDescription(this->nodeId, args.getParameters().get("Description"));
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            const auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpenDAQOPCUAServerModule");
            LOG_D("OPC UA Component {} failed to set node attribute \"{}\": {}", this->object.getGlobalId(), attrName, e.what());
        }
        catch (...)
        {
            const auto loggerComponent = this->daqContext.getLogger().getOrAddComponent("OpenDAQOPCUAServerModule");
            LOG_D("OPC UA Component {} failed to set node attribute \"{}\".", this->object.getGlobalId(), attrName);
        }

        selfChange = false;
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
