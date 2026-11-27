#include <opendaq/add_streaming_config_impl.h>
#include <coreobjects/property_factory.h>
#include <opendaq/add_component_config_factory.h>
#include <opendaq/authentication_method_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace add_streaming_config
{
static const char* TypeConfig = "TypeConfig";
static const char* AuthenticationConfig = "AuthenticationConfig";

static const char* StreamingTypeKey = "streamingType";
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType)
    : AddStreamingConfigImpl(streamingType, Partial())
{
    Super::addProperty(ObjectProperty(add_streaming_config::TypeConfig, streamingType.createDefaultConfig()));
    if (const auto authenticationConfig = streamingType.createAuthenticationConfig(); authenticationConfig.assigned())
        Super::addProperty(ObjectProperty(add_streaming_config::AuthenticationConfig, authenticationConfig));
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType, Partial)
    : Super()
    , streamingType(streamingType)
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

    return daqTry([&]
    {
        *config = objPtr.hasProperty(add_streaming_config::AuthenticationConfig)
                      ? objPtr.getPropertyValue(add_streaming_config::AuthenticationConfig).asPtr<IAuthenticationConfig>().detach()
                      : nullptr;
    });
}

ErrCode AddStreamingConfigImpl::bind(IDict* streamingTypes, IAddStreamingConfig** bound)
{
    OPENDAQ_PARAM_NOT_NULL(bound);

    return daqTry([&]
    {
        const auto types = DictPtr<IString, IStreamingType>::Borrow(streamingTypes);
        const StringPtr typeId = streamingType.getId();
        if (!types.assigned() || !types.hasKey(typeId))
            DAQ_THROW_EXCEPTION(NotFoundException, "Streaming type \"{}\" is not available", typeId);

        // A JSON round trip applies the values of this config to one created for the given type, as the bind of an add device config does.
        const auto serializer = JsonSerializer();
        checkErrorInfo(serializeForUpdate(serializer));

        auto config = AddStreamingConfig(types.get(typeId));
        const auto update = [&config](const SerializedObjectPtr& saved)
        {
            checkErrorInfo(config.asPtr<IUpdatable>()->updateInternal(saved, nullptr));
        };

        try
        {
            JsonDeserializer().callCustomProc(update, serializer.getOutput());
        }
        catch (const DaqException& e)
        {
            DAQ_THROW_EXCEPTION(InvalidValueException, "{}", e.what());
        }

        ClearLegacyCredentials(config.getTypeConfig(), config.getAuthenticationConfig());
        *bound = config.detach();
    });
}

void AddStreamingConfigImpl::ClearLegacyCredentials(const PropertyObjectPtr& typeConfig,
                                                    const AuthenticationConfigPtr& authenticationConfig)
{
    if (!authenticationConfig.assigned() || !authenticationConfig.getAuthenticationMethods().hasKey(StandardUserNamePasswordId))
        return;

    for (const auto name : {"Username", "Password"})
        if (typeConfig.hasProperty(name))
            typeConfig.clearPropertyValue(name);
}

ErrCode AddStreamingConfigImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    return daqTry([&]
    {
        auto lock = getRecursiveConfigLock2();
        auto obj = createWithImplementation<IPropertyObject, AddStreamingConfigImpl>(streamingType, Partial());
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
            return createWithImplementation<IAddStreamingConfig, AddStreamingConfigImpl>(streamingType, Partial());
        };

        *obj = Super::DeserializePropertyObject(serialized, context, factoryCallback, createConfig).detach();
    });
}

ErrCode AddStreamingConfigImpl::serializeCustomValues(ISerializer* serializer, bool forUpdate)
{
    OPENDAQ_RETURN_IF_FAILED(Super::serializeCustomValues(serializer, forUpdate));
    if (forUpdate)
        return OPENDAQ_SUCCESS;

    return daqTry([&]
    {
        const auto serializerPtr = SerializerPtr::Borrow(serializer);
        serializerPtr.key(add_streaming_config::StreamingTypeKey);
        streamingType.asPtr<ISerializable>().serialize(serializerPtr);
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
