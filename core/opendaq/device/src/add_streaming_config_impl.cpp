#include <opendaq/add_streaming_config_impl.h>
#include <opendaq/transport_option_impl.h>
#include <opendaq/container_utils.h>
#include <opendaq/add_config_transport_selection.h>
#include <coreobjects/property_factory.h>
#include <opendaq/component_type_private_ptr.h>
#include <coreobjects/eval_value_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace add_streaming_config
{
static const char* TypeConfig = "TypeConfig";
static const char* AuthenticationConfig = "AuthenticationConfig";

static const char* StreamingTypeKey = "streamingType";
static const char* AvailableTransportOptionsKey = "availableTransportOptions";
static const char* SelectedTransportOptionKey = "selectedTransportOption";
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType)
    : AddStreamingConfigImpl(streamingType, DictPtr<IString, ITransportOption>())
{
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType,
                                               const DictPtr<IString, ITransportOption>& transportOptions)
    : AddStreamingConfigImpl(
          streamingType, transportOptions.assigned() ? transportOptions : streamingType.getAvailableTransportOptions(), Partial())
{
    Super::addProperty(ObjectProperty(add_streaming_config::TypeConfig, streamingType.createDefaultConfig()));
    if (this->streamingType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods().getCount() > 0)
        Super::addProperty(ObjectProperty(add_streaming_config::AuthenticationConfig, streamingType.createAuthenticationConfig()));

    add_config::AddTransportSelectionProperties(objPtr, this->transportOptions);
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType,
                                               const DictPtr<IString, ITransportOption>& transportOptions,
                                               Partial)
    : Super()
    , streamingType(streamingType)
    , transportOptions(FrozenClone(transportOptions.assigned() ? transportOptions : Dict<IString, ITransportOption>()))
{
}

ErrCode AddStreamingConfigImpl::getStreamingType(IStreamingType** type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = streamingType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AddStreamingConfigImpl::getTypeConfig(IPropertyObject** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    return daqTry([&] { *config = objPtr.getPropertyValue(add_streaming_config::TypeConfig).asPtr<IPropertyObject>().detach(); });
}

ErrCode AddStreamingConfigImpl::getAuthenticationConfig(IAuthenticationConfig** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    if (this->streamingType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods().getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "This streaming type supports no authentication method");

    return daqTry([&] { *config = objPtr.getPropertyValue(add_streaming_config::AuthenticationConfig).asPtr<IAuthenticationConfig>().detach(); });
}

ErrCode AddStreamingConfigImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    return daqTry([&]
    {
        auto lock = getRecursiveConfigLock2();
        auto obj = createWithImplementation<IPropertyObject, AddStreamingConfigImpl>(streamingType, transportOptions, Partial());
        dynamic_cast<AddStreamingConfigImpl*>(obj.getObject())->configureClonedMembers(getCloneParameters());
        *cloned = obj.detach();
    });
}

ErrCode AddStreamingConfigImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ErrCode AddStreamingConfigImpl::getAvailableTransportOptions(IDict** transportOptions)
{
    OPENDAQ_PARAM_NOT_NULL(transportOptions);

    *transportOptions = this->transportOptions.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AddStreamingConfigImpl::setTransportOption(IString* id)
{
    return add_config::SetTransportOption(objPtr, transportOptions, id);
}

ErrCode AddStreamingConfigImpl::getSelectedTransportOption(ITransportOption** transportOption)
{
    return add_config::GetSelectedTransportOption(objPtr, transportOptions, transportOption);
}

ErrCode AddStreamingConfigImpl::setPort(Int port)
{
    return add_config::SetPort(objPtr, port);
}

ErrCode AddStreamingConfigImpl::getPort(Int* port)
{
    return add_config::GetPort(objPtr, transportOptions, port);
}

ConstCharPtr AddStreamingConfigImpl::SerializeId()
{
    return "AddStreamingConfig";
}

ErrCode AddStreamingConfigImpl::Deserialize(ISerializedObject* serialized,
                                            IBaseObject* context,
                                            IFunction* factoryCallback,
                                            IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&]
    {
        const auto createConfig =
            [&factoryCallback](const SerializedObjectPtr& serialized, const BaseObjectPtr& context, const StringPtr& /*className*/)
        {
            const StreamingTypePtr streamingType = serialized.readObject(add_streaming_config::StreamingTypeKey, context, factoryCallback);

            // The key is left out for a config without options
            const DictPtr<IString, ITransportOption> options =
                serialized.hasKey(add_streaming_config::AvailableTransportOptionsKey)
                    ? DictPtr<IString, ITransportOption>(
                          serialized.readObject(add_streaming_config::AvailableTransportOptionsKey, context, factoryCallback).asPtr<IDict>())
                    : Dict<IString, ITransportOption>();

            return createWithImplementation<IAddStreamingConfig, AddStreamingConfigImpl>(streamingType, options, Partial());
        };

        *obj = Super::DeserializePropertyObject(serialized, context, factoryCallback, createConfig).detach();
    });
}

ErrCode AddStreamingConfigImpl::serializeCustomValues(ISerializer* serializer, bool forUpdate)
{
    OPENDAQ_RETURN_IF_FAILED(Super::serializeCustomValues(serializer, forUpdate));

    return daqTry([&]
    {
        const auto serializerPtr = SerializerPtr::Borrow(serializer);
        if (forUpdate)
        {
            // A property value equal to the default is not saved, and the first option is the default.
            // Without this key the first option of the server would become the first option of the type on load.
            if (transportOptions.getCount() > 0)
            {
                const StringPtr selected = objPtr.getPropertyValue(TransportOptionConfigProperty);
                serializerPtr.key(add_streaming_config::SelectedTransportOptionKey);
                serializerPtr.writeString(selected);
            }

            return;
        }

        serializerPtr.key(add_streaming_config::StreamingTypeKey);
        streamingType.asPtr<ISerializable>().serialize(serializerPtr);

        // The options may be the ones the server of the device announces, which the Streaming type does not list
        if (transportOptions.getCount() > 0)
        {
            serializerPtr.key(add_streaming_config::AvailableTransportOptionsKey);
            transportOptions.serialize(serializerPtr);
        }
    });
}

ErrCode AddStreamingConfigImpl::updateInternal(ISerializedObject* obj, IBaseObject* context)
{
    const ErrCode errCode = Super::updateInternal(obj, context);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    if (errCode == OPENDAQ_IGNORED)
        return errCode;

    return daqTry([&]
    {
        const auto saved = SerializedObjectPtr::Borrow(obj);
        if (saved.hasKey(add_streaming_config::SelectedTransportOptionKey) && streamingType.getAvailableTransportOptions().getCount() > 0)
            checkErrorInfo(setTransportOption(saved.readString(add_streaming_config::SelectedTransportOptionKey)));
    });
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C" ErrCode PUBLIC_EXPORT createAddStreamingConfig(IAddStreamingConfig** objTmp, IStreamingType* streamingType)
{
    return daq::createObject<IAddStreamingConfig, AddStreamingConfigImpl>(objTmp, streamingType);
}

#endif

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AddStreamingConfigImpl)

END_NAMESPACE_OPENDAQ
