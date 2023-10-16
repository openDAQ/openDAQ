#include "opcuatms_client/objects/tms_client_property_object_impl.h"
#include "coreobjects/callable_info_factory.h"
#include "coreobjects/eval_value_factory.h"
#include "coreobjects/property_object_factory.h"
#include "opcuaclient/browser/opcuabrowser.h"
#include "opcuatms_client/objects/tms_client_property_factory.h"
#include "open62541/tmsbt_nodeids.h"
#include "opcuatms/converters/variant_converter.h"
#include "opcuatms_client/objects/tms_client_property_object_factory.h"
#include "opcuatms_client/objects/tms_client_function_factory.h"
#include "opcuatms_client/objects/tms_client_procedure_factory.h"
#include "opendaq/signal_remote_impl.h"
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

template <typename Impl>
void TmsClientPropertyObjectBaseImpl<Impl>::addProperties(const tsl::ordered_map<OpcUaNodeId, OpcUaObject<UA_ReferenceDescription>>& references)
{
    const auto introspectionVariableTypeId = OpcUaNodeId(NAMESPACE_TMSBT, UA_TMSBTID_INTROSPECTIONVARIABLETYPE);
    const auto structureVariableTypeId = OpcUaNodeId(NAMESPACE_TMSBT, UA_TMSBTID_STRUCTUREVARIABLETYPE);
    const auto referenceVariableTypeId = OpcUaNodeId(NAMESPACE_TMSBT, UA_TMSBTID_REFERENCEVARIABLETYPE);
    const auto variableBlockTypeId = OpcUaNodeId(NAMESPACE_TMSBT, UA_TMSBTID_VARIABLEBLOCKTYPE);

    for (auto& [childNodeId, ref] : references)
    {
        const auto typeId = OpcUaNodeId(ref->typeDefinition.nodeId);
        auto propName = String(client->readBrowseName(childNodeId));
        Bool hasProp;
        daq::checkErrorInfo(Impl::hasProperty(propName, &hasProp));

        if (referenceUtils.isInstanceOf(typeId, referenceVariableTypeId))
        {
            try
            {
                if (!hasProp)
                {
                    StringPtr refPropEval = VariantConverter<IString>::ToDaqObject(client->readValue(childNodeId));
                    auto refProp = ReferenceProperty(propName, EvalValue(refPropEval));
                    daq::checkErrorInfo(Impl::addProperty(refProp));
                }

                referenceVariableIdMap.insert(std::pair(propName, childNodeId));
                const auto& refPropReferences = referenceUtils.getReferences(childNodeId);
                addProperties(refPropReferences);
            }
            catch(...)
            {
                // TODO: Log failure to add function/procedure.
                continue;
            }
        }
        else if (referenceUtils.isInstanceOf(typeId, introspectionVariableTypeId) ||
                 referenceUtils.isInstanceOf(typeId, structureVariableTypeId))
        {
            try
            {
                if (!hasProp)
                {
                    auto prop = TmsClientProperty(daqContext, clientContext, ref->nodeId.nodeId);
                    daq::checkErrorInfo(Impl::addProperty(prop));
                }
                introspectionVariableIdMap.insert(std::pair(propName, childNodeId));
            }                     
            catch(...)
            {
                // TODO: Log failure to add function/procedure.
                continue;
            }
        }
        else if (referenceUtils.isInstanceOf(typeId, variableBlockTypeId))
        {
            if (!hasProp)
            {
                auto obj = TmsClientPropertyObject(daqContext, clientContext, childNodeId);
                auto prop = ObjectPropertyBuilder(propName, obj).setDescription(String(client->readDescription(childNodeId)));

                const auto evaluationVariableTypeId = OpcUaNodeId(NAMESPACE_TMSBT, UA_TMSBTID_EVALUATIONVARIABLETYPE);
                const auto variableBlockRefs = referenceUtils.getReferences(childNodeId);

                for (auto& [variableBlockNodeId, variableBlockRef] : variableBlockRefs)
                {
                    const auto browseName = referenceUtils.getBrowseName(variableBlockRef);
                    if (referenceUtils.isInstanceOf(variableBlockRef->typeDefinition.nodeId, evaluationVariableTypeId))
                    {
                        auto evalId = referenceUtils.getChildNodeId(variableBlockNodeId, "EvaluationExpression");

                        StringPtr evalStr = VariantConverter<IString>::ToDaqObject(client->readValue(evalId));

                        if (browseName == "IsReadOnly")
                        {
                            if (evalStr.assigned())
                                prop.setReadOnly(EvalValue(evalStr).asPtr<IBoolean>());
                            else
                                prop.setReadOnly(VariantConverter<IBoolean>::ToDaqObject(client->readValue(variableBlockNodeId)));
                        }
                        else if (browseName == "IsVisible")
                        {
                            if (evalStr.assigned())
                                prop.setVisible(EvalValue(evalStr).asPtr<IBoolean>());
                            else
                                prop.setVisible(VariantConverter<IBoolean>::ToDaqObject(client->readValue(variableBlockNodeId)));
                        }
                    }
                }
                
                daq::checkErrorInfo(Impl::addProperty(prop.build()));
            }

            objectTypeIdMap.insert(std::pair(propName, childNodeId));
        }
        
    }
}

template <class Impl>
void TmsClientPropertyObjectBaseImpl<Impl>::addMethodProperties(
    const tsl::ordered_map<opcua::OpcUaNodeId, opcua::OpcUaObject<UA_ReferenceDescription>>& references, const OpcUaNodeId& parentNodeId)
{
    const auto methodTypeId = OpcUaNodeId(0, UA_NS0ID_METHODNODE);

    for (auto& [childNodeId, ref] : references)
    {
        const auto typeId = OpcUaNodeId(ref->typeDefinition.nodeId);
        auto propName = String(client->readBrowseName(childNodeId));
        Bool hasProp;
        daq::checkErrorInfo(Impl::hasProperty(propName, &hasProp));

        if (ref->nodeClass == UA_NODECLASS_METHOD)
        {
            if (!hasProp)
            {
                ListPtr<IArgumentInfo> inputArgs;
                ListPtr<IArgumentInfo> outputArgs;

                try
                {
                    if (referenceUtils.hasReference(childNodeId, "InputArguments"))
                        inputArgs = VariantConverter<IArgumentInfo>::ToDaqList(client->readValue(referenceUtils.getChildNodeId(childNodeId, "InputArguments")));

                    if (referenceUtils.hasReference(childNodeId, "OutputArguments"))
                        outputArgs = VariantConverter<IArgumentInfo>::ToDaqList(client->readValue(referenceUtils.getChildNodeId(childNodeId, "OutputArguments")));
                }
                catch(...)
                {
                    // TODO: Log failure to add function/procedure.
                    continue;
                }

                if (outputArgs.assigned() && outputArgs.getCount() == 1)
                {
                    auto callableInfo = FunctionInfo(outputArgs[0].getType(), inputArgs);
                    auto prop = FunctionPropertyBuilder(propName, callableInfo).setReadOnly(true).build();
                    daq::checkErrorInfo(Impl::addProperty(prop));

                    auto clientFunc = TmsClientFunction(clientContext, daqContext, parentNodeId, childNodeId);
                    daq::checkErrorInfo(Impl::setProtectedPropertyValue(propName, clientFunc));
                }
                else
                {
                    auto callableInfo = ProcedureInfo(inputArgs);
                    auto prop = FunctionPropertyBuilder(propName, callableInfo).setReadOnly(true).build();
                    daq::checkErrorInfo(Impl::addProperty(prop));

                    auto clientProcedure = TmsClientProcedure(clientContext, daqContext, parentNodeId, childNodeId);
                    daq::checkErrorInfo(Impl::setProtectedPropertyValue(propName, clientProcedure));
                }
            }
        }
    }
}

template <typename Impl>
void TmsClientPropertyObjectBaseImpl<Impl>::browseRawProperties()
{
    const auto& references = referenceUtils.getReferences(nodeId);
    addProperties(references);

    // TODO: Make sure that this is a DeviceType node
    if (hasReference("MethodSet"))
    {
        const auto methodNodeId = referenceUtils.getChildNodeId(nodeId, "MethodSet");
        const auto& methodReferences = referenceUtils.getReferences(methodNodeId);
        addMethodProperties(methodReferences, methodNodeId);
    }
    else
    {
        addMethodProperties(references, nodeId);
    }
}

template class TmsClientPropertyObjectBaseImpl<PropertyObjectImpl>;
template class TmsClientPropertyObjectBaseImpl<ComponentImpl<>>;
template class TmsClientPropertyObjectBaseImpl<FolderImpl<IFolderConfig>>;
template class TmsClientPropertyObjectBaseImpl<IoFolderImpl>;
template class TmsClientPropertyObjectBaseImpl<Device>;
template class TmsClientPropertyObjectBaseImpl<FunctionBlock>;
template class TmsClientPropertyObjectBaseImpl<Channel>;
template class TmsClientPropertyObjectBaseImpl<SignalRemote<SignalStandardProps::Skip>>;
template class TmsClientPropertyObjectBaseImpl<InputPortImpl>;
template class TmsClientPropertyObjectBaseImpl<StreamingInfoConfigImpl>;


END_NAMESPACE_OPENDAQ_OPCUA_TMS
