#include "opcuatms_client/objects/tms_client_property_object_impl.h"
#include "coreobjects/callable_info_factory.h"
#include "coreobjects/eval_value_factory.h"
#include "coreobjects/property_object_factory.h"
#include "opcuaclient/browser/opcuabrowser.h"
#include "opcuatms_client/objects/tms_client_property_factory.h"
#include "open62541/daqbt_nodeids.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms_client/objects/tms_client_property_object_factory.h"
#include "opcuatms_client/objects/tms_client_function_factory.h"
#include "opcuatms_client/objects/tms_client_procedure_factory.h"
#include "opendaq/mirrored_signal_impl.h"
#include "opendaq/input_port_impl.h"
#include "opendaq/channel_impl.h"
#include "opendaq/device_impl.h"
#include "opendaq/io_folder_impl.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

template <class Impl>
ErrCode TmsClientPropertyObjectBaseImpl<Impl>::setPropertyValueInternal(IString* propertyName, IBaseObject* value, bool protectedWrite)
{
    return daqTry(
        [&]()
        {
            if (const auto& it = introspectionVariableIdMap.find((StringPtr) propertyName); it != introspectionVariableIdMap.cend())
            {
                if (protectedWrite)
                {
                    PropertyPtr prop;
                    checkErrorInfo(getProperty(propertyName, &prop));
                    const bool readOnly = prop.getReadOnly();
                    if (readOnly)
                        return OPENDAQ_SUCCESS;
                }

                const auto variant = VariantConverter<IBaseObject>::ToVariant(value, nullptr, daqContext);
                client->writeValue(it->second, variant);
                return OPENDAQ_SUCCESS;
            }

            if (const auto& it = referenceVariableIdMap.find((StringPtr) propertyName); it != referenceVariableIdMap.cend())
            {
                const auto refProp = this->objPtr.getProperty(propertyName).getReferencedProperty();
                return setPropertyValue(refProp.getName(), value);
            }

            if (const auto& it = objectTypeIdMap.find((StringPtr) propertyName); it != objectTypeIdMap.cend())
            {
                return this->makeErrorInfo(OPENDAQ_ERR_NOTIMPLEMENTED, "Object type properties cannot be set over OPC UA");
            }

            return OPENDAQ_ERR_NOTFOUND;
        });
}

template <class Impl>
void TmsClientPropertyObjectBaseImpl<Impl>::init()
{
    if (!this->daqContext.getLogger().assigned())
        throw ArgumentNullException("Logger must not be null");

    this->loggerComponent = this->daqContext.getLogger().getOrAddComponent("TmsClientPropertyObject");
    clientContext->readObjectAttributes(nodeId);
    browseRawProperties();
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    return setPropertyValueInternal(propertyName, value, false);
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    return setPropertyValueInternal(propertyName, value, true);
}

template <typename Impl>
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::getPropertyValue(IString* propertyName, IBaseObject** value)
{
    return daqTry([&]() {
        if (const auto& introIt = introspectionVariableIdMap.find((StringPtr) propertyName); introIt != introspectionVariableIdMap.cend())
        {
            const auto variant = client->readValue(introIt->second);
            const auto object = VariantConverter<IBaseObject>::ToDaqObject(variant, daqContext);
            Impl::setProtectedPropertyValue(propertyName, object);
        }
        else if (referenceVariableIdMap.count((StringPtr) propertyName))
        {
            const auto refProp = this->objPtr.getProperty(propertyName).getReferencedProperty();
            return getPropertyValue(refProp.getName(), value);
        }
        else if (const auto& objIt = objectTypeIdMap.find((StringPtr) propertyName); objIt != objectTypeIdMap.cend())
        {
            *value = TmsClientPropertyObject(daqContext, clientContext, objIt->second).detach();
            return OPENDAQ_SUCCESS;
        }

        return Impl::getPropertyValue(propertyName, value);
    });
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
ErrCode INTERFACE_FUNC TmsClientPropertyObjectBaseImpl<Impl>::getOnPropertyValueWrite(IString* propertyName, IEvent** event)
{
    return Impl::getOnPropertyValueWrite(propertyName, event);
}

template <typename Impl>
ErrCode TmsClientPropertyObjectBaseImpl<Impl>::getOnPropertyValueRead(IString* propertyName, IEvent** event)
{
    return Impl::getOnPropertyValueRead(propertyName, event);
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
void TmsClientPropertyObjectBaseImpl<Impl>::addProperties(const OpcUaNodeId& parentId,
                                                          std::map<uint32_t, PropertyPtr>& orderedProperties,
                                                          std::vector<PropertyPtr>& unorderedProperties)
{
    const auto introspectionVariableTypeId = OpcUaNodeId(NAMESPACE_TMSBT, UA_TMSBTID_INTROSPECTIONVARIABLETYPE);
    const auto structureVariableTypeId = OpcUaNodeId(NAMESPACE_TMSBT, UA_TMSBTID_STRUCTUREVARIABLETYPE);
    const auto referenceVariableTypeId = OpcUaNodeId(NAMESPACE_TMSBT, UA_TMSBTID_REFERENCEVARIABLETYPE);
    const auto variableBlockTypeId = OpcUaNodeId(NAMESPACE_TMSBT, UA_TMSBTID_VARIABLEBLOCKTYPE);

    const auto& references = clientContext->getReferenceBrowser()->browse(parentId);

    for (auto& [childNodeId, ref] : references.byNodeId)
    {
        const auto typeId = OpcUaNodeId(ref->typeDefinition.nodeId);
        const auto propName = String(utils::ToStdString(ref->browseName.name));

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

                referenceVariableIdMap.insert(std::pair(propName, childNodeId));
                addProperties(childNodeId, orderedProperties, unorderedProperties);
            }
            catch(...)
            {
                // TODO: Log failure to add function/procedure.
                continue;
            }
        }
        else if (clientContext->getReferenceBrowser()->isSubtypeOf(typeId, introspectionVariableTypeId) ||
                 clientContext->getReferenceBrowser()->isSubtypeOf(typeId, structureVariableTypeId))
        {
            try
            {
                if (!hasProp)
                    prop = TmsClientProperty(daqContext, clientContext, ref->nodeId.nodeId);

                introspectionVariableIdMap.insert(std::pair(propName, childNodeId));
            }                     
            catch(const std::exception& e)
            {
                // TODO: Log failure to add function/procedure.
                continue;
            }
        }
        else if (clientContext->getReferenceBrowser()->isSubtypeOf(typeId, variableBlockTypeId))
        {
            if (!hasProp)
            {
                auto obj = TmsClientPropertyObject(daqContext, clientContext, childNodeId);
                const auto description = reader->getValue(childNodeId, UA_ATTRIBUTEID_DESCRIPTION).toString();
                auto propBuilder = ObjectPropertyBuilder(propName, obj).setDescription(String(description));

                const auto evaluationVariableTypeId = OpcUaNodeId(NAMESPACE_DAQBT, UA_DAQBTID_EVALUATIONVARIABLETYPE);
                const auto variableBlockRefs = clientContext->getReferenceBrowser()->browse(childNodeId);

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
                                propBuilder.setReadOnly(VariantConverter<IBoolean>::ToDaqObject(reader->getValue(variableBlockNodeId, UA_ATTRIBUTEID_VALUE)));
                        }
                        else if (browseName == "IsVisible")
                        {
                            if (evalStr.assigned())
                                propBuilder.setVisible(EvalValue(evalStr).asPtr<IBoolean>());
                            else
                                propBuilder.setVisible(VariantConverter<IBoolean>::ToDaqObject(reader->getValue(variableBlockNodeId, UA_ATTRIBUTEID_VALUE)));
                        }
                    }
                }

                prop = propBuilder.build();
            }

            objectTypeIdMap.insert(std::pair(propName, childNodeId));
        }

        if (prop.assigned())
        {
            auto numberInList = readChildNumberInList(childNodeId);
            if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedProperties.count(numberInList))
                orderedProperties.insert(std::pair<uint32_t, PropertyPtr>(numberInList, prop));
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
                    LOG_W("Failed to parse method properties {}", e.what());
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

                functionPropValues.insert(std::pair<std::string, BaseObjectPtr>(propName, func));
                if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedProperties.count(numberInList))
                    orderedProperties.insert(std::pair<uint32_t, PropertyPtr>(numberInList, prop));
                else
                    unorderedProperties.push_back(prop);
            }
        }
    }
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

    for (const auto& val : orderedProperties)
        daq::checkErrorInfo(Impl::addProperty(val.second));
    for (const auto& val : unorderedProperties)
        daq::checkErrorInfo(Impl::addProperty(val));
    for (const auto& val : functionPropValues)
        daq::checkErrorInfo(Impl::setProtectedPropertyValue(String(val.first), val.second));

}

template class TmsClientPropertyObjectBaseImpl<PropertyObjectImpl>;
template class TmsClientPropertyObjectBaseImpl<ComponentImpl<>>;
template class TmsClientPropertyObjectBaseImpl<FolderImpl<IFolderConfig>>;
template class TmsClientPropertyObjectBaseImpl<IoFolderImpl>;
template class TmsClientPropertyObjectBaseImpl<Device>;
template class TmsClientPropertyObjectBaseImpl<FunctionBlock>;
template class TmsClientPropertyObjectBaseImpl<Channel>;
template class TmsClientPropertyObjectBaseImpl<MirroredSignal>;
template class TmsClientPropertyObjectBaseImpl<InputPortImpl>;
template class TmsClientPropertyObjectBaseImpl<StreamingInfoConfigImpl>;


END_NAMESPACE_OPENDAQ_OPCUA_TMS
