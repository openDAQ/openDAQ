#include "opcuatms_server/objects/tms_server_property_object.h"
#include <opendaq/device_ptr.h>
#include <coretypes/function_ptr.h>
#include "coreobjects/argument_info_factory.h"
#include "coreobjects/property_object_internal_ptr.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms_server/objects/tms_server_property.h"
#include "open62541/nodeids.h"
#include "open62541/statuscodes.h"
#include "open62541/daqbsp_nodeids.h"
#include "open62541/daqbt_nodeids.h"
#include "open62541/types_generated.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsServerPropertyObject::TmsServerPropertyObject(const PropertyObjectPtr& object,
                                                 const OpcUaServerPtr& server,
                                                 const ContextPtr& context,
                                                 const TmsServerContextPtr& tmsContext,
                                                 const std::unordered_set<std::string>& ignoredProps)
    : Super(object, server, context, tmsContext)
      , ignoredProps(ignoredProps)
{
}

TmsServerPropertyObject::TmsServerPropertyObject(const PropertyObjectPtr& object,
                                                 const opcua::OpcUaServerPtr& server,
                                                 const ContextPtr& context,
                                                 const TmsServerContextPtr& tmsContext,
                                                 const StringPtr& name)
    : TmsServerPropertyObject(object, server, context, tmsContext)
{
    this->name = name;
}

TmsServerPropertyObject::TmsServerPropertyObject(const PropertyObjectPtr& object,
                                                 const opcua::OpcUaServerPtr& server,
                                                 const ContextPtr& context,
                                                 const TmsServerContextPtr& tmsContext,
                                                 const StringPtr& name,
                                                 const PropertyPtr& objProp)
    : TmsServerPropertyObject(object, server, context, tmsContext, name)
{
    this->objProp = objProp;
}

TmsServerPropertyObject::~TmsServerPropertyObject()
{
    for (auto prop : this->object.getAllProperties())
        this->object.getOnPropertyValueWrite(prop.getName()) -= event(this, &TmsServerPropertyObject::triggerEvent);
}

std::string TmsServerPropertyObject::getBrowseName()
{
    if (name.assigned())
        return name;

    const auto className = object.getClassName();
    return className != "" ? className : "PropertyObject";
}

OpcUaNodeId TmsServerPropertyObject::getTmsTypeId()
{
    return OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_VARIABLEBLOCKTYPE);
}

void TmsServerPropertyObject::addChildNodes()
{
    if (objProp.assigned())
    {
        if (objProp.getVisibleUnresolved().assigned())
            registerEvalValueNode("IsVisible", [this]() { return objProp.getVisibleUnresolved(); });
        if (objProp.getReadOnlyUnresolved().assigned())
            registerEvalValueNode("IsReadOnly", [this]() { return objProp.getReadOnlyUnresolved(); });
    }

    uint32_t propNumber = 0;
    std::unordered_map<std::string, uint32_t> propOrder;
    for (const auto& prop : object.getAllProperties())
    {
        if (ignoredProps.count(prop.getName()))
            continue;

        propOrder.insert(std::pair<std::string, uint32_t>(prop.getName(), propNumber));
        propNumber++;
    }

    for (const auto& prop : object.getAllProperties())
    {
        if (ignoredProps.count(prop.getName()))
            continue;

        // NOTE: ctObject types cannot be placed below ReferenceVariableType properties
        if (prop.getValueType() != ctObject || prop.getReferencedProperty().assigned())
        {
            if (prop.getIsReferenced())
                continue;
            if (prop.getValueType() == ctFunc || prop.getValueType() == ctProc)
            {
                addMethodPropertyNode(prop, propOrder[prop.getName()]);
                continue;
            }

            OpcUaNodeId childNodeId;
            std::shared_ptr<TmsServerProperty> serverInfo;
            const auto propName = prop.getName();
            if (hasChildNode(propName))
            {
                serverInfo = std::make_shared<TmsServerProperty>(prop, server, daqContext, tmsContext, object, propOrder);
                childNodeId = serverInfo->registerToExistingOpcUaNode(nodeId);
            }
            else
            {
                serverInfo = registerTmsObjectOrAddReference<TmsServerProperty>(nodeId, prop, std::numeric_limits<uint32_t>::max(), object, propOrder);
                childNodeId = serverInfo->getNodeId();
            }
            
            childProperties.insert({childNodeId, serverInfo});
        }
        else
        {
            const auto propName = prop.getName();
            PropertyObjectPtr obj = object.getPropertyValue(propName);
            auto serverInfo = registerTmsObjectOrAddReference<TmsServerPropertyObject>(nodeId, obj, propOrder[propName], propName, prop);
            auto childNodeId = serverInfo->getNodeId();
            childObjects.insert({childNodeId, serverInfo});
        }
    }

    addBeginUpdateNode();
    addEndUpdateNode();
}

void TmsServerPropertyObject::bindCallbacks()
{
    for (const auto& [id, prop] : childProperties)
    {
        this->object.getOnPropertyValueWrite(prop->getBrowseName()) += event(this, &TmsServerPropertyObject::triggerEvent);
        bindPropertyCallbacks(prop->getBrowseName());
    }

    bindMethodCallbacks();
}

bool TmsServerPropertyObject::createOptionalNode(const opcua::OpcUaNodeId& nodeId)
{
    const auto name = server->readBrowseNameString(nodeId);

    if (name == "<VariableBlock>" || name == "<BaseDataVariable>")
        return false;

    if (!objProp.assigned() && (name == "IsReadOnly" || name == "IsVisible")) 
        return false;

    return true;
}

void TmsServerPropertyObject::bindPropertyCallbacks(const std::string& name)
{
    if (!this->object.getProperty(name).asPtr<IPropertyInternal>().getReferencedPropertyUnresolved().assigned())
    {
        addReadCallback(name, [this, name]() {
            const auto value = this->object.getPropertyValue(name);
            return VariantConverter<IBaseObject>::ToVariant(value, nullptr, daqContext);
        });

        const auto freezable = this->object.asPtrOrNull<IFreezable>();
        if (!freezable.assigned() || !this->object.isFrozen())
        {
            addWriteCallback(name, [this, name](const OpcUaVariant& variant) {
                const auto value = VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext);
                this->object.setPropertyValue(name, value);
                return UA_STATUSCODE_GOOD;
            });
        }
    }
    else
    {
        addReadCallback(name, [this, name]() {
            const auto refProp = this->object.getProperty(name).asPtr<IPropertyInternal>().getReferencedPropertyUnresolved();
            return VariantConverter<IBaseObject>::ToVariant(refProp.getEval(), nullptr, daqContext);
        });
    }
}

void TmsServerPropertyObject::setMethodParentNodeId(const OpcUaNodeId& methodParentNodeId)
{
    this->methodParentNodeId = methodParentNodeId;
}

void TmsServerPropertyObject::registerEvalValueNode(const std::string& nodeName, TmsServerEvalValue::ReadCallback readCallback)
{
    auto nodeId = getChildNodeId(nodeName);
    auto serverObject = std::make_shared<TmsServerEvalValue>(server, daqContext, tmsContext);
    serverObject->setReadCallback(std::move(readCallback));
    auto childNodeId = serverObject->registerToExistingOpcUaNode(nodeId);

    childEvalValues.insert({childNodeId, serverObject});
}

void TmsServerPropertyObject::addMethodPropertyNode(const PropertyPtr& prop, uint32_t numberInList)
{
    const auto name = prop.getName();
    OpcUaNodeId parentId = methodParentNodeId.isNull() ? nodeId : methodParentNodeId;
    OpcUaNodeId newNodeId(0);
    AddMethodNodeParams params(newNodeId, parentId);
    params.referenceTypeId = OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT));

    const auto callableInfo = prop.getCallableInfo();
    const auto returnType = callableInfo.getReturnType();

    try
    {
        if (returnType != ctUndefined)
        {
            auto outputArg = VariantConverter<IArgumentInfo>::ToVariant(ArgumentInfo("", returnType));
            CheckStatusCodeException(UA_Array_copy(outputArg->data, 1, (void**) &params.outputArguments, GetUaDataType<UA_Argument>()));
            params.outputArgumentsSize = 1;
        }

        const auto args = callableInfo.getArguments();
        if (args.assigned() && args.getCount() > 0)
        {
            auto inputArg = VariantConverter<IArgumentInfo>::ToArrayVariant(args);
            CheckStatusCodeException(UA_Array_copy(inputArg->data, args.getCount(), (void**) &params.inputArguments, GetUaDataType<UA_Argument>()));
            params.inputArgumentsSize = args.getCount();
        }

        params.setBrowseName(name);
        auto methodNodeId = server->addMethodNode(params);

        OpcUaNodeId numberInListRequestedNodeId(0);
        AddVariableNodeParams numberInListParams(numberInListRequestedNodeId, methodNodeId);
        numberInListParams.setBrowseName("NumberInList");
        numberInListParams.setDataType(OpcUaNodeId(UA_TYPES[UA_TYPES_UINT32].typeId));
        
        numberInListParams.typeDefinition = OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));
        const auto numberInListNodeId = server->addVariableNode(numberInListParams);
        server->writeValue(numberInListNodeId, OpcUaVariant(numberInList));

        methodProps.insert({methodNodeId, {name, prop.getValueType()}});
    }
    catch(...)
    {
        // TODO: Log failure to add method node
    }
}

void TmsServerPropertyObject::bindMethodCallbacks()
{
    for (const auto& [nodeId, props] : methodProps)
    {
        auto methodName = props.first;
        auto coreType = props.second;

        // TODO: Return more specific errors
        addEvent(nodeId)->onMethodCall(
        [this, methodName, coreType](NodeEventManager::MethodArgs args) -> UA_StatusCode
        {
            try
            {
                const BaseObjectPtr method = this->object.getPropertyValue(methodName);
                if (args.inputSize > 0)
                {
                    BaseObjectPtr daqArg;
                    if (args.inputSize > 1)
                    {
                        daqArg = List<IBaseObject>();
                        for (size_t i = 0; i < args.inputSize; ++i)
                        {
                            auto variant = daq::opcua::OpcUaVariant(args.input[i]);
                            daqArg.asPtr<IList>().pushBack(VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext));
                        }
                    }
                    else
                    {
                        auto variant = daq::opcua::OpcUaVariant(args.input[0]);
                        daqArg = VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext);
                    }
                    
                    if (coreType == ctFunc && args.outputSize > 0)
                        args.output[0] = VariantConverter<IBaseObject>::ToVariant(method.asPtr<IFunction>()(daqArg), nullptr, daqContext)
                                             .getDetachedValue();
                    else
                        method.asPtr<IProcedure>()(daqArg);

                    return UA_STATUSCODE_GOOD;
                }

                if (coreType == ctFunc && args.outputSize > 0)
                    args.output[0] =
                        VariantConverter<IBaseObject>::ToVariant(method.asPtr<IFunction>()(), nullptr, daqContext).getDetachedValue();
                else
                    method.asPtr<IProcedure>()();

                
                return UA_STATUSCODE_GOOD;
            }
            catch(...)
            {
                return UA_STATUSCODE_BAD;
            }
        });
    }
}

void TmsServerPropertyObject::addBeginUpdateNode()
{
    OpcUaNodeId nodeIdOut;
    AddMethodNodeParams params(nodeIdOut, nodeId);
    params.referenceTypeId = OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT));
    params.setBrowseName("BeginUpdate");
    auto methodNodeId = server->addMethodNode(params);

    auto callback = [this](NodeEventManager::MethodArgs args)
    {
        const auto status = this->object->beginUpdate();
        return status == OPENDAQ_SUCCESS ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
    };

    addEvent(methodNodeId)->onMethodCall(callback);
}

void TmsServerPropertyObject::addEndUpdateNode()
{
    OpcUaNodeId nodeIdOut;
    AddMethodNodeParams params(nodeIdOut, nodeId);
    params.referenceTypeId = OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT));
    params.setBrowseName("EndUpdate");
    auto methodNodeId = server->addMethodNode(params);

    auto callback = [this](NodeEventManager::MethodArgs args)
    {
        const auto status = this->object->endUpdate();
        return status == OPENDAQ_SUCCESS ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
    };

    addEvent(methodNodeId)->onMethodCall(callback);
}

void TmsServerPropertyObject::addPropertyNode(const std::string& name, const opcua::OpcUaNodeId& parentId)
{
    auto params = AddVariableNodeParams(UA_NODEID_NULL, parentId);
    params.setBrowseName(name);
    params.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    params.attr->accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    server->addVariableNode(params);
}

void TmsServerPropertyObject::configureNodeAttributes(opcua::OpcUaObject<UA_ObjectAttributes>& attr)
{
    Super::configureNodeAttributes(attr);

    attr->writeMask |= UA_WRITEMASK_DISPLAYNAME;
}

void TmsServerPropertyObject::triggerEvent(PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args)
{
    if(!this->server->getUaServer())
        return;

    EventAttributes attributes;
    attributes.setTime(UA_DateTime_now());
    attributes.setMessage("Property value changed");
    this->server->triggerEvent(OpcUaNodeId(UA_NS0ID_BASEEVENTTYPE), nodeId, attributes);
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
