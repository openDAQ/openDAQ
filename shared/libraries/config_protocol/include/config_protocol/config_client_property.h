/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <coreobjects/property_impl.h>
#include <config_protocol/config_client_object_impl.h>
#include <config_protocol/config_protocol_deserialize_context.h>
#include <atomic>

namespace daq::config_protocol
{

struct ConfigPropertyBuildParams
{
    Bool hasOnReadListeners = false;
    Bool hasSelectionValuesListener = false;
    Bool hasSuggestedValuesListeners = false;
    Int propertyType = static_cast<Int>(PropertyType::Undefined);
};

class ConfigClientPropertyImpl : public PropertyImpl, public ConfigClientObjectImpl
{
public:
    ConfigClientPropertyImpl(const PropertyBuilderPtr& propertyBuilder,
                             const ConfigProtocolClientCommPtr& configProtocolClientComm,
                             const std::string& remoteGlobalId,
                             const ConfigPropertyBuildParams& buildParams);
    
    ErrCode INTERFACE_FUNC getHasOnReadListeners(Bool* hasListeners) override;
    ErrCode INTERFACE_FUNC getHasOnGetSuggestedValuesListeners(Bool* hasListeners) override;
    ErrCode INTERFACE_FUNC getHasOnGetSelectionValuesListeners(Bool* hasListeners) override;
    ErrCode INTERFACE_FUNC getPropertyType(PropertyType* type) override;

    ErrCode getSuggestedValuesInternal(IList** values, bool lock) override;
    ErrCode getSelectionValuesInternal(IBaseObject** values, bool lock) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    void setRemoteGlobalId(const std::string newId) override;

    Bool hasOnReadListeners;
    Bool hasOnGetSuggestedValuesListeners;
    Bool hasOnGetSelectionValuesListeners;

private:
    // Set once selection values read from the server were a dictionary; read by any thread
    std::atomic<bool> remoteSelectionValuesAreDict{false};
};

inline ConfigClientPropertyImpl::ConfigClientPropertyImpl(const PropertyBuilderPtr& propertyBuilder,
                                                          const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                          const std::string& remoteGlobalId,
                                                          const ConfigPropertyBuildParams& buildParams)
    : PropertyImpl(propertyBuilder.getObject())
    , ConfigClientObjectImpl(configProtocolClientComm, remoteGlobalId)
{
    this->hasOnGetSuggestedValuesListeners = buildParams.hasSuggestedValuesListeners;
    this->hasOnGetSelectionValuesListeners = buildParams.hasSelectionValuesListener;
    this->hasOnReadListeners = buildParams.hasOnReadListeners;

    // The server sends the type of a property whose selection values it computes; without it the type is deduced from the value type
    if (buildParams.propertyType != static_cast<Int>(PropertyType::Undefined))
        propertyType = static_cast<PropertyType>(buildParams.propertyType);
    else if (hasOnGetSelectionValuesListeners && (valueType == ctInt || valueType == ctString || valueType == ctFloat))
        propertyType = valueType == ctInt && !isIntegerValueSelection ? PropertyType::IndexSelection : PropertyType::Selection;
}

inline ErrCode INTERFACE_FUNC ConfigClientPropertyImpl::getHasOnReadListeners(Bool* hasListeners)
{
    OPENDAQ_PARAM_NOT_NULL(hasListeners);

    *hasListeners = hasOnReadListeners;
    return OPENDAQ_SUCCESS;
}

inline ErrCode INTERFACE_FUNC ConfigClientPropertyImpl::getHasOnGetSuggestedValuesListeners(Bool* hasListeners)
{
    OPENDAQ_PARAM_NOT_NULL(hasListeners);
    
    *hasListeners = hasOnGetSuggestedValuesListeners;
    return OPENDAQ_SUCCESS;
}

inline ErrCode INTERFACE_FUNC ConfigClientPropertyImpl::getHasOnGetSelectionValuesListeners(Bool* hasListeners)
{
    OPENDAQ_PARAM_NOT_NULL(hasListeners);
    
    *hasListeners = hasOnGetSelectionValuesListeners;
    return OPENDAQ_SUCCESS;
}

inline ErrCode INTERFACE_FUNC ConfigClientPropertyImpl::getPropertyType(PropertyType* type)
{
    OPENDAQ_RETURN_IF_FAILED(PropertyImpl::getPropertyType(type));

    // Without the type from the server, only its values, a dictionary, tell a sparse selection from an index selection
    if (*type == PropertyType::IndexSelection && remoteSelectionValuesAreDict)
        *type = PropertyType::SparseSelection;
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConfigClientPropertyImpl::getSuggestedValuesInternal(IList** values, bool lock)
{
    const auto protocolVersion = this->clientComm->getProtocolVersion();
    if (!hasOnGetSuggestedValuesListeners || protocolVersion < 17)
        return PropertyImpl::getSuggestedValuesInternal(values, lock);
    
    OPENDAQ_PARAM_NOT_NULL(values);
    return daqTry([this, values] 
    {
        auto ownerPtr = getOwner();
        if (!ownerPtr.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NO_OWNER, "Owner of config property not assigned.");
        auto ownerInternal = ownerPtr.asPtr<IPropertyObjectInternal>();

        auto pathPtr = ownerInternal.getPath();
        std::string path = pathPtr.assigned() ? pathPtr : "";
        *values = this->clientComm->getSuggestedValues(this->remoteGlobalId, path, this->name).detach();
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ConfigClientPropertyImpl::getSelectionValuesInternal(IBaseObject** values, bool lock)
{
    const auto protocolVersion = this->clientComm->getProtocolVersion();
    if (!hasOnGetSelectionValuesListeners || protocolVersion < 17)
        return PropertyImpl::getSelectionValuesInternal(values, lock);

    OPENDAQ_PARAM_NOT_NULL(values);

    return daqTry([this, values] 
    {
        auto ownerPtr = getOwner();
        if (!ownerPtr.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NO_OWNER, "Owner of config property not assigned.");
        auto ownerInternal = ownerPtr.asPtr<IPropertyObjectInternal>();

        auto pathPtr = ownerInternal.getPath();
        std::string path = pathPtr.assigned() ? pathPtr : "";
        *values = this->clientComm->getSelectionValues(this->remoteGlobalId, path, this->name).detach();
        if (BaseObjectPtr::Borrow(*values).supportsInterface<IDict>())
            remoteSelectionValuesAreDict = true;
        return OPENDAQ_SUCCESS;
    });
}

inline ErrCode ConfigClientPropertyImpl::Deserialize(ISerializedObject* serialized,
                                                     IBaseObject* context,
                                                     IFunction* factoryCallback,
                                                     IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&obj, &serialized, &context, &factoryCallback]()
    {
        if (!serialized)
            DAQ_THROW_EXCEPTION(ArgumentNullException, "Serialized object not assigned");

        const auto contextPtr = BaseObjectPtr::Borrow(context);
        if (!contextPtr.assigned())
            DAQ_THROW_EXCEPTION(ArgumentNullException, "Deserialization context not assigned");

        const auto componentDeserializeContext = contextPtr.asPtrOrNull<IComponentDeserializeContext>(true);
        if (!componentDeserializeContext.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid deserialization context");

        const auto ctx = componentDeserializeContext.asPtr<IConfigProtocolDeserializeContext>();

        StringPtr name;
        ErrCode errCode = serialized->readString(String("name"), &name);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);

        const auto builder = PropertyBuilder(name);
        ReadBuilderDeserializeValues(builder, serialized, context, factoryCallback);

        auto params = ConfigPropertyBuildParams();
        errCode = serialized->readBool(String("HasOnReadListeners"), &params.hasOnReadListeners);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);

        errCode = serialized->readBool(String("HasSelectionValuesListeners"), &params.hasSelectionValuesListener);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);

        errCode = serialized->readBool(String("HasSuggestedValuesListeners"), &params.hasSuggestedValuesListeners);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);

        errCode = serialized->readInt(String("propertyType"), &params.propertyType);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);

        PropertyPtr prop = createWithImplementation<IProperty, ConfigClientPropertyImpl>(builder, ctx->getClientComm(), ctx->getRemoteGlobalId(), params);

        *obj = prop.detach();
        return OPENDAQ_SUCCESS;
    });
}

inline void ConfigClientPropertyImpl::setRemoteGlobalId(const std::string newId)
{
    ConfigClientObjectImpl::setRemoteGlobalId(newId);
    if (this->valueType == CoreType::ctObject)
    {
        if (auto configObj = defaultValue.asPtrOrNull<IConfigClientObject>(true); configObj.assigned())
            configObj->setRemoteGlobalId(String(newId));
    }
}

}
