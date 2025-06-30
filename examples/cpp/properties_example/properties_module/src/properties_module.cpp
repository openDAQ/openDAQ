#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <properties_module/properties_fb_basic_types.h>
#include <properties_module/properties_fb_container_types.h>
#include <properties_module/properties_fb_objects_and_classes.h>
#include <properties_module/properties_fb_reference_properties.h>
#include <properties_module/properties_module.h>
#include <properties_module/version.h>

BEGIN_NAMESPACE_PROPERTIES_MODULE

PropertiesModule::PropertiesModule(ContextPtr ctx)
    : Module("PropertiesModule",
             VersionInfo(PROPERTIES_MODULE_MAJOR_VERSION, PROPERTIES_MODULE_MINOR_VERSION, PROPERTIES_MODULE_PATCH_VERSION),
             std::move(ctx),
             "PropertiesModule")
{
}

DictPtr<IString, IFunctionBlockType> PropertiesModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    const auto basicTypes = ExampleFBPropertyBasicTypes::CreateType();
    types.set(basicTypes.getId(), basicTypes);

    const auto containerTypes = ExampleFBPropertyContainerTypes::CreateType();
    types.set(containerTypes.getId(), containerTypes);

    const auto objectsAndClasses = ExampleFBPropertyObjectsAndClasses::CreateType();
    types.set(objectsAndClasses.getId(), objectsAndClasses);

    const auto referenceProperties = ExampleFBPropertyReferenceProperties::CreateType();
    types.set(referenceProperties.getId(), referenceProperties);

    return types;
}

FunctionBlockPtr PropertiesModule::onCreateFunctionBlock(const StringPtr& id,
                                                         const ComponentPtr& parent,
                                                         const StringPtr& localId,
                                                         const PropertyObjectPtr& /*config*/)
{
    if (id == ExampleFBPropertyBasicTypes::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, ExampleFBPropertyBasicTypes>(context, parent, localId);
        return fb;
    }
    if (id == ExampleFBPropertyContainerTypes::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, ExampleFBPropertyContainerTypes>(context, parent, localId);
        return fb;
    }
    if (id == ExampleFBPropertyObjectsAndClasses::CreateType().getId())
    {
        FunctionBlockPtr fb =
            createWithImplementation<IFunctionBlock, ExampleFBPropertyObjectsAndClasses>(context, parent, localId);
        return fb;
    }
    if (id == ExampleFBPropertyReferenceProperties::CreateType().getId())
    {
        FunctionBlockPtr fb =
            createWithImplementation<IFunctionBlock, ExampleFBPropertyReferenceProperties>(context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id)
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_PROPERTIES_MODULE
