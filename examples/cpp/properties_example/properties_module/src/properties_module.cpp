#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <properties_module/properties_fb_1.h>
#include <properties_module/properties_fb_2.h>
#include <properties_module/properties_fb_3.h>
#include <properties_module/properties_fb_4.h>
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

    const auto propertiesType1 = PropertiesFb1::CreateType();
    types.set(propertiesType1.getId(), propertiesType1);

    const auto propertiesType2 = PropertiesFb2::CreateType();
    types.set(propertiesType2.getId(), propertiesType2);

    const auto propertiesType3 = PropertiesFb3::CreateType();
    types.set(propertiesType3.getId(), propertiesType3);

    const auto propertiesType4 = PropertiesFb4::CreateType();
    types.set(propertiesType4.getId(), propertiesType4);

    return types;
}

FunctionBlockPtr PropertiesModule::onCreateFunctionBlock(const StringPtr& id,
                                                         const ComponentPtr& parent,
                                                         const StringPtr& localId,
                                                         const PropertyObjectPtr& /*config*/)
{
    if (id == PropertiesFb1::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, PropertiesFb1>(context, parent, localId);
        return fb;
    }
    if (id == PropertiesFb2::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, PropertiesFb2>(context, parent, localId);
        return fb;
    }
    if (id == PropertiesFb3::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, PropertiesFb3>(context, parent, localId);
        return fb;
    }
    if (id == PropertiesFb4::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, PropertiesFb4>(context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id)
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_PROPERTIES_MODULE
