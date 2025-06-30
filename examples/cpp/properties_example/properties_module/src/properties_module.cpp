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

    const auto basicTypes = PropertiesFbBasicAndCallback::CreateType();
    types.set(basicTypes.getId(), basicTypes);

    const auto containerTypes = PropertiesFbContainerTypesAndSelection::CreateType();
    types.set(containerTypes.getId(), containerTypes);

    const auto objectsAndClasses = PropertiesFbObjectPropProceduresFunctionsInheritance::CreateType();
    types.set(objectsAndClasses.getId(), objectsAndClasses);

    const auto referenceProperties = PropertiesFbReferencesValidationCoertionConditional::CreateType();
    types.set(referenceProperties.getId(), referenceProperties);

    return types;
}

FunctionBlockPtr PropertiesModule::onCreateFunctionBlock(const StringPtr& id,
                                                         const ComponentPtr& parent,
                                                         const StringPtr& localId,
                                                         const PropertyObjectPtr& /*config*/)
{
    if (id == PropertiesFbBasicAndCallback::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, PropertiesFbBasicAndCallback>(context, parent, localId);
        return fb;
    }
    if (id == PropertiesFbContainerTypesAndSelection::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, PropertiesFbContainerTypesAndSelection>(context, parent, localId);
        return fb;
    }
    if (id == PropertiesFbObjectPropProceduresFunctionsInheritance::CreateType().getId())
    {
        FunctionBlockPtr fb =
            createWithImplementation<IFunctionBlock, PropertiesFbObjectPropProceduresFunctionsInheritance>(context, parent, localId);
        return fb;
    }
    if (id == PropertiesFbReferencesValidationCoertionConditional::CreateType().getId())
    {
        FunctionBlockPtr fb =
            createWithImplementation<IFunctionBlock, PropertiesFbReferencesValidationCoertionConditional>(context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id)
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_PROPERTIES_MODULE
