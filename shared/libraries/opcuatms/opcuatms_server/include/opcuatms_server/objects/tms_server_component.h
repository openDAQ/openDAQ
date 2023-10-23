/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/tags_ptr.h>
#include "opcuatms_server/objects/tms_server_property_object.h"
#include "opcuatms/converters/variant_converter.h"
#include "open62541/tmsdevice_nodeids.h"


BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

template <typename Ptr = ComponentPtr>
class TmsServerComponent;
using TmsServerComponentPtr = std::shared_ptr<TmsServerComponent<ComponentPtr>>;

template <typename Ptr>
class TmsServerComponent : public TmsServerObjectBaseImpl<Ptr>
{
public:
    using Super = TmsServerObjectBaseImpl<Ptr>;

    TmsServerComponent(const ComponentPtr& object, const opcua::OpcUaServerPtr& server, const ContextPtr& context);

    bool createOptionalNode(const opcua::OpcUaNodeId& nodeId) override;

    std::string getBrowseName() override;
    std::string getDisplayName() override;
    opcua::OpcUaNodeId getReferenceType() override;
    void bindCallbacks() override;
    void addChildNodes() override;

protected:
    opcua::OpcUaNodeId getTmsTypeId() override;

    std::unique_ptr<TmsServerPropertyObject> tmsPropertyObject;
};

using namespace opcua;

template <typename Ptr>
TmsServerComponent<Ptr>::TmsServerComponent(const ComponentPtr& object, const OpcUaServerPtr& server, const ContextPtr& context)
    : Super(object, server, context)
{
    tmsPropertyObject = std::make_unique<TmsServerPropertyObject>(this->object, this->server, this->daqContext);
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
OpcUaNodeId TmsServerComponent<Ptr>::getReferenceType()
{
    return OpcUaNodeId(0, UA_NS0ID_HASCOMPONENT);
}

template <typename Ptr>
OpcUaNodeId TmsServerComponent<Ptr>::getTmsTypeId()
{
    return OpcUaNodeId(NAMESPACE_TMSDEVICE, UA_TMSDEVICEID_DAQCOMPONENTTYPE);
}

template <typename Ptr>
bool TmsServerComponent<Ptr>::createOptionalNode(const OpcUaNodeId& nodeId)
{
    return false;
}

template <typename Ptr>
void TmsServerComponent<Ptr>::bindCallbacks()
{
    this->addReadCallback("Tags",[this]()
      {
          const TagsPtr tags = this->object.getTags();
          if (tags.assigned())
              return VariantConverter<IString>::ToArrayVariant(tags.getList());
          return VariantConverter<IString>::ToArrayVariant(List<IString>());
      });

    this->addReadCallback("Active", [this]() { return VariantConverter<IBoolean>::ToVariant( this->object.getActive()); });
    if (!this->object.template asPtrOrNull<IFreezable>().assigned() || !this->object.isFrozen())
    {
        this->addWriteCallback("Active", [this](const OpcUaVariant& variant){
            this->object.setActive(VariantConverter<IBoolean>::ToDaqObject(variant));
            return UA_STATUSCODE_GOOD;
        });
    }
}

template <typename Ptr>
void TmsServerComponent<Ptr>::addChildNodes()
{
    tmsPropertyObject->registerToExistingOpcUaNode(this->nodeId);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
