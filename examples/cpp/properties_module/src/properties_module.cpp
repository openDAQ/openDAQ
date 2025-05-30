#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <properties_module/properties_fb.h>
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

    const auto propertiesType = PropertiesFb::CreateType();
    types.set(propertiesType.getId(), propertiesType);

    return types;
}

FunctionBlockPtr PropertiesModule::onCreateFunctionBlock(const StringPtr& id,
                                                         const ComponentPtr& parent,
                                                         const StringPtr& localId,
                                                         const PropertyObjectPtr& /*config*/)
{
    if (id == PropertiesFb::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, PropertiesFb>(context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id)
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_PROPERTIES_MODULE
