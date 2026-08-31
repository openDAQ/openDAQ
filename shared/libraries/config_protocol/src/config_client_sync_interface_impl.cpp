#include <config_protocol/config_client_sync_interface_impl.h>
#include <opendaq/sync_interface.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <opendaq/component_deserialize_context_ptr.h>
#include <coretypes/objectptr.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/function_ptr.h>

namespace daq::config_protocol
{

ConfigClientSyncInterfaceImpl::ConfigClientSyncInterfaceImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                             const std::string& remoteGlobalId,
                                                             const TypeManagerPtr& manager)
    : Super(configProtocolClientComm, remoteGlobalId, manager)
{
}

ErrCode ConfigClientSyncInterfaceImpl::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    if (remoteUpdating)
        return Impl::setPropertyValue(propertyName, value);
    return Super::setPropertyValue(propertyName, value);
}

ErrCode ConfigClientSyncInterfaceImpl::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    if (remoteUpdating)
        return Impl::setProtectedPropertyValue(propertyName, value);
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED, "Setting protected values is not allowed on client sync interface");
}

ErrCode ConfigClientSyncInterfaceImpl::clearPropertyValue(IString* propertyName)
{
    if (remoteUpdating)
        return Impl::clearPropertyValue(propertyName);
    return Super::clearPropertyValue(propertyName);
}

ErrCode ConfigClientSyncInterfaceImpl::addProperty(IProperty* property)
{
    if (remoteUpdating)
        return Impl::addProperty(property);
    return Super::addProperty(property);
}

ErrCode ConfigClientSyncInterfaceImpl::removeProperty(IString* propertyName)
{
    if (remoteUpdating)
        return Impl::removeProperty(propertyName);
    return Super::removeProperty(propertyName);
}

ErrCode ConfigClientSyncInterfaceImpl::beginUpdate()
{
    if (remoteUpdating)
        return Impl::beginUpdate();
    return Super::beginUpdate();
}

ErrCode ConfigClientSyncInterfaceImpl::endUpdate()
{
    if (remoteUpdating)
        return Impl::endUpdate();
    return Super::endUpdate();
}

ErrCode ConfigClientSyncInterfaceImpl::deserializeValues(ISerializedObject* serializedObject,
                                                          IBaseObject* context,
                                                          IFunction* callbackFactory)
{
    OPENDAQ_PARAM_NOT_NULL(serializedObject);

    Bool hasSyncStatus {False};
    OPENDAQ_RETURN_IF_FAILED(serializedObject->hasKey(String("SyncStatus"), &hasSyncStatus));

    if (hasSyncStatus)
    {
        BaseObjectPtr objPtr;
        OPENDAQ_RETURN_IF_FAILED(serializedObject->readObject(String("SyncStatus"), context, callbackFactory, &objPtr));
        
        if (const auto newStatusContainer = objPtr.asPtrOrNull<IComponentStatusContainer>(); newStatusContainer.assigned())
            statusContainer = newStatusContainer;
    }

    return OPENDAQ_SUCCESS;
}

ErrCode ConfigClientSyncInterfaceImpl::getDeserializedParameter(IString* parameter, IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(parameter);
    OPENDAQ_PARAM_NOT_NULL(value);
    return OPENDAQ_NOTFOUND;
}

ErrCode ConfigClientSyncInterfaceImpl::Deserialize(ISerializedObject* serialized,
                                                    IBaseObject* context,
                                                    IFunction* factoryCallback,
                                                    IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);
    OPENDAQ_PARAM_NOT_NULL(context);

    const ErrCode errCode = daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        const auto contextPtr = BaseObjectPtr::Borrow(context);
        if (!contextPtr.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Deserialization context not assigned");

        const auto componentDeserializeContext = contextPtr.asPtrOrNull<IComponentDeserializeContext>(true);
        if (!componentDeserializeContext.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Invalid deserialization context");

        const auto configDeserializeContext = componentDeserializeContext.asPtr<IConfigProtocolDeserializeContext>();

        const auto serializedPtr = SerializedObjectPtr::Borrow(serialized);
        const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

        PropertyObjectPtr propObj = Super::DeserializePropertyObject(
            serializedPtr,
            contextPtr,
            factoryCallbackPtr,
            [&factoryCallback](const SerializedObjectPtr& serialized, const ComponentDeserializeContextPtr& deserializeContext, const StringPtr& className)
            {
                const auto ctx = deserializeContext.asPtr<IConfigProtocolDeserializeContext>(true);
                auto obj = createWithImplementation<ISyncInterface, ConfigClientSyncInterfaceImpl>(
                    ctx->getClientComm(),
                    ctx->getRemoteGlobalId(),
                    ctx->getTypeManager());
                obj.asPtr<IDeserializeComponent>(true).deserializeValues(serialized, deserializeContext, factoryCallback);
                obj.as<IConfigClientObject>(true)->setRemoteUpdating(true);
                return obj;
            });

        propObj.as<IConfigClientObject>(true)->setRemoteUpdating(false);
        const auto deserializeComponent = propObj.asPtr<IDeserializeComponent>(true);
        deserializeComponent.complete();

        *obj = propObj.detach();
        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

void ConfigClientSyncInterfaceImpl::handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args)
{
    switch (static_cast<CoreEventId>(args.getEventId()))
    {
        case CoreEventId::StatusChanged:
            statusChanged(args);
            break;
        default:
            break;
    }

    Super::handleRemoteCoreObjectInternal(sender, args);
}

void ConfigClientSyncInterfaceImpl::statusChanged(const CoreEventArgsPtr& args)
{
    const DictPtr<IString, IBaseObject> params = args.getParameters();
    StringPtr msg = params.getOrDefault("Message", "");

    for (const auto& [key, value] : params)
    {
        if (value.getCoreType() == CoreType::ctEnumeration)
        {
            statusContainer.asPtr<IComponentStatusContainerPrivate>().setStatusWithMessage(
                key, value.template asPtr<IEnumeration>(true), msg);
            msg = String("");
        }
    }
}

} // namespace daq::config_protocol
