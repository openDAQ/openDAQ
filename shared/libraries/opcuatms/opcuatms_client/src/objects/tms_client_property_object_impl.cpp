#include <opcuatms_client/objects/tms_client_property_object_impl.h>
#include <coreobjects/callable_info_factory.h>
#include <coreobjects/eval_value_factory.h>
#include <coreobjects/property_object_factory.h>
#include <opcuaclient/browser/opcuabrowser.h>
#include <opcuatms_client/objects/tms_client_property_factory.h>
#include <open62541/daqbt_nodeids.h>
#include <opcuatms/converters/variant_converter.h>
#include <opcuatms_client/objects/tms_client_property_object_factory.h>
#include <opcuatms_client/objects/tms_client_function_factory.h>
#include <opcuatms_client/objects/tms_client_procedure_factory.h>
#include <opendaq/mirrored_signal_impl.h>
#include <opendaq/input_port_impl.h>
#include <opendaq/channel_impl.h>
#include <opendaq/mirrored_device_impl.h>
#include <opendaq/io_folder_impl.h>
#include <opendaq/sync_component_impl.h>
#include <opcuatms/errors.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

namespace detail
{
    std::unordered_set<std::string> ignoredPropertyNames{"ServerCapabilities", "OperationMode", "OperationModeOptions"};
}

template <class Impl>
ErrCode TmsClientPropertyObjectBaseImpl<Impl>::setOPCUAPropertyValueInternal(IString* propertyName, IBaseObject* value, bool protectedWrite)
{
    if (propertyName == nullptr)
    {
        LOG_W("Failed to set value for property with nullptr name on OpcUA client property object");
        return OPENDAQ_SUCCESS;
    }
    auto propertyNamePtr = StringPtr::Borrow(propertyName);

    if (this->isChildProperty(propertyNamePtr))
    {
        PropertyPtr prop;
        ErrCode err = getProperty(propertyNamePtr, &prop);
        if (OPENDAQ_FAILED(err))
            return err;

        if (!prop.assigned())
            throw NotFoundException(R"(Child property "{}" not found)", propertyNamePtr);
        if (protectedWrite)
            return prop.asPtr<IPropertyInternal>()->setValueProtected(value);
        return prop->setValue(value);
    }

    StringPtr lastProcessDescription = "";
    ErrCode errCode = daqTry(
        [&]()
        {
            if (const auto& it = introspectionVariableIdMap.find(propertyNamePtr); it != introspectionVariableIdMap.cend())
            {
                PropertyPtr prop;
                checkErrorInfo(getProperty(propertyName, &prop));
                if (!protectedWrite)
                {
                    lastProcessDescription = "Checking existing property is read-only";
                    if (prop.getReadOnly())
                        return OPENDAQ_ERR_ACCESSDENIED;
                }

                BaseObjectPtr valuePtr = value;
                const auto ct = prop.getValueType();
                const auto valueCt = valuePtr.getCoreType();
                if (ct != valueCt)
                    valuePtr = valuePtr.convertTo(ct);

                lastProcessDescription = "Writing property value";
                const auto variant = VariantConverter<IBaseObject>::ToVariant(valuePtr, nullptr, daqContext);
                client->writeValue(it->second, variant);
                return OPENDAQ_SUCCESS;
            }

            if (const auto& it = referenceVariableIdMap.find(propertyNamePtr); it != referenceVariableIdMap.cend())
            {
                lastProcessDescription = "Setting property value";
                const auto refProp = this->objPtr.getProperty(propertyName).getReferencedProperty();
                return setPropertyValue(refProp.getName(), value);
            }

            if (const auto& it = objectTypeIdMap.find((propertyNamePtr)); it != objectTypeIdMap.cend())
            {
                lastProcessDescription = "Object type properties cannot be set over OpcUA";
                return OPENDAQ_ERR_NOTIMPLEMENTED;
            }

            lastProcessDescription = "Property not found";
            return OPENDAQ_ERR_NOTFOUND;
        });

    if (OPENDAQ_FAILED(errCode))
        LOG_W("Failed to set value for property \"{}\" on OpcUA client property object: {}", propertyNamePtr, lastProcessDescription);
    
    if (errCode == OPENDAQ_ERR_NOTFOUND || errCode == OPENDAQ_ERR_ACCESSDENIED)
        return errCode;

    return OPENDAQ_SUCCESS;
}

template <class Impl>
void TmsClientPropertyObjectBaseImpl<Impl>::init()
{
    if (!this->daqContext.getLogger().assigned())
        DAQ_THROW_EXCEPTION(ArgumentNullException, "Logger must not be null");

    this->loggerComponent = this->daqContext.getLogger().getOrAddComponent("TmsClientPropertyObject");
    clientContext->readObjectAttributes(nodeId);
    browseRawProperties();
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    return setOPCUAPropertyValueInternal(propertyName, value, false);
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    return setOPCUAPropertyValueInternal(propertyName, value, true);
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::getPropertyValue(IString* propertyName, IBaseObject** value)
{
    if (propertyName == nullptr)
    {
        LOG_W("Failed to get value for property with nullptr name on OpcUA client property object");
        return OPENDAQ_SUCCESS;
    }
    auto propertyNamePtr = StringPtr::Borrow(propertyName);

    if (this->isChildProperty(propertyNamePtr))
    {
        PropertyPtr prop;
        ErrCode err = getProperty(propertyNamePtr, &prop);
        if (OPENDAQ_FAILED(err))
            return err;

        if (!prop.assigned())
            throw NotFoundException(R"(Child property "{}" not found)", propertyNamePtr);

        return prop->getValue(value);
    }

    StringPtr lastProccessDescription = "";
    ErrCode errCode = daqTry([&]
    {
        if (const auto& introIt = introspectionVariableIdMap.find(propertyNamePtr); introIt != introspectionVariableIdMap.cend())
        {
            const auto variant = client->readValue(introIt->second);
            const auto object = VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext);
            Impl::setProtectedPropertyValue(propertyName, object);
        }
        else if (referenceVariableIdMap.count(propertyNamePtr))
        {
            const auto refProp = this->objPtr.getProperty(propertyName).getReferencedProperty();
            return getPropertyValue(refProp.getName(), value);
        }

        return Impl::getPropertyValue(propertyName, value);
    });
    if (OPENDAQ_FAILED(errCode))
    {
        LOG_W("Failed to get value for property \"{}\" on OpcUA client property object", propertyNamePtr);
    }
    return OPENDAQ_SUCCESS;
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::getPropertySelectionValue(IString* propertyName, IBaseObject** value)
{
    BaseObjectPtr object;
    TmsClientPropertyObjectBaseImpl::getPropertyValue(propertyName, &object);
    return Impl::getPropertySelectionValue(propertyName, value);
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::clearPropertyValue(IString* propertyName)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode TmsClientPropertyObjectBaseImpl<Impl>::clearProtectedPropertyValue(IString* propertyName)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::getProperty(IString* propertyName, IProperty** value)
{
    return Impl::getProperty(propertyName, value);
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::addProperty(IProperty* property)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <typename Impl>
ErrCode TmsClientPropertyObjectBaseImpl<Impl>::removeProperty(IString* propertyName)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::getOnPropertyValueWrite(IString* /*propertyName*/, IEvent** /*event*/)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

template <typename Impl>
ErrCode TmsClientPropertyObjectBaseImpl<Impl>::getOnPropertyValueRead(IString* /*propertyName*/, IEvent** /*event*/)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

template <class Impl>
ErrCode TmsClientPropertyObjectBaseImpl<Impl>::getOnAnyPropertyValueWrite(IEvent** /*event*/)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

template <class Impl>
ErrCode TmsClientPropertyObjectBaseImpl<Impl>::getOnAnyPropertyValueRead(IEvent** /*event*/)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::getVisibleProperties(IList** properties)
{
    return Impl::getVisibleProperties(properties);
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::hasProperty(IString* propertyName, Bool* hasProperty)
{
    return Impl::hasProperty(propertyName, hasProperty);
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::getAllProperties(IList** properties)
{
    return Impl::getAllProperties(properties);
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::setPropertyOrder(IList* orderedPropertyNames)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::beginUpdate()
{
    if (!hasReference("BeginUpdate"))
        return OPENDAQ_SUCCESS;

    const auto beginUpdateId = getNodeId("BeginUpdate");
    OpcUaCallMethodRequest request;
    request->inputArgumentsSize = 0;
    request->objectId = nodeId.copyAndGetDetachedValue();
    request->methodId = beginUpdateId.copyAndGetDetachedValue();
    client->callMethod(request);
    return OPENDAQ_SUCCESS;
}

template <class Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::endUpdate()
{
    if (!hasReference("EndUpdate"))
        return OPENDAQ_SUCCESS;

    const auto endUpdateId = getNodeId("EndUpdate");
    OpcUaCallMethodRequest request;
    request->inputArgumentsSize = 0;
    request->objectId = nodeId.copyAndGetDetachedValue();
    request->methodId = endUpdateId.copyAndGetDetachedValue();
    client->callMethod(request);
    return OPENDAQ_SUCCESS;
}

template <class Impl>
void TmsClientPropertyObjectBaseImpl<Impl>::addProperties(const OpcUaNodeId& parentId,
                                                          std::map<uint32_t, PropertyPtr>& orderedProperties,
                                                          std::vector<PropertyPtr>& unorderedProperties)
{
    const auto introspectionVariableTypeId = OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_INTROSPECTIONVARIABLETYPE);
    const auto structureVariableTypeId = OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_STRUCTUREVARIABLETYPE);
    const auto referenceVariableTypeId = OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_REFERENCEVARIABLETYPE);
    const auto variableBlockTypeId = OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_VARIABLEBLOCKTYPE);

    auto reader = clientContext->getAttributeReader();
    const auto& references = clientContext->getReferenceBrowser()->browse(parentId);

    for (auto& [childNodeId, ref] : references.byNodeId)
    {
        const auto typeId = OpcUaNodeId(ref->typeDefinition.nodeId);
        auto propName = String(utils::ToStdString(ref->browseName.name));
        if (propBrowseName.count(propName))
            propName = propBrowseName[propName];
        if (detail::ignoredPropertyNames.count(propName))
            continue;

        Bool hasProp;
        daq::checkErrorInfo(Impl::hasProperty(propName, &hasProp));
        PropertyPtr prop;
        if (clientContext->getReferenceBrowser()->isSubtypeOf(typeId, referenceVariableTypeId))
        {
            try
            {
                if (!hasProp)
                {
                    StringPtr refPropEval = VariantConverter<IString>::ToDaqObject(reader->getValue(childNodeId, UA_ATTRIBUTEID_VALUE));
                    prop = ReferenceProperty(propName, EvalValue(refPropEval));
                }

                referenceVariableIdMap.emplace(propName, childNodeId);
                addProperties(childNodeId, orderedProperties, unorderedProperties);
            }
            catch(const std::exception& e)
            {
                LOG_W("Failed to add {} reference property on OpcUa client property object: {}", propName, e.what());
                continue;
            }
        }
        else if (clientContext->getReferenceBrowser()->isSubtypeOf(typeId, introspectionVariableTypeId) ||
                 clientContext->getReferenceBrowser()->isSubtypeOf(typeId, structureVariableTypeId))
        {
            try
            {
                if (!hasProp)
                    prop = TmsClientProperty(daqContext, clientContext, ref->nodeId.nodeId, propName);

                introspectionVariableIdMap.emplace(propName, childNodeId);
            }
            catch(const std::exception& e)
            {
                LOG_W("Failed to add {} property on OpcUa client property object: {}", propName, e.what());
                continue;
            }
        }
        else if (clientContext->getReferenceBrowser()->isSubtypeOf(typeId, variableBlockTypeId))
        {
            try
            {
                if (!hasProp)
                {
                    prop = addVariableBlockProperty(propName, childNodeId);
                }
                else if (this->objPtr.template supportsInterface<ISyncComponent>())
                {
                    Impl::removeProperty(propName);
                    prop = addVariableBlockProperty(propName, childNodeId);
                }

                objectTypeIdMap.emplace(propName, childNodeId);
            }
            catch (const std::exception& e)
            {
                LOG_W("Failed to add {} property on OpcUa client property object: {}", propName, e.what());
                continue;
            }
        }

        if (prop.assigned())
        {
            auto numberInList = tryReadChildNumberInList(childNodeId);
            if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedProperties.count(numberInList))
                orderedProperties.emplace(numberInList, prop);
            else
                unorderedProperties.push_back(prop);
        }
    }
}

template <class Impl>
void TmsClientPropertyObjectBaseImpl<Impl>::addMethodProperties(const OpcUaNodeId& parentNodeId,
                                                                std::map<uint32_t, PropertyPtr>& orderedProperties,
                                                                std::vector<PropertyPtr>& unorderedProperties,
                                                                std::unordered_map<std::string, BaseObjectPtr>& functionPropValues)
{
    auto browser = clientContext->getReferenceBrowser();
    auto reader = clientContext->getAttributeReader();

    const auto& references = browser->browse(parentNodeId);
    const auto methodTypeId = OpcUaNodeId(0, UA_NS0ID_METHODNODE);

    for (auto& [childNodeId, ref] : references.byNodeId)
    {
        const auto typeId = OpcUaNodeId(ref->typeDefinition.nodeId);
        const auto propName = String(utils::ToStdString(ref->browseName.name));

        if (isIgnoredMethodProperty(propName))
            continue;

        Bool hasProp;
        daq::checkErrorInfo(Impl::hasProperty(propName, &hasProp));

        if (ref->nodeClass == UA_NODECLASS_METHOD)
        {
            if (!hasProp)
            {
                ListPtr<IArgumentInfo> inputArgs;
                ListPtr<IArgumentInfo> outputArgs;
                uint32_t numberInList = std::numeric_limits<uint32_t>::max();

                try
                {
                    if (browser->hasReference(childNodeId, "InputArguments"))
                    {
                        const auto inputArgsId = browser->getChildNodeId(childNodeId, "InputArguments");
                        inputArgs = VariantConverter<IArgumentInfo>::ToDaqList(reader->getValue(inputArgsId, UA_ATTRIBUTEID_VALUE));
                    }

                    if (browser->hasReference(childNodeId, "OutputArguments"))
                    {
                        const auto outputArgsId = browser->getChildNodeId(childNodeId, "OutputArguments");
                        outputArgs = VariantConverter<IArgumentInfo>::ToDaqList(reader->getValue(outputArgsId, UA_ATTRIBUTEID_VALUE));
                    }

                    if (browser->hasReference(childNodeId, "NumberInList"))
                    {
                        const auto numberInListId = browser->getChildNodeId(childNodeId, "NumberInList");
                        numberInList = VariantConverter<IInteger>::ToDaqObject(reader->getValue(numberInListId, UA_ATTRIBUTEID_VALUE));
                    }
                }
                catch(const std::exception& e)
                {
                    LOG_W("Failed to parse method properties on OpcUa client property object: {}", e.what());
                    continue;
                }

                BaseObjectPtr prop;
                BaseObjectPtr func;
                if (outputArgs.assigned() && outputArgs.getCount() == 1)
                {
                    auto callableInfo = FunctionInfo(outputArgs[0].getType(), inputArgs);
                    prop = FunctionPropertyBuilder(propName, callableInfo).setReadOnly(true).build();
                    func = TmsClientFunction(clientContext, daqContext, parentNodeId, childNodeId);
                }
                else
                {
                    auto callableInfo = ProcedureInfo(inputArgs);
                    prop = FunctionPropertyBuilder(propName, callableInfo).setReadOnly(true).build();
                    func = TmsClientProcedure(clientContext, daqContext, parentNodeId, childNodeId);
                }

                functionPropValues.emplace(propName, func);
                if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedProperties.count(numberInList))
                    orderedProperties.emplace(numberInList, prop);
                else
                    unorderedProperties.push_back(prop);
            }
        }
    }
}

template <class Impl>
PropertyPtr TmsClientPropertyObjectBaseImpl<Impl>::addVariableBlockProperty(const StringPtr& propName, const OpcUaNodeId& propNodeId)
{
    auto reader = this->clientContext->getAttributeReader();

    auto obj = TmsClientPropertyObject(daqContext, clientContext, propNodeId);
    const auto description = reader->getValue(propNodeId, UA_ATTRIBUTEID_DESCRIPTION).toString();
    auto propBuilder = ObjectPropertyBuilder(propName, obj).setDescription(String(description));

    const auto evaluationVariableTypeId = OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_EVALUATIONVARIABLETYPE);
    const auto variableBlockRefs = clientContext->getReferenceBrowser()->browse(propNodeId);

    for (auto& [browseName, variableBlockRef] : variableBlockRefs.byBrowseName)
    {
        const auto variableBlockNodeId = OpcUaNodeId(variableBlockRef->nodeId.nodeId);

        if (clientContext->getReferenceBrowser()->isSubtypeOf(variableBlockRef->typeDefinition.nodeId, evaluationVariableTypeId))
        {
            auto evalId = clientContext->getReferenceBrowser()->getChildNodeId(variableBlockNodeId, "EvaluationExpression");

            StringPtr evalStr = VariantConverter<IString>::ToDaqObject(reader->getValue(evalId, UA_ATTRIBUTEID_VALUE));

            if (browseName == "IsReadOnly")
            {
                if (evalStr.assigned())
                    propBuilder.setReadOnly(EvalValue(evalStr).asPtr<IBoolean>());
                else
                    propBuilder.setReadOnly(
                        VariantConverter<IBoolean>::ToDaqObject(reader->getValue(variableBlockNodeId, UA_ATTRIBUTEID_VALUE)));
            }
            else if (browseName == "IsVisible")
            {
                if (evalStr.assigned())
                    propBuilder.setVisible(EvalValue(evalStr).asPtr<IBoolean>());
                else
                    propBuilder.setVisible(
                        VariantConverter<IBoolean>::ToDaqObject(reader->getValue(variableBlockNodeId, UA_ATTRIBUTEID_VALUE)));
            }
        }
    }

    return propBuilder.build();
}

template <typename Impl>
void TmsClientPropertyObjectBaseImpl<Impl>::browseRawProperties()
{
    std::map<uint32_t, PropertyPtr> orderedProperties;
    std::vector<PropertyPtr> unorderedProperties;
    std::unordered_map<std::string, BaseObjectPtr> functionPropValues;

    addProperties(nodeId, orderedProperties, unorderedProperties);

    // TODO: Make sure that this is a DeviceType node
    if (hasReference("MethodSet"))
    {
        const auto methodNodeId = clientContext->getReferenceBrowser()->getChildNodeId(nodeId, "MethodSet");
        addMethodProperties(methodNodeId, orderedProperties, unorderedProperties, functionPropValues);
    }
    else
    {
        addMethodProperties(nodeId, orderedProperties, unorderedProperties, functionPropValues);
    }

    auto addPropertyIgnoreDuplicates = [this](const daq::PropertyPtr& prop)
    {
        auto ec = Impl::addProperty(prop);
        if (ec != OPENDAQ_ERR_ALREADYEXISTS)
            return ec;
        LOG_W("OPC UA exposes two properties with the same name \"{}\". The duplicate property will be ignored.", prop.getName())
        return OPENDAQ_SUCCESS;
    };

    for (const auto& val : orderedProperties)
        daq::checkErrorInfo(addPropertyIgnoreDuplicates(val.second));
    for (const auto& val : unorderedProperties)
        daq::checkErrorInfo(addPropertyIgnoreDuplicates(val));
    for (const auto& val : functionPropValues)
        daq::checkErrorInfo(Impl::setProtectedPropertyValue(String(val.first), val.second));

}

template <class Impl>
bool TmsClientPropertyObjectBaseImpl<Impl>::isIgnoredMethodProperty(const std::string& browseName)
{
    return browseName == "BeginUpdate" || browseName == "EndUpdate" || browseName == "GetErrorInformation";
}

template <class Impl>
PropertyObjectPtr TmsClientPropertyObjectBaseImpl<Impl>::cloneChildPropertyObject(const PropertyPtr& prop)
{
    const auto propPtrInternal = prop.asPtr<IPropertyInternal>();
    if (propPtrInternal.assigned() && propPtrInternal.getValueTypeUnresolved() == ctObject && prop.getDefaultValue().assigned())
    {
        const auto propName = prop.getName();
        const auto defaultValueObj = prop.getDefaultValue().asPtrOrNull<IPropertyObject>();
        if (!defaultValueObj.assigned())
            return nullptr;

        if (!isBasePropertyObject(defaultValueObj))
        {
            return defaultValueObj.asPtr<IPropertyObjectInternal>(true).clone();
        }

        if (const auto& objIt = objectTypeIdMap.find(propName); objIt != objectTypeIdMap.cend())
        {
            return TmsClientPropertyObject(daqContext, clientContext, objIt->second);
        }
        
        throw NotFoundException{"Object property with name {} not found", propName};
    }

    return nullptr;
}

template <class Impl>
bool TmsClientPropertyObjectBaseImpl<Impl>::isBasePropertyObject(const PropertyObjectPtr& propObj)
{
    return !propObj.supportsInterface<IServerCapabilityConfig>()
            && !propObj.supportsInterface<IAddressInfo>()
            && !propObj.supportsInterface<IConnectedClientInfo>();
}

template class TmsClientPropertyObjectBaseImpl<PropertyObjectImpl>;
template class TmsClientPropertyObjectBaseImpl<ComponentImpl<IComponent,ITmsClientComponent>>;
template class TmsClientPropertyObjectBaseImpl<FolderImpl<IFolderConfig, ITmsClientComponent>>;
template class TmsClientPropertyObjectBaseImpl<IoFolderImpl<ITmsClientComponent>>;
template class TmsClientPropertyObjectBaseImpl<MirroredDeviceBase<ITmsClientComponent>>;
template class TmsClientPropertyObjectBaseImpl<FunctionBlockImpl<IFunctionBlock, ITmsClientComponent>>;
template class TmsClientPropertyObjectBaseImpl<ChannelImpl<ITmsClientComponent>>;
template class TmsClientPropertyObjectBaseImpl<MirroredSignalBase<ITmsClientComponent>>;
template class TmsClientPropertyObjectBaseImpl<GenericInputPortImpl<ITmsClientComponent>>;
template class TmsClientPropertyObjectBaseImpl<ServerCapabilityConfigImpl>;
template class TmsClientPropertyObjectBaseImpl<GenericSyncComponentImpl<ISyncComponent, ITmsClientComponent>>;


END_NAMESPACE_OPENDAQ_OPCUA_TMS
