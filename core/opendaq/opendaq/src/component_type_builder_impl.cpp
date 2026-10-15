#include <opendaq/component_type_builder_impl.h>
#include <opendaq/server_type_impl.h>
#include <opendaq/device_type_impl.h>
#include <opendaq/function_block_type_impl.h>
#include <opendaq/streaming_type_impl.h>
#include <coretypes/validation.h>
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

ComponentTypeBuilderImpl::ComponentTypeBuilderImpl(ComponentTypeSort sort)
    : sort(sort)
{
}

ErrCode ComponentTypeBuilderImpl::build(IComponentType** componentType)
{
    const ErrCode errCode = daqTry([&componentType, this]
    {
        const auto builderPtr = this->borrowPtr<ComponentTypeBuilderPtr>();
        switch (sort)
        {
            case ComponentTypeSort::Server:
                *componentType = createWithImplementation<IComponentType, ServerTypeImpl>(builderPtr).detach();
                return OPENDAQ_SUCCESS;
            case ComponentTypeSort::Device:
                *componentType = createWithImplementation<IComponentType, DeviceTypeImpl>(builderPtr).detach();
                return OPENDAQ_SUCCESS;
            case ComponentTypeSort::FunctionBlock:
                *componentType = createWithImplementation<IComponentType, FunctionBlockTypeImpl>(builderPtr).detach();
                return OPENDAQ_SUCCESS;
            case ComponentTypeSort::Streaming:
                *componentType = createWithImplementation<IComponentType, StreamingTypeImpl>(builderPtr).detach();
                return OPENDAQ_SUCCESS;
            case ComponentTypeSort::Undefined:
                break;
        }

        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE);
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode ComponentTypeBuilderImpl::setId(IString* id)
{
    this->id = id;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->id.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::setTypeSort(ComponentTypeSort sort)
{
    this->sort = sort;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getTypeSort(ComponentTypeSort* sort)
{
    OPENDAQ_PARAM_NOT_NULL(sort);

    *sort = this->sort;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::setName(IString* name)
{
    this->name = name;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::setDescription(IString* description)
{
    this->description = description;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);

    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::setConnectionStringPrefix(IString* prefix)
{
    this->prefix = prefix;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getConnectionStringPrefix(IString** prefix)
{
    OPENDAQ_PARAM_NOT_NULL(prefix);

    *prefix = this->prefix.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::setDefaultConfig(IPropertyObject* defaultConfig)
{
    if (this->linkedDeviceType.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_OPERATION,
                                   "setDefaultConfig cannot be used together with a linked device type - the "
                                   "built Streaming type's default config is redirected to the linked device "
                                   "type's own instead");

    this->defaultConfig = defaultConfig;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getDefaultConfig(IPropertyObject** defaultConfig)
{
    OPENDAQ_PARAM_NOT_NULL(defaultConfig);

    *defaultConfig = this->defaultConfig.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::addAuthenticationMethod(IAuthenticationMethod* method)
{
    OPENDAQ_PARAM_NOT_NULL(method);

    if (sort != ComponentTypeSort::Device && sort != ComponentTypeSort::Streaming)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "addAuthenticationMethod only applies to a Device or Streaming type");

    if (this->linkedDeviceType.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_OPERATION,
                                   "addAuthenticationMethod cannot be used together with a linked device type - "
                                   "the built Streaming type's supported authentication methods are redirected "
                                   "to the linked device type's own instead");

    const auto methodPtr = AuthenticationMethodPtr::Borrow(method);
    const StringPtr id = methodPtr.getId();

    if (!this->supportedAuthenticationMethods.assigned())
        this->supportedAuthenticationMethods = Dict<IString, IAuthenticationMethod>();

    if (this->supportedAuthenticationMethods.hasKey(id))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ALREADYEXISTS, "An authentication method with the same id was already added");

    this->supportedAuthenticationMethods.set(id, methodPtr);
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getSupportedAuthenticationMethods(IDict** authenticationMethods)
{
    OPENDAQ_PARAM_NOT_NULL(authenticationMethods);

    *authenticationMethods = this->supportedAuthenticationMethods.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::setLinkedDeviceType(IDeviceType* deviceType)
{
    if (sort != ComponentTypeSort::Streaming)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "setLinkedDeviceType only applies to a Streaming type");

    if (this->defaultConfig.assigned() || (this->supportedAuthenticationMethods.assigned() && this->supportedAuthenticationMethods.getCount() > 0))
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_OPERATION,
                                   "setLinkedDeviceType cannot be used together with a default config already "
                                   "set or authentication methods already added");

    this->linkedDeviceType = deviceType;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getLinkedDeviceType(IDeviceType** deviceType)
{
    OPENDAQ_PARAM_NOT_NULL(deviceType);

    *deviceType = this->linkedDeviceType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createComponentTypeBuilder(IComponentTypeBuilder** objTmp)
{
    return createObject<IComponentTypeBuilder, ComponentTypeBuilderImpl>(objTmp, ComponentTypeSort::Undefined);
}

extern "C"
ErrCode PUBLIC_EXPORT createDeviceTypeBuilder(IComponentTypeBuilder** objTmp)
{
    return createObject<IComponentTypeBuilder, ComponentTypeBuilderImpl>(objTmp, ComponentTypeSort::Device);
}

extern "C"
ErrCode PUBLIC_EXPORT createStreamingTypeBuilder(IComponentTypeBuilder** objTmp)
{
    return createObject<IComponentTypeBuilder, ComponentTypeBuilderImpl>(objTmp, ComponentTypeSort::Streaming);
}

extern "C"
ErrCode PUBLIC_EXPORT createServerTypeBuilder(IComponentTypeBuilder** objTmp)
{
    return createObject<IComponentTypeBuilder, ComponentTypeBuilderImpl>(objTmp, ComponentTypeSort::Server);
}

extern "C"
ErrCode PUBLIC_EXPORT createFunctionBlockTypeBuilder(IComponentTypeBuilder** objTmp)
{
    return createObject<IComponentTypeBuilder, ComponentTypeBuilderImpl>(objTmp, ComponentTypeSort::FunctionBlock);
}

#endif

END_NAMESPACE_OPENDAQ
