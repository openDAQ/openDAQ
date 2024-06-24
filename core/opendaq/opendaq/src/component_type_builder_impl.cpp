#include <opendaq/component_type_builder_impl.h>
#include <opendaq/server_type_impl.h>
#include <opendaq/device_type_impl.h>
#include <opendaq/function_block_type_impl.h>
#include <opendaq/streaming_type_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

ComponentTypeBuilderImpl::ComponentTypeBuilderImpl(ComponentTypeSort sort)
    : sort(sort)
{
}

ErrCode ComponentTypeBuilderImpl::build(IComponentType** componentType)
{
    return daqTry([&componentType, this]
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

        return OPENDAQ_ERR_INVALIDTYPE;
    });
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
    this->defaultConfig = defaultConfig;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getDefaultConfig(IPropertyObject** defaultConfig)
{
    OPENDAQ_PARAM_NOT_NULL(defaultConfig);

    *defaultConfig = this->defaultConfig.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::setAltIds(IList* altIds)
{
    this->altIds = altIds;
    return OPENDAQ_SUCCESS;
}

ErrCode ComponentTypeBuilderImpl::getAltIds(IList** altIds)
{
    OPENDAQ_PARAM_NOT_NULL(altIds);

    *altIds = this->altIds.addRefAndReturn();
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
