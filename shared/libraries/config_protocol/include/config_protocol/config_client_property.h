#pragma once
#include <coreobjects/property_impl.h>
#include <config_protocol/config_client_object_impl.h>
#include <config_protocol/config_protocol_deserialize_context.h>

namespace daq::config_protocol
{

struct ConfigPropertyBuildParams
{
    Bool hasOnReadListeners = false;
    Bool hasSelectionValuesListener = false;
    Bool hasSuggestedValuesListeners = false;
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

    ErrCode getSuggestedValuesInternal(IList** values, bool lock) override;
    ErrCode getSelectionValuesInternal(IBaseObject** values, bool lock) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    Bool hasOnReadListeners;
    Bool hasOnGetSuggestedValuesListeners;
    Bool hasOnGetSelectionValuesListeners;
};

inline ConfigClientPropertyImpl::ConfigClientPropertyImpl(const PropertyBuilderPtr& propertyBuilder,
                                                          const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                          const std::string& remoteGlobalId,
                                                          const ConfigPropertyBuildParams& buildParams)
    : ConfigClientObjectImpl(configProtocolClientComm, remoteGlobalId)
{
    this->valueType = propertyBuilder.getValueType();
    this->name = propertyBuilder.getName();
    this->description = propertyBuilder.getDescription();
    this->unit = propertyBuilder.getUnit();
    this->minValue = propertyBuilder.getMinValue();
    this->maxValue = propertyBuilder.getMaxValue();
    this->defaultValue = propertyBuilder.getDefaultValue();
    this->visible = propertyBuilder.getVisible();
    this->readOnly = propertyBuilder.getReadOnly();
    this->selectionValues = propertyBuilder.getSelectionValues();
    this->suggestedValues = propertyBuilder.getSuggestedValues();
    this->refProp = propertyBuilder.getReferencedProperty();
    this->coercer = propertyBuilder.getCoercer();
    this->validator = propertyBuilder.getValidator();
    this->callableInfo = propertyBuilder.getCallableInfo();
    this->onValueWrite = (IEvent*) propertyBuilder.getOnPropertyValueWrite();
    this->onValueRead = (IEvent*) propertyBuilder.getOnPropertyValueRead();
    this->onSuggestedValuesRead = (IEvent*) propertyBuilder.getOnSuggestedValuesRead();
    this->onSelectionValuesRead = (IEvent*) propertyBuilder.getOnSelectionValuesRead();

    this->hasOnGetSuggestedValuesListeners = buildParams.hasSuggestedValuesListeners;
    this->hasOnGetSelectionValuesListeners = buildParams.hasSelectionValuesListener;
    this->hasOnReadListeners = buildParams.hasOnReadListeners;

    propPtr = this->borrowPtr<PropertyPtr>();
    owner = nullptr;

    checkErrorInfo(validateDuringConstruction());
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
        std::string path = pathPtr.assigned() ? path : "";
        *values = this->clientComm->getSuggestedValues(this->remoteGlobalId, path, this->name);
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
        std::string path = pathPtr.assigned() ? path : "";
        *values = this->clientComm->getSelectionValues(this->remoteGlobalId, path, this->name);
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

        PropertyPtr prop = createWithImplementation<IProperty, ConfigClientPropertyImpl>(builder, ctx->getClientComm(), ctx->getRemoteGlobalId(), params);

        *obj = prop.detach();
    });
}
}
